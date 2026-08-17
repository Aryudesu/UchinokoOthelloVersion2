#pragma once

struct AnimationClip {
    int firstFrame = 0;
    int frameCount = 1;
    int frameDurationMs = 1;
    bool loop = false;
};

class SpriteAnimator {
public:
    void Play(const AnimationClip& clip, int nowMs) noexcept {
        if (clip.frameCount <= 0 || clip.frameDurationMs <= 0) {
            Reset();
            return;
        }

        clip_ = clip;
        startedAtMs_ = nowMs;
        currentFrame_ = clip.firstFrame;
        playing_ = true;
        finished_ = false;
    }

    void Update(int nowMs) noexcept {
        if (!playing_) return;

        int elapsedMs = nowMs - startedAtMs_;
        if (elapsedMs < 0) elapsedMs = 0;

        const int frameOffset = elapsedMs / clip_.frameDurationMs;

        if (clip_.loop) {
            currentFrame_ =
                clip_.firstFrame + frameOffset % clip_.frameCount;
            return;
        }

        if (frameOffset >= clip_.frameCount) {
            currentFrame_ =
                clip_.firstFrame + clip_.frameCount - 1;
            playing_ = false;
            finished_ = true;
            return;
        }

        currentFrame_ = clip_.firstFrame + frameOffset;
    }

    void Reset() noexcept {
        clip_ = {};
        startedAtMs_ = 0;
        currentFrame_ = 0;
        playing_ = false;
        finished_ = false;
    }

    [[nodiscard]] int CurrentFrame() const noexcept {
        return currentFrame_;
    }

    [[nodiscard]] bool IsPlaying() const noexcept {
        return playing_;
    }

    [[nodiscard]] bool IsFinished() const noexcept {
        return finished_;
    }

private:
    AnimationClip clip_{};
    int startedAtMs_ = 0;
    int currentFrame_ = 0;
    bool playing_ = false;
    bool finished_ = false;
};
