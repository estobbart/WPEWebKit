/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Alp Toker <alp.toker@collabora.co.uk>
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CurlFormDataStream.h"

#if USE(CURL)

#include "CurlContext.h"
#include "Logging.h"
#include <wtf/MainThread.h>

namespace WebCore {

CurlFormDataStream::CurlFormDataStream(const FormData* formData)
{
    ASSERT(isMainThread());

    if (!formData || formData->isEmpty())
        return;

    m_formData = formData->isolatedCopy();

    // Resolve the blob elements so the formData can correctly report it's size.
    m_formData = m_formData->resolveBlobReferences();
}

CurlFormDataStream::~CurlFormDataStream()
{

}

void CurlFormDataStream::clean()
{
    if (m_postData)
        m_postData = nullptr;

    if (m_fileHandle != FileSystem::invalidPlatformFileHandle) {
        FileSystem::closeFile(m_fileHandle);
        m_fileHandle = FileSystem::invalidPlatformFileHandle;
    }
}

std::optional<const Vector<char>&> CurlFormDataStream::getPostData()
{
    if (!m_formData)
        return std::nullopt;

    if (!m_postData)
        m_postData = std::make_unique<Vector<char>>(m_formData->flatten());

    return *m_postData;
}

bool CurlFormDataStream::shouldUseChunkTransfer()
{
    computeContentLength();

    return m_shouldUseChunkTransfer;
}

unsigned long long CurlFormDataStream::totalSize()
{
    computeContentLength();

    return m_totalSize;
}

void CurlFormDataStream::computeContentLength()
{
    static auto maxCurlOffT = CurlHandle::maxCurlOffT();

    if (!m_formData || m_isContentLengthUpdated)
        return;

    m_isContentLengthUpdated = true;

    for (const auto& element : m_formData->elements()) {
        if (element.m_type == FormDataElement::Type::EncodedFile) {
            long long fileSize;
            if (FileSystem::getFileSize(element.m_filename, fileSize)) {
                if (fileSize > maxCurlOffT) {
                    // File size is too big for specifying it to curl
                    m_shouldUseChunkTransfer = true;
                }

                m_totalSize += fileSize;
            } else
                m_shouldUseChunkTransfer = true;
        } else
            m_totalSize += element.m_data.size();
    }
}

std::optional<size_t> CurlFormDataStream::read(char* buffer, size_t size)
{
    if (!m_formData)
        return std::nullopt;

    const auto totalElementSize = m_formData->elements().size();
    if (m_elementPosition >= totalElementSize)
        return 0;

    size_t totalReadBytes = 0;

    while ((m_elementPosition < totalElementSize) && (totalReadBytes < size)) {
        const auto& element = m_formData->elements().at(m_elementPosition);

        std::optional<size_t> readBytes;
        size_t bufferSize = size - totalReadBytes;
        char* bufferPosition = buffer + totalReadBytes;

        if (element.m_type == FormDataElement::Type::EncodedFile)
            readBytes = readFromFile(element, bufferPosition, bufferSize);
        else
            readBytes = readFromData(element, bufferPosition, bufferSize);

        if (!readBytes)
            return std::nullopt;

        totalReadBytes += *readBytes;
    }

    m_totalReadSize += totalReadBytes;

    return totalReadBytes;
}

std::optional<size_t> CurlFormDataStream::readFromFile(const FormDataElement& element, char* buffer, size_t size)
{
    if (m_fileHandle == FileSystem::invalidPlatformFileHandle)
        m_fileHandle = FileSystem::openFile(element.m_filename, FileSystem::FileOpenMode::Read);

    if (!FileSystem::isHandleValid(m_fileHandle)) {
        LOG(Network, "Curl - Failed while trying to open %s for upload\n", element.m_filename.utf8().data());
        m_fileHandle = FileSystem::invalidPlatformFileHandle;
        return std::nullopt;
    }

    auto readBytes = FileSystem::readFromFile(m_fileHandle, buffer, size);
    if (readBytes < 0) {
        LOG(Network, "Curl - Failed while trying to read %s for upload\n", element.m_filename.utf8().data());
        FileSystem::closeFile(m_fileHandle);
        m_fileHandle = FileSystem::invalidPlatformFileHandle;
        return std::nullopt;
    }

    if (!readBytes) {
        FileSystem::closeFile(m_fileHandle);
        m_fileHandle = FileSystem::invalidPlatformFileHandle;
        m_elementPosition++;
    }

    return readBytes;
}

std::optional<size_t> CurlFormDataStream::readFromData(const FormDataElement& element, char* buffer, size_t size)
{
    size_t elementSize = element.m_data.size() - m_dataOffset;
    const char* elementBuffer = element.m_data.data() + m_dataOffset;

    size_t readBytes = elementSize > size ? size : elementSize;
    memcpy(buffer, elementBuffer, readBytes);

    if (elementSize > readBytes)
        m_dataOffset += readBytes;
    else {
        m_dataOffset = 0;
        m_elementPosition++;
    }

    return readBytes;
}

} // namespace WebCore

#endif
