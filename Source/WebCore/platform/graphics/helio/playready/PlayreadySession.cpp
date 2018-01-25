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

#include "config.h"
#include "PlayreadySession.h"
#include "drmerror.h"

#if USE(PLAYREADY)
//#include "MediaPlayerPrivateGStreamer.h"
#include "UUID.h" // TODO: Where is this file?

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
#include "WebKitMediaKeyError.h"
#endif

#include <runtime/JSCInlines.h>
#include <runtime/TypedArrayInlines.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/CString.h>

// GST_DEBUG_CATEGORY_EXTERN(webkit_media_playready_decrypt_debug_category);
// #define GST_CAT_DEFAULT webkit_media_playready_decrypt_debug_category
//
// G_LOCK_DEFINE_STATIC (pr_decoder_lock);

extern DRM_CONST_STRING g_dstrDrmPath;

namespace WebCore {

// The default location of CDM DRM store.
// /opt/drm/playready/drmstore.dat
const DRM_WCHAR g_rgwchCDMDrmStoreName[] = {
    WCHAR_CAST('/'), WCHAR_CAST('o'), WCHAR_CAST('p'), WCHAR_CAST('t'),
    WCHAR_CAST('/'), WCHAR_CAST('d'), WCHAR_CAST('r'), WCHAR_CAST('m'),
    WCHAR_CAST('/'), WCHAR_CAST('p'), WCHAR_CAST('l'), WCHAR_CAST('a'),
    WCHAR_CAST('y'), WCHAR_CAST('r'), WCHAR_CAST('e'), WCHAR_CAST('a'),
    WCHAR_CAST('d'), WCHAR_CAST('y'), WCHAR_CAST('/'), WCHAR_CAST('d'),
    WCHAR_CAST('r'), WCHAR_CAST('m'), WCHAR_CAST('s'), WCHAR_CAST('t'),
    WCHAR_CAST('o'), WCHAR_CAST('r'), WCHAR_CAST('e'), WCHAR_CAST('.'),
    WCHAR_CAST('d'), WCHAR_CAST('a'), WCHAR_CAST('t'), WCHAR_CAST('\0')
};

// The default location of CDM DRM store.
// /tmp/drmstore.dat
// const DRM_WCHAR g_rgwchCDMDrmStoreName[] = {WCHAR_CAST('/'), WCHAR_CAST('t'), WCHAR_CAST('m'), WCHAR_CAST('p'), WCHAR_CAST('/'),
//                                             WCHAR_CAST('d'), WCHAR_CAST('r'), WCHAR_CAST('m'), WCHAR_CAST('s'), WCHAR_CAST('t'),
//                                             WCHAR_CAST('o'), WCHAR_CAST('r'), WCHAR_CAST('e'), WCHAR_CAST('.'), WCHAR_CAST('d'),
//                                             WCHAR_CAST('a'), WCHAR_CAST('t'), WCHAR_CAST('\0')};

const DRM_CONST_STRING g_dstrCDMDrmStoreName = CREATE_DRM_STRING(g_rgwchCDMDrmStoreName);

const DRM_CONST_STRING* g_rgpdstrRights[1] = {&g_dstrWMDRM_RIGHT_PLAYBACK};

// default PR DRM path
// /opt/drm/playready
const DRM_WCHAR g_rgwchCDMDrmPath[] = {
    WCHAR_CAST('/'), WCHAR_CAST('o'), WCHAR_CAST('p'), WCHAR_CAST('t'),
    WCHAR_CAST('/'), WCHAR_CAST('d'), WCHAR_CAST('r'), WCHAR_CAST('m'),
    WCHAR_CAST('/'), WCHAR_CAST('p'), WCHAR_CAST('l'), WCHAR_CAST('a'),
    WCHAR_CAST('y'), WCHAR_CAST('r'), WCHAR_CAST('e'), WCHAR_CAST('a'),
    WCHAR_CAST('d'), WCHAR_CAST('y'), WCHAR_CAST('\0')
};

const DRM_CONST_STRING g_dstrCDMDrmPath = CREATE_DRM_STRING(g_rgwchCDMDrmPath);

struct _PlayreadySession : PlayreadySession {
    using PlayreadySession::_drmSessionInit;
    using PlayreadySession::_nextTask;
};

static void *playready_start(void *playreadySession) {
    printf("PlayreadySession playready_start thread START\n");
    _PlayreadySession *prSession = static_cast<_PlayreadySession *>(playreadySession);

    //static_cast<MediaPlayerPrivateHelio*>(helioPlayer)->decryptionSessionStarted(std::move(std::make_unique<PlayreadySession>()));
    // while(static_cast<MediaPlayerPrivateHelio*>(helioPlayer)->nextDecryptionSessionTask()) {
    //
    // }
    // TODO: Signal the condition so that the constructor returns
    printf("prSession->_drmSessionInit\n");
    prSession->_drmSessionInit();
    printf("prSession->_nextTask\n");
    while (prSession->_nextTask()) { }
    printf("PlayreadySession playready_start thread END\n");
}

PlayreadySession* PlayreadySession::gPlayreadySession = nullptr;
std::queue<std::function<void(PlayreadySession *)>> PlayreadySession::prTaskQueue;
std::queue<std::function<void(PlayreadySession *)>> PlayreadySession::prDecryptQueue;
pthread_cond_t PlayreadySession::queueCond;
pthread_mutex_t PlayreadySession::queueMutex;

static void PlayreadySession::playreadyTask(TaskPriority priority, std::function< void (PlayreadySession*) > task) {
    printf("PlayreadySession::playreadyTask %i\n", priority);
    if (gPlayreadySession == nullptr) {
        // TODO: This should occur in the thread and let the function
        // quickly return..
        // Or let the thread do the actual init...
        printf("gPlayreadySession = new PlayreadySession\n");
        gPlayreadySession = new PlayreadySession();
    }

    printf("playreadyTask pthread_mutex_lock(&queueMutex);\n");
    pthread_mutex_lock(&queueMutex);
    if (priority == KEY) {
        prTaskQueue.push(task);
    } else if (priority == DECRYPT) {
        prDecryptQueue.push(task);
    }
    pthread_cond_signal(&queueCond);
    pthread_mutex_unlock(&queueMutex);
    printf("playreadyTask pthread_mutex_unlock(&queueMutex);\n");
}

PlayreadySession::PlayreadySession()
    : m_key()
    , m_poAppContext(nullptr)
    , m_pbOpaqueBuffer(nullptr)
    , m_pbRevocationBuffer(nullptr)
    , m_eKeyState(KEY_INIT)
    , m_fCommit(FALSE)
    , m_sessionId(createCanonicalUUIDString())
{
    printf("PlayreadySession::PlayreadySession()\n");
    pthread_cond_init(&queueCond, NULL);
    pthread_mutex_init(&queueMutex, NULL);

    pthread_mutex_lock(&queueMutex);
    pthread_create(&pr_thread_id, NULL, playready_start, this);

    pthread_cond_wait(&queueCond, &queueMutex);
    pthread_mutex_unlock(&queueMutex);
}

PlayreadySession::~PlayreadySession()
{
    printf("PlayreadySession Releasing resources\n");

    Drm_Uninitialize (m_poAppContext);

    if (DRM_REVOCATION_IsRevocationSupported())
        SAFE_OEM_FREE(m_pbRevocationBuffer);

    SAFE_OEM_FREE(m_pbOpaqueBuffer);
    SAFE_OEM_FREE(m_poAppContext);
}

void PlayreadySession::resetSession() {
    pthread_mutex_lock(&queueMutex);

    prTaskQueue = std::queue<std::function<void(PlayreadySession *)>>();
    prDecryptQueue = std::queue<std::function<void(PlayreadySession *)>>();

    m_eKeyState = KEY_INIT;
    m_sessionId = createCanonicalUUIDString();

    pthread_mutex_unlock(&queueMutex);
}

bool PlayreadySession::_nextTask() {

    pthread_mutex_lock(&queueMutex);
    // PlayreadySession::_nextTask pthread_cond_wait 0 1
    if (prTaskQueue.empty() && (prDecryptQueue.empty() || m_eKeyState != KEY_READY)) {
        printf("PlayreadySession::_nextTask pthread_cond_wait %i %i\n",
               prDecryptQueue.empty(), (prDecryptQueue.empty() || m_eKeyState < KEY_READY));
        pthread_cond_wait(&queueCond, &queueMutex);
    }

    bool keyTask = !prTaskQueue.empty();
    printf("PlayreadySession::_nextTask pthread_cond_wait resume %s task.\n", keyTask ? "keyTask" : "decryptTask");
    printf("PlayreadySession::_nextTask remaing %i \n", keyTask ? prTaskQueue.size() : prDecryptQueue.size());
    auto fn = keyTask ? prTaskQueue.front() : prDecryptQueue.front();
    pthread_mutex_unlock(&queueMutex);

    fn(this);

    pthread_mutex_lock(&queueMutex);

    // Queue  could be modified by the resetSession
    if (!prTaskQueue.empty() || !prDecryptQueue.empty()) {
        keyTask ? prTaskQueue.pop() : prDecryptQueue.pop();
    }
    printf("PlayreadySession::_nextTask task popped, %i left\n", keyTask ? prTaskQueue.size() : prDecryptQueue.size());
    pthread_mutex_unlock(&queueMutex);

    return true;
}

void PlayreadySession::_drmSessionInit() {
    printf("void PlayreadySession::_drmSessionInit() \n");
    pthread_mutex_lock(&queueMutex);
    pthread_cond_signal(&queueCond);
    pthread_mutex_unlock(&queueMutex);


    DRM_RESULT dr = DRM_SUCCESS;
    DRM_ID oSessionID;
    DRM_DWORD cchEncodedSessionID = SIZEOF(m_rgchSessionID);
    m_oAESContext = { 0 };

    ChkMem(m_pbOpaqueBuffer = (DRM_BYTE *)Oem_MemAlloc(MINIMUM_APPCONTEXT_OPAQUE_BUFFER_SIZE));
    m_cbOpaqueBuffer = MINIMUM_APPCONTEXT_OPAQUE_BUFFER_SIZE;

    ChkMem(m_poAppContext = (DRM_APP_CONTEXT *)Oem_MemAlloc(SIZEOF(DRM_APP_CONTEXT)));
    ZEROMEM(m_poAppContext, SIZEOF(DRM_APP_CONTEXT));

    g_dstrDrmPath = g_dstrCDMDrmPath;

    // Initialize DRM app context.
    printf("PlayreadySession -> Drm_Initialize %s\n", g_dstrCDMDrmStoreName);
    ChkDR(Drm_Initialize(m_poAppContext,
                         NULL,
                         m_pbOpaqueBuffer,
                         m_cbOpaqueBuffer,
                         &g_dstrCDMDrmStoreName));

    if (DRM_REVOCATION_IsRevocationSupported()) {
        ChkMem(m_pbRevocationBuffer = (DRM_BYTE *)Oem_MemAlloc(REVOCATION_BUFFER_SIZE));

        printf("PlayreadySession -> Drm_Revocation_SetBuffer\n");
        ChkDR(Drm_Revocation_SetBuffer(m_poAppContext,
                                       m_pbRevocationBuffer,
                                       REVOCATION_BUFFER_SIZE));
    }

    // Generate a random media session ID.
    printf("PlayreadySession -> Oem_Random_GetBytes\n");
    ChkDR(Oem_Random_GetBytes(NULL, (DRM_BYTE *)&oSessionID, SIZEOF(oSessionID)));

    ZEROMEM(m_rgchSessionID, SIZEOF(m_rgchSessionID));
    // Store the generated media session ID in base64 encoded form.
    printf("PlayreadySession -> DRM_B64_EncodeA\n");
    ChkDR(DRM_B64_EncodeA((DRM_BYTE *)&oSessionID,
                          SIZEOF(oSessionID),
                          m_rgchSessionID,
                          &cchEncodedSessionID,
                          0));

    printf("PlayreadySession initialized\n");

 ErrorExit:
    if (DRM_FAILED(dr)) {
        DRM_CHAR *errName = NULL;
        DRM_ERR_GetErrorNameFromCode(dr, &errName);
        m_eKeyState = KEY_ERROR;
        printf("Playready ERROR initialization failed: %s\n", errName);
    }
}

// PlayReady license policy callback which should be
// customized for platform/environment that hosts the CDM.
// It is currently implemented as a place holder that
// does nothing.
DRM_RESULT DRM_CALL PlayreadySession::_PolicyCallback(const DRM_VOID *, DRM_POLICY_CALLBACK_TYPE, const DRM_VOID *)
{
    return DRM_SUCCESS;
}

//
// Expected synchronisation from caller. This method is not thread-safe!
//
RefPtr<Uint8Array> PlayreadySession::playreadyGenerateKeyRequest(Uint8Array* initData, const String& customData, String& destinationURL, unsigned short& errorCode, uint32_t& systemCode)
{
    RefPtr<Uint8Array> result;
    DRM_RESULT dr = DRM_SUCCESS;
    DRM_BYTE *pbChallenge = NULL;
    DRM_DWORD cbChallenge = 0;
    DRM_CHAR *pchSilentURL = NULL;
    DRM_DWORD cchSilentURL = 0;

    printf("PlayreadySession generating key request\n");
    // GST_MEMDUMP("init data", initData->data(), initData->byteLength());

    // The current state MUST be KEY_INIT otherwise error out.
    ChkBOOL(m_eKeyState == KEY_INIT, DRM_E_INVALIDARG);

    ChkDR(Drm_Content_SetProperty(m_poAppContext,
                                  DRM_CSP_AUTODETECT_HEADER,
                                  initData->data(),
                                  initData->byteLength()));
    printf("PlayreadySession init data set on DRM context\n");

    // FIXME: Revert once re-use of key is fixed
    Drm_Reader_Bind(m_poAppContext, g_rgpdstrRights, NO_OF(g_rgpdstrRights), _PolicyCallback, NULL, &m_oDecryptContext);
    printf("PlayreadySession DRM reader bound\n");

    // Try to figure out the size of the license acquisition
    // challenge to be returned.
    dr = Drm_LicenseAcq_GenerateChallenge(m_poAppContext,
                                          g_rgpdstrRights,
                                          sizeof(g_rgpdstrRights) / sizeof(DRM_CONST_STRING*),
                                          NULL,
                                          customData.isEmpty() ? NULL: customData.utf8().data(),
                                          customData.isEmpty() ? 0 : customData.length(),
                                          NULL,
                                          &cchSilentURL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &cbChallenge);

    if (dr == DRM_E_BUFFERTOOSMALL) {
        if (cchSilentURL > 0) {
            ChkMem(pchSilentURL = (DRM_CHAR *)Oem_MemAlloc(cchSilentURL + 1));
            ZEROMEM(pchSilentURL, cchSilentURL + 1);
            printf("PlayreadySession allocated silent url size %d\n", cchSilentURL);
        }

        // Allocate buffer that is sufficient to store the license acquisition
        // challenge.
        if (cbChallenge > 0) {
            ChkMem(pbChallenge = (DRM_BYTE *)Oem_MemAlloc(cbChallenge));
            printf("PlayreadySession allocated challenge size %d\n", cbChallenge);
        }

        dr = DRM_SUCCESS;
    } else {
         ChkDR(dr);
         printf("PlayreadySession challenge generated\n");
    }

    // Supply a buffer to receive the license acquisition challenge.
    ChkDR(Drm_LicenseAcq_GenerateChallenge(m_poAppContext,
                                           g_rgpdstrRights,
                                           sizeof(g_rgpdstrRights) / sizeof(DRM_CONST_STRING*),
                                           NULL,
                                           customData.isEmpty() ? NULL: customData.utf8().data(),
                                           customData.isEmpty() ? 0 : customData.length(),
                                           pchSilentURL,
                                           &cchSilentURL,
                                           NULL,
                                           NULL,
                                           pbChallenge,
                                           &cbChallenge));

    printf("PlayreadySession generated license request of size %d\n", cbChallenge);
    // GST_MEMDUMP("generated license request :", pbChallenge, cbChallenge);

    result = Uint8Array::create(pbChallenge, cbChallenge);
    destinationURL = static_cast<const char *>(pchSilentURL);
    printf("PlayreadySession destination URL : %s\n", destinationURL.utf8().data());

    m_eKeyState = KEY_PENDING;
    systemCode = dr;
    errorCode = 0;
    SAFE_OEM_FREE(pbChallenge);
    SAFE_OEM_FREE(pchSilentURL);

    return result;

ErrorExit:
    if (DRM_FAILED(dr)) {
        printf("PlayreadySession ERROR DRM key generation failed\n");
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
        errorCode = WebKitMediaKeyError::MEDIA_KEYERR_CLIENT;
#endif
    }
    return result;
}

//
// Expected synchronisation from caller. This method is not thread-safe!
//
bool PlayreadySession::playreadyProcessKey(Uint8Array* key, unsigned short& errorCode, uint32_t& systemCode)
{
    DRM_RESULT dr = DRM_SUCCESS;
    DRM_LICENSE_RESPONSE oLicenseResponse = {eUnknownProtocol, 0};
    uint8_t *m_pbKeyMessageResponse = key->data();
    uint32_t m_cbKeyMessageResponse = key->byteLength();

    // GST_MEMDUMP("response received :", key->data(), key->byteLength());

    // The current state MUST be KEY_PENDING otherwise error out.
    ChkBOOL(m_eKeyState == KEY_PENDING, DRM_E_INVALIDARG);

    ChkArg(m_pbKeyMessageResponse != NULL && m_cbKeyMessageResponse > 0);

    ChkDR(Drm_LicenseAcq_ProcessResponse(m_poAppContext,
                                         DRM_PROCESS_LIC_RESPONSE_SIGNATURE_NOT_REQUIRED,
                                         NULL,
                                         NULL,
                                         const_cast<DRM_BYTE *>(m_pbKeyMessageResponse),
                                         m_cbKeyMessageResponse,
                                         &oLicenseResponse));

    ChkDR(Drm_Reader_Bind(m_poAppContext,
                          g_rgpdstrRights,
                          NO_OF(g_rgpdstrRights),
                          _PolicyCallback,
                          NULL,
                          &m_oDecryptContext));

    m_key = key->possiblySharedBuffer();
    errorCode = 0;
    m_eKeyState = KEY_READY;
    printf("PlayreadySession key processed, now ready for content decryption\n");
    systemCode = dr;
    return true;

ErrorExit:
    if (DRM_FAILED(dr)) {
        printf("PlayreadySession failed processing license response\n");
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
        errorCode = WebKitMediaKeyError::MEDIA_KEYERR_CLIENT;
#endif
        m_eKeyState = KEY_ERROR;
    }
    return false;
}

int PlayreadySession::processPayload(const void* iv, uint32_t ivSize, void* payloadData, uint32_t payloadDataSize)
{
    DRM_RESULT dr = DRM_SUCCESS;
    /*
    64 bit iv?? must manage it's own counter
    typedef struct
    {
        DRM_UINT64  qwInitializationVector;
        DRM_UINT64  qwBlockOffset;
        DRM_BYTE    bByteOffset;  // Byte offset within the current block
    } DRM_AES_COUNTER_MODE_CONTEXT;
    */

    //G_LOCK (pr_decoder_lock); //TODO:
    ChkDR(Drm_Reader_InitDecrypt(&m_oDecryptContext, NULL, 0));

    if (iv && ivSize) {
        uint8_t* ivData = (uint8_t*) iv;

        // FIXME: IV bytes need to be swapped ???
        // Looks like this has a define now `SWAP_IV_BYTES_IN_DECRYPT` ??
        uint8_t temp;
        for (uint32_t i = 0; i < ivSize / 2; i++) {
            temp = ivData[i];
            ivData[i] = ivData[ivSize - i - 1];
            ivData[ivSize - i - 1] = temp;
        }
        memset(&m_oAESContext, 0, sizeof(DRM_AES_COUNTER_MODE_CONTEXT));

        // TODO: it looks like the `Drm_Reader_Decrypt` handles the iv
        // increment, so we should only memcpy when a sample is skipped?
        MEMCPY(&m_oAESContext.qwInitializationVector, ivData, ivSize);
    }

    ChkDR(Drm_Reader_Decrypt(&m_oDecryptContext, &m_oAESContext, (DRM_BYTE*) payloadData,  payloadDataSize));

    // Call commit during the decryption of the first sample.
    if (!m_fCommit) {
        ChkDR(Drm_Reader_Commit(m_poAppContext, _PolicyCallback, NULL));
        m_fCommit = TRUE;
    }

    //G_UNLOCK (pr_decoder_lock); TODO:

    return 0;

ErrorExit:
    //G_UNLOCK (pr_decoder_lock); // TODO:
    return 1;
}
}

#endif
