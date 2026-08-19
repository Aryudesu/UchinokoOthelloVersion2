#pragma once

#include "model/BitBoard.h"

#include <array>
#include <cstddef>
#include <string>

struct DifficultyProfile {
    std::string name;
    int searchDepth = 5;
    int exactEndgameEmpty = 14;
    int timeLimitMs = 10'000;
    int minimumThinkingMs = 600;
    int maximumThinkingMs = 800;
};

class GameSettings {
public:
    static constexpr std::size_t DifficultyCount = 3;

    static GameSettings& GetInstance();

    void Load(const std::string& path);

    [[nodiscard]] Disc playerDisc() const noexcept { return playerDisc_; }
    [[nodiscard]] Disc aiDisc() const noexcept {
        return playerDisc_ == Disc::Black ? Disc::White : Disc::Black;
    }
    [[nodiscard]] int difficultyIndex() const noexcept {
        return difficultyIndex_;
    }
    [[nodiscard]] const DifficultyProfile& difficulty() const noexcept {
        return profiles_[static_cast<std::size_t>(difficultyIndex_)];
    }

    void TogglePlayerOrder() noexcept;
    void PreviousDifficulty() noexcept;
    void NextDifficulty() noexcept;

private:
    GameSettings() = default;

    std::array<DifficultyProfile, DifficultyCount> profiles_{ {
        { "EASY", 3, 8, 3'000, 600, 800 },
        { "NORMAL", 5, 14, 10'000, 600, 800 },
        { "HARD", 10, 16, 10'000, 600, 800 },
    } };
    Disc playerDisc_ = Disc::Black;
    int difficultyIndex_ = 1;
};
