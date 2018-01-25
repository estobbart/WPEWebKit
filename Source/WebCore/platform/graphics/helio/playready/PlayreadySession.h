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

#include <pthread.h>
#include <queue>

namespace WebCore {

class PlayreadySession {

public:

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

    enum TaskPriority {
        KEY = 1,
        DECRYPT = 2
    };

    // TODO: Need to split this up.. a decode task needs a specific key id?
    // TODO: when a MediaPlayer tears down it needs to be able to destroy a key?

    // TODO: These tasks need to know if decryption is being done or not
    // maybe pass in a state flag or something?

    // Expect delays the first time this is called, all follow up calls should
    // occur faster once the Drm_Initialize happens.
    static void playreadyTask(TaskPriority priority, std::function< void (PlayreadySession*) > task);

    // The functions below will lock/unlock the playready session
    // They can be called in a task handler callback
    // The taskQueue has it's own mutex so it's safe to request additional
    // tasks within a task.

    // TODO: MAy need to keep a map of keys, and let the decryption tasks
    // specify which key they need, to be able to do license rotation

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

    int processPayload(const void* iv, uint32_t ivSize, void* payloadData, uint32_t payloadDataSize);

    // Helper for PlayreadySession clients.
    const String& sessionId() { return m_sessionId; }

    void resetSession();

    // TODO: hide this..
    PlayreadySession();

protected:
    RefPtr<ArrayBuffer> m_key;
    void _drmSessionInit();
    bool _nextTask();

private:
    ~PlayreadySession();

    // static std::unique_ptr<PlayreadySession> prSession;
    static PlayreadySession *gPlayreadySession;

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

    String m_sessionId;

    pthread_t pr_thread_id;
    static std::queue<std::function<void(PlayreadySession *)>> prTaskQueue;
    static std::queue<std::function<void(PlayreadySession *)>> prDecryptQueue;
    static pthread_cond_t queueCond;
    static pthread_mutex_t queueMutex;
};

}
#endif

#endif
