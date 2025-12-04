#pragma once
#include <vector>
#include <string>
#include "DxLib.h"
#include "core/Ids.h"  // SoundID

class SoundManager {
public:
    static SoundManager& GetInstance() {
        static SoundManager inst;
        return inst;
    }

    // --- SE ---
    void SetSE(int index, const std::string& file);
    void PlaySE(int index);

    // --- BGM ---
    void SetBGM(SoundID id, int loopSamplePos, const std::string& file);
    void PlayBGM(SoundID id);
    void StopBGM(SoundID id);
    void DeleteBGM(SoundID id);

    // SoftSound を使った BGM (必要なら)
    void SetSSBGM(SoundID id, int loopSamplePos, const std::string& file);
    void PlaySSBGM(SoundID id);
    void StopSSBGM(SoundID id);
    void DeleteSSBGM(SoundID id);

    // 音量（0～100）
    void SetSEVolume(int v);
    void SetBGMVolume(int v);
    int  GetSEVolume()  const { return mSEVolume; }
    int  GetBGMVolume() const { return mBGMVolume; }

    // （デバッグ用UIは分離推奨）

private:
    SoundManager();
    ~SoundManager();

    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    using Handle = int;
    static constexpr Handle INVALID_HANDLE = -1;

    std::vector<Handle> mSE;   // SE用
    std::vector<Handle> mBGM;  // BGM用 (SoundID を index として使う)

    int mSEVolume = 50;
    int mBGMVolume = 60;

    int mSoftSoundHandle = INVALID_HANDLE;

    void safeDelete(Handle& h);
    void ensureSESize(int index);
    void ensureBGMSize(SoundID id);
};
