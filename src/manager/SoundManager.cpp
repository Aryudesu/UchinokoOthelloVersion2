#include "manager/SoundManager.h"
#include "core/Ids.h"
#include <cassert>

SoundManager::SoundManager() {
    // 初期状態は全部 INVALID_HANDLE にしておく
    mSE.resize(30, INVALID_HANDLE);
    // BGMは SoundID の最大値に応じてリサイズしてもいいし、都度ensureでもOK
    mBGM.resize(4, INVALID_HANDLE);
    SetSEVolume(mSEVolume);
    SetBGMVolume(mBGMVolume);
}

SoundManager::~SoundManager() {
    // 全SEを破棄
    for (auto& h : mSE) safeDelete(h);
    // 全BGMを破棄
    for (auto& h : mBGM) safeDelete(h);
    // SoftSound も破棄
    if (mSoftSoundHandle != INVALID_HANDLE) {
        DeleteSoftSound(mSoftSoundHandle);
        mSoftSoundHandle = INVALID_HANDLE;
    }
}

void SoundManager::safeDelete(Handle& h) {
    if (h != INVALID_HANDLE) {
        DeleteSoundMem(h);
        h = INVALID_HANDLE;
    }
}

void SoundManager::ensureSESize(int index) {
    if (index < 0) return;
    if (index >= (int)mSE.size()) {
        mSE.resize(index + 1, INVALID_HANDLE);
    }
}

void SoundManager::ensureBGMSize(SoundID id) {
    int idx = static_cast<int>(id);
    if (idx < 0) return;
    if (idx >= (int)mBGM.size()) {
        mBGM.resize(idx + 1, INVALID_HANDLE);
    }
}

// ---------- SE ----------

void SoundManager::SetSE(int index, const std::string& file) {
    ensureSESize(index);
    safeDelete(mSE[index]);
    mSE[index] = LoadSoundMem(file.c_str());
    if (mSE[index] != INVALID_HANDLE) {
        ChangeVolumeSoundMem(255 * mSEVolume / 100, mSE[index]);
    }
}

void SoundManager::PlaySE(int index) {
    if (index < 0 || index >= (int)mSE.size()) return;
    int h = mSE[index];
    if (h == INVALID_HANDLE) return;
    PlaySoundMem(h, DX_PLAYTYPE_BACK);
}

// ---------- BGM ----------

void SoundManager::SetBGM(SoundID id, int loopSamplePos, const std::string& file) {
    ensureBGMSize(id);
    int idx = static_cast<int>(id);

    safeDelete(mBGM[idx]);
    mBGM[idx] = LoadSoundMem(file.c_str());
    if (mBGM[idx] != INVALID_HANDLE) {
        SetLoopPosSoundMem(loopSamplePos, mBGM[idx]);
        ChangeVolumeSoundMem(255 * mBGMVolume / 100, mBGM[idx]);
    }
}

void SoundManager::PlayBGM(SoundID id) {
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= (int)mBGM.size()) return;
    int h = mBGM[idx];
    if (h == INVALID_HANDLE) return;

    ChangeVolumeSoundMem(255 * mBGMVolume / 100, h);
    PlaySoundMem(h, DX_PLAYTYPE_LOOP);
}

void SoundManager::StopBGM(SoundID id) {
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= (int)mBGM.size()) return;
    int h = mBGM[idx];
    if (h == INVALID_HANDLE) return;

    StopSoundMem(h);
}

void SoundManager::DeleteBGM(SoundID id) {
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= (int)mBGM.size()) return;
    safeDelete(mBGM[idx]);
}

// ---------- SoftSound ベースのBGM ----------

void SoundManager::SetSSBGM(SoundID id, int loopSamplePos, const std::string& file) {
    ensureBGMSize(id);
    int idx = static_cast<int>(id);

    // 既存 BGM・SoftSound を安全に破棄
    safeDelete(mBGM[idx]);
    if (mSoftSoundHandle != INVALID_HANDLE) {
        DeleteSoftSound(mSoftSoundHandle);
        mSoftSoundHandle = INVALID_HANDLE;
    }

    mSoftSoundHandle = LoadSoftSound(file.c_str());
    if (mSoftSoundHandle == INVALID_HANDLE) return;

    mBGM[idx] = LoadSoundMemFromSoftSound(mSoftSoundHandle);
    if (mBGM[idx] != INVALID_HANDLE) {
        SetLoopPosSoundMem(loopSamplePos, mBGM[idx]);
        ChangeVolumeSoundMem(255 * mBGMVolume / 100, mBGM[idx]);
    }
}

void SoundManager::PlaySSBGM(SoundID id) {
    PlayBGM(id); // 中身が SoftSound 由来でもハンドルは mBGM に入ってるので同じでOK
}

void SoundManager::StopSSBGM(SoundID id) {
    StopBGM(id);
}

void SoundManager::DeleteSSBGM(SoundID id) {
    DeleteBGM(id);
    if (mSoftSoundHandle != INVALID_HANDLE) {
        DeleteSoftSound(mSoftSoundHandle);
        mSoftSoundHandle = INVALID_HANDLE;
    }
}

// ---------- Volume ----------

void SoundManager::SetSEVolume(int v) {
    if (v < 0) v = 0;
    else if (v > 100) v = 100;
    mSEVolume = v;

    for (auto h : mSE) {
        if (h != INVALID_HANDLE) {
            ChangeVolumeSoundMem(255 * mSEVolume / 100, h);
        }
    }
}

void SoundManager::SetBGMVolume(int v) {
    if (v < 0) v = 0;
    else if (v > 100) v = 100;
    mBGMVolume = v;

    for (auto h : mBGM) {
        if (h != INVALID_HANDLE) {
            ChangeVolumeSoundMem(255 * mBGMVolume / 100, h);
        }
    }
}
