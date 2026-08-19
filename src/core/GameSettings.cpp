#include "core/GameSettings.h"

#include "util/LoadIni.h"

#include <algorithm>
#include <utility>

namespace {
    constexpr const char* Sections[] = {
        "Easy",
        "Normal",
        "Hard",
    };

    int clamped(
        const INIDat& ini,
        const char* section,
        const char* key,
        int fallback,
        int minimum,
        int maximum
    ) {
        return std::clamp(
            ini.GetInt(section, key, fallback),
            minimum,
            maximum
        );
    }
}

GameSettings& GameSettings::GetInstance() {
    static GameSettings instance;
    return instance;
}

void GameSettings::Load(const std::string& path) {
    profiles_ = { {
        { "EASY", 3, 8, 3'000, 600, 800 },
        { "NORMAL", 5, 14, 10'000, 600, 800 },
        { "HARD", 10, 16, 10'000, 600, 800 },
    } };

    INIDat ini(path);

    for (std::size_t i = 0; i < profiles_.size(); ++i) {
        DifficultyProfile& profile = profiles_[i];
        const char* section = Sections[i];

        profile.searchDepth = clamped(
            ini, section, "depth", profile.searchDepth, 1, 12
        );
        profile.exactEndgameEmpty = clamped(
            ini, section, "exact_endgame_empty",
            profile.exactEndgameEmpty, 0, 20
        );
        profile.timeLimitMs = clamped(
            ini, section, "time_limit_ms",
            profile.timeLimitMs, 100, 60'000
        );
        profile.minimumThinkingMs = clamped(
            ini, section, "minimum_thinking_ms",
            profile.minimumThinkingMs, 0, 10'000
        );
        profile.maximumThinkingMs = clamped(
            ini, section, "maximum_thinking_ms",
            profile.maximumThinkingMs, 0, 10'000
        );
        if (profile.minimumThinkingMs > profile.maximumThinkingMs) {
            std::swap(
                profile.minimumThinkingMs,
                profile.maximumThinkingMs
            );
        }
    }
}

void GameSettings::TogglePlayerOrder() noexcept {
    playerDisc_ = playerDisc_ == Disc::Black
        ? Disc::White
        : Disc::Black;
}

void GameSettings::PreviousDifficulty() noexcept {
    difficultyIndex_ =
        (difficultyIndex_ + static_cast<int>(DifficultyCount) - 1) %
        static_cast<int>(DifficultyCount);
}

void GameSettings::NextDifficulty() noexcept {
    difficultyIndex_ =
        (difficultyIndex_ + 1) %
        static_cast<int>(DifficultyCount);
}
