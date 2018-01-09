/* Playready Session management
 *
 * Copyright (C) 2016 Igalia S.L
 * Copyright (C) 2016 Metrological
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#ifndef PlayreadySession_h
#define PlayreadySession_h

#if USE(PLAYREADY)

#include <drmtypes.h>
#include <drmcommon.h>
#include <drmmanager.h>
#include <drmmathsafe.h>
#undef __in
#undef __out
#include <runtime/Uint8Array.h>
#include <wtf/Forward.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class PlayreadySession {

private:
    enum KeyState {
        // Has been initialized.
        KEY_INIT = 0,
        // Has a key message pending to be processed.
        KEY_PENDING = 1,
        // Has a usable key.
        KEY_READY = 2,
        // Has an error.
        KEY_ERROR = 3,
        // Has been closed.
        KEY_CLOSED = 4
    };

public:
    PlayreadySession();
    ~PlayreadySession();

    /**
     * initData is the data in the PSSH box without the header, etc (see librcvmf pssh)
     *   NOTE(estobb200): the manifest usually contains the entire pssh box, so it needs be stripped
     *   before passing as an arg.
     * customData is the data in the HTMLMEdiaElement's generateKeyRequest passed to MediaPlayerPrivate
     * destinationURL is decoded from the initData and assgined
     * errorCode and systemCode are sort of broken, don't rely on them here.
     */
    RefPtr<Uint8Array> playreadyGenerateKeyRequest(Uint8Array* initData, const String& customData, String& destinationURL, unsigned short& errorCode, uint32_t& systemCode);
    bool playreadyProcessKey(Uint8Array* key, unsigned short& errorCode, uint32_t& systemCode);

    bool keyRequested() const { return m_eKeyState == KEY_PENDING; }
    bool ready() const { return m_eKeyState == KEY_READY; }
    bool init() const { return m_eKeyState == KEY_INIT; }
    int processPayload(const void* iv, uint32_t ivSize, void* payloadData, uint32_t payloadDataSize);

    // Helper for PlayreadySession clients.
    Lock& mutex() { return m_prSessionMutex; }
    const String& sessionId() { return m_sessionId; }

protected:
    RefPtr<ArrayBuffer> m_key;

private:
    static DRM_RESULT DRM_CALL _PolicyCallback(const DRM_VOID* f_pvOutputLevelsData, DRM_POLICY_CALLBACK_TYPE f_dwCallbackType, const DRM_VOID* f_pv);

    DRM_APP_CONTEXT* m_poAppContext { nullptr };
    DRM_DECRYPT_CONTEXT m_oDecryptContext;
    DRM_AES_COUNTER_MODE_CONTEXT m_oAESContext;

    DRM_BYTE* m_pbOpaqueBuffer { nullptr };
    DRM_DWORD m_cbOpaqueBuffer;

    DRM_BYTE* m_pbRevocationBuffer { nullptr };
    KeyState m_eKeyState;
    DRM_CHAR m_rgchSessionID[CCH_BASE64_EQUIV(SIZEOF(DRM_ID)) + 1];
    DRM_BOOL m_fCommit;

    Lock m_prSessionMutex;
    String m_sessionId;
};

}
#endif

#endif
