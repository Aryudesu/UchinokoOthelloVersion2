#pragma once

#include "core/Ids.h"

#include <array>
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
};

class CharacterPresenter {
public:
    void Start(const std::string& configPath);
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

    std::array<std::string, ExpressionCount> imagePaths_{};
    std::array<std::string, 11> messages_{};
    Expression expression_ = Expression::Normal;
    std::string message_;
    bool loaded_ = false;

    void loadExpression(Expression expression);
    [[nodiscard]] static Expression expressionFor(
        CharacterReaction reaction
    ) noexcept;
    [[nodiscard]] static std::size_t indexOf(Expression expression) noexcept;
    [[nodiscard]] static std::size_t indexOf(
        CharacterReaction reaction
    ) noexcept;
};
