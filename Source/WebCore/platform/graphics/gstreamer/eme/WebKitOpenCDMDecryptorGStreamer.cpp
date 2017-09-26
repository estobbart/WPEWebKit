/* GStreamer OpenCDM decryptor
 *
 * Copyright (C) 2016-2017 TATA ELXSI
 * Copyright (C) 2016-2017 Metrological
 * Copyright (C) 2016-2017 Igalia S.L
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

#include "config.h"
#include "WebKitOpenCDMDecryptorGStreamer.h"

#if (ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)) && USE(GSTREAMER) && USE(OPENCDM)

#include "GUniquePtrGStreamer.h"

#include <open_cdm.h>
#include <wtf/text/WTFString.h>

#if USE(SVP)
#include <gst_brcm_svp_meta.h>
#include "b_secbuf.h"

struct Rpc_Secbuf_Info {
    uint8_t *ptr;
    uint32_t type;
    size_t   size;
    void    *token;
};
#endif

#define GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_OPENCDM_DECRYPT, WebKitOpenCDMDecryptPrivate))

struct _WebKitOpenCDMDecryptPrivate {
    String m_session;
    std::unique_ptr<media::OpenCdm> m_openCdm;
};

static void webKitMediaOpenCDMDecryptorFinalize(GObject*);
static gboolean webKitMediaOpenCDMDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt*, GstEvent*);
static gboolean webKitMediaOpenCDMDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer*, GstBuffer*, unsigned, GstBuffer*);

GST_DEBUG_CATEGORY(webkit_media_opencdm_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_opencdm_decrypt_debug_category

#define webkit_media_opencdm_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitOpenCDMDecrypt, webkit_media_opencdm_decrypt, WEBKIT_TYPE_MEDIA_CENC_DECRYPT);

static void webkit_media_opencdm_decrypt_class_init(WebKitOpenCDMDecryptClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = webKitMediaOpenCDMDecryptorFinalize;

    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(elementClass,
        "Decrypt content with OpenCDM support",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decrypts media with OpenCDM support",
        "TataElxsi");

    GST_DEBUG_CATEGORY_INIT(webkit_media_opencdm_decrypt_debug_category,
        "webkitopencdm", 0, "OpenCDM decryptor");

    WebKitMediaCommonEncryptionDecryptClass* cencClass = WEBKIT_MEDIA_CENC_DECRYPT_CLASS(klass);
    cencClass->handleKeyResponse = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorHandleKeyResponse);
    cencClass->decrypt = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorDecrypt);

    g_type_class_add_private(klass, sizeof(WebKitOpenCDMDecryptPrivate));
}

static void webkit_media_opencdm_decrypt_init(WebKitOpenCDMDecrypt* self)
{
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(self);
    self->priv = priv;
    new (priv) WebKitOpenCDMDecryptPrivate();
}

static void webKitMediaOpenCDMDecryptorFinalize(GObject* object)
{
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_DECRYPT(object));
    priv->m_openCdm->ReleaseMem();
    priv->~WebKitOpenCDMDecryptPrivate();
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static gboolean webKitMediaOpenCDMDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent* event)
{
    const GstStructure* structure = gst_event_get_structure(event);
    if (!gst_structure_has_name(structure, "drm-session"))
        return false;

    GUniqueOutPtr<char> temporarySession;
    gst_structure_get(structure, "session", G_TYPE_STRING, &temporarySession.outPtr(), nullptr);
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_DECRYPT(self));
    ASSERT(temporarySession);

    if(priv->m_session != temporarySession.get() ) {
        priv->m_session = temporarySession.get();
        GST_WARNING_OBJECT(self, "selecting session %s", priv->m_session.utf8().data());
        priv->m_openCdm = std::make_unique<media::OpenCdm>();
        priv->m_openCdm->SelectSession(priv->m_session.utf8().data());
    }else{
        GST_WARNING_OBJECT(self, "session already selected! %s", priv->m_session.utf8().data());
    }
    return true;
}

static gboolean webKitMediaOpenCDMDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* ivBuffer, GstBuffer* buffer, unsigned subSampleCount, GstBuffer* subSamplesBuffer)
{
#if USE(SVP)
    struct Rpc_Secbuf_Info sb_info;
#endif
    GstMapInfo ivMap;
    if (!gst_buffer_map(ivBuffer, &ivMap, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map IV");
        return false;
    }

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, static_cast<GstMapFlags>(GST_MAP_READWRITE))) {
        gst_buffer_unmap(ivBuffer, &ivMap);
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        return false;
    }

    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_DECRYPT(self));
    ASSERT(priv->sessionMetaData);

    int errorCode;
    bool returnValue = true;
    if (subSamplesBuffer) {
        GstMapInfo subSamplesMap;
        if (!gst_buffer_map(subSamplesBuffer, &subSamplesMap, GST_MAP_READ)) {
            GST_ERROR_OBJECT(self, "Failed to map subsample buffer");
            returnValue = false;
            goto beach;
        }

        GUniquePtr<GstByteReader> reader(gst_byte_reader_new(subSamplesMap.data, subSamplesMap.size));

        uint16_t inClear = 0;
        uint32_t inEncrypted = 0;
        uint32_t totalEncrypted = 0;
        unsigned position;
        unsigned total = 0;
        // Find out the total size of the encrypted data.
        for (position = 0; position < subSampleCount; position++) {
            gst_byte_reader_get_uint16_be(reader.get(), &inClear);
            gst_byte_reader_get_uint32_be(reader.get(), &inEncrypted);
            totalEncrypted += inEncrypted;
            total += + inClear + inEncrypted;
        }
        gst_byte_reader_set_pos(reader.get(), 0);

        if (total != map.size) {
            GST_ERROR_OBJECT(self, "Subsample byte total != buffer.size, unable to decrypt");
            returnValue = false;
            goto beach;
        }

        if (totalEncrypted > 0)
        {
#if USE(SVP)
            brcm_svp_meta_data_t * ptr = (brcm_svp_meta_data_t *) g_malloc(sizeof(brcm_svp_meta_data_t));
            if (ptr) {
                uint32_t chunks_cnt = 0;
                svp_chunk_info * ci = NULL;
                //uint32_t clear_start = 0;
                memset((uint8_t *)ptr, 0, sizeof(brcm_svp_meta_data_t));
                ptr->sub_type = GST_META_BRCM_SVP_TYPE_2;
                // The next line need to change to assign the opaque handle after calling ->processPayload()
                ptr->u.u2.secbuf_ptr = NULL; //pData;
                chunks_cnt = subSampleCount;
                ptr->u.u2.chunks_cnt = chunks_cnt;
                if (chunks_cnt) {
                    ci = (svp_chunk_info *)g_malloc(chunks_cnt * sizeof(svp_chunk_info));
                    ptr->u.u2.chunk_info = ci;
                }
            }
            GST_TRACE_OBJECT(self, "[HHH] subSampleCount=%i.\n", subSampleCount);
            totalEncrypted += sizeof(Rpc_Secbuf_Info); //make sure enough data for metadata
#endif

            // Build a new buffer storing the entire encrypted cipher.
            GUniquePtr<uint8_t> holdEncryptedData(reinterpret_cast<uint8_t*>(malloc(totalEncrypted)));
            uint8_t* encryptedData = holdEncryptedData.get();
            unsigned index = 0;
            for (position = 0; position < subSampleCount; position++) {
                gst_byte_reader_get_uint16_be(reader.get(), &inClear);
                gst_byte_reader_get_uint32_be(reader.get(), &inEncrypted);
                memcpy(encryptedData, map.data + index + inClear, inEncrypted);
                index += inClear + inEncrypted;
                encryptedData += inEncrypted;
#if USE(SVP)
                if (ptr && ptr->u.u2.chunks_cnt > 0 && ptr->u.u2.chunk_info) {
                    svp_chunk_info * ci = ptr->u.u2.chunk_info;
                    ci[position].clear_size = inClear;
                    ci[position].encrypted_size = inEncrypted;
                }
#endif
            }
            gst_byte_reader_set_pos(reader.get(), 0);

            // Decrypt cipher.
            if (errorCode = priv->m_openCdm->Decrypt(holdEncryptedData.get(), static_cast<uint32_t>(totalEncrypted),
                ivMap.data, static_cast<uint32_t>(ivMap.size))) {
                GST_WARNING_OBJECT(self, "ERROR - packet decryption failed [%d]", errorCode);
                gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
                returnValue = false;
                goto beach;
            }

#if USE(SVP)
            // Update the opaque handle and push the metadata to gstbuffer
            GST_TRACE_OBJECT(self, "\n[HHH] after decryption with subsamples.\n");
            encryptedData = holdEncryptedData.get();
            if (ptr) {
                memcpy(&sb_info, encryptedData, sizeof(Rpc_Secbuf_Info));
                GST_TRACE_OBJECT(self, "[HHH] sb_inf: ptr=%p, type=%i, size=%i, token=%p\n", sb_info.ptr, sb_info.type, sb_info.size, sb_info.token);
                if (B_Secbuf_AllocWithToken(sb_info.size, (B_Secbuf_Type)sb_info.type, sb_info.token, &sb_info.ptr)) {
                    GST_ERROR_OBJECT(self, "[HHH] B_Secbuf_AllocWithToken() failed!\n");
                } else {
                    GST_TRACE_OBJECT(self, "[HHH] B_Secbuf_AllocWithToken() succeeded.\n");
                }
                ptr->u.u2.secbuf_ptr = sb_info.ptr; //assign the handle here!
                gst_buffer_add_brcm_svp_meta(buffer, ptr);
            }
#else
            // Re-build sub-sample data.
            index = 0;
            encryptedData = holdEncryptedData.get();
            total = 0;
            for (position = 0; position < subSampleCount; position++) {
                gst_byte_reader_get_uint16_be(reader.get(), &inClear);
                gst_byte_reader_get_uint32_be(reader.get(), &inEncrypted);

                memcpy(map.data + total + inClear, encryptedData + index, inEncrypted);
                index += inEncrypted;
                total += inClear + inEncrypted;
            }
#endif
        }
        gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
        if (totalEncrypted == 0) {
            GST_WARNING_OBJECT(self, "totalEncrypted is 0, not calling decrypt()");
            return returnValue;
        }
    } else {
        uint8_t* encryptedData;
        uint8_t* fEncryptedData;
        uint32_t totalEncryptedSize = 0;
#if USE(SVP)
        brcm_svp_meta_data_t * ptr = (brcm_svp_meta_data_t *) g_malloc(sizeof(brcm_svp_meta_data_t));
        if (ptr) {
            svp_chunk_info * ci = NULL;
            memset((uint8_t *)ptr, 0, sizeof(brcm_svp_meta_data_t));
            ptr->sub_type = GST_META_BRCM_SVP_TYPE_2;
            // The next line need to change to assign the opaque handle after calling ->processPayload()
            ptr->u.u2.secbuf_ptr = NULL; //pData;
            ptr->u.u2.chunks_cnt = 1;

            ci = (svp_chunk_info *)g_malloc(sizeof(svp_chunk_info));
            ptr->u.u2.chunk_info = ci;
            ci[0].clear_size = 0;
            ci[0].encrypted_size = map.size;

            totalEncryptedSize = map.size + sizeof(Rpc_Secbuf_Info); //make sure it is enough for metadata
            encryptedData = (uint8_t*) g_malloc(totalEncryptedSize);
            fEncryptedData = encryptedData;
            memcpy(encryptedData, map.data, map.size);
        }
#else
        encryptedData = map.data;
        fEncryptedData = encryptedData;
        totalEncryptedSize = map.size;
#endif
        // Decrypt cipher.
        if (errorCode = priv->m_openCdm->Decrypt(encryptedData, static_cast<uint32_t>(totalEncryptedSize),
            ivMap.data, static_cast<uint32_t>(ivMap.size))) {
            GST_WARNING_OBJECT(self, "ERROR - packet decryption failed [%d]", errorCode);
            returnValue = false;
            g_free(fEncryptedData);
            goto beach;
        }
#if USE(SVP)
        // Update the opaque handle and push the metadata to gstbuffer
        GST_TRACE_OBJECT(self, "[HHH] after decryption - single sample buffer\n");
        if (ptr) {
            memcpy(&sb_info, fEncryptedData, sizeof(Rpc_Secbuf_Info));
            GST_TRACE_OBJECT(self, "[HHH] sb_inf: ptr=%p, type=%i, size=%i, token=%p\n", sb_info.ptr, sb_info.type, sb_info.size, sb_info.token);
            if (B_Secbuf_AllocWithToken(sb_info.size, (B_Secbuf_Type)sb_info.type, sb_info.token, &sb_info.ptr)) {
                GST_ERROR_OBJECT(self, "[HHH] B_Secbuf_AllocWithToken() failed!\n");
            } else {
                GST_TRACE_OBJECT(self, "[HHH] B_Secbuf_AllocWithToken() succeeded.\n");
            }
            ptr->u.u2.secbuf_ptr = sb_info.ptr; //assign the handle here!
            gst_buffer_add_brcm_svp_meta(buffer, ptr);
        }
        g_free(fEncryptedData);
#endif
    }

beach:
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unmap(ivBuffer, &ivMap);
    return returnValue;
}

#endif // (ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)) && USE(GSTREAMER) && USE(OPENCDM)
