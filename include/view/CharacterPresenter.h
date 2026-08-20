#pragma once

#include "core/Ids.h"

#include <array>
#include <cstddef>
#include <string>

enum class CharacterReaction {
    Start,
    Thinking,
    PlayerMove,
    AiMove,
    Advantage,
    Disadvantage,
    Win,
    Lose,
    Draw,
    PlayerPass,
    AiPass,
    Count,
};

class CharacterPresenter {
public:
    void Start(const std::string& configPath, int difficultyIndex);
    void End();

    void Show(CharacterReaction reaction);
    void Draw() const;

private:
    enum class Expression {
        Normal,
        Happy,
        Serious,
        Troubled,
        Count,
    };

    static constexpr std::size_t ExpressionCount =
        static_cast<std::size_t>(Expression::Count);
    static constexpr std::size_t ReactionCount =
        static_cast<std::size_t>(CharacterReaction::Count);

    std::array<int, ExpressionCount> expressionRows_{ { 0, 9, 5, 10 } };
    std::array<std::string, ReactionCount> messages_{};
    Expression expression_ = Expression::Normal;
    CharacterReaction reaction_ = CharacterReaction::Start;
    bool loaded_ = false;

    [[nodiscard]] static Expression expressionFor(
        CharacterReaction reaction
    ) noexcept;
    [[nodiscard]] static std::size_t indexOf(Expression expression) noexcept;
    [[nodiscard]] static std::size_t indexOf(
        CharacterReaction reaction
    ) noexcept;
};
