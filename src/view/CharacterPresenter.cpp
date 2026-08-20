#include "view/CharacterPresenter.h"

#include "manager/ImageManager.h"
#include "util/LoadIni.h"
#include "DxLib.h"

#include <algorithm>

namespace {
    constexpr int FacePanelLeft = 4;
    constexpr int FacePanelTop = 196;
    constexpr int FacePanelRight = 204;
    constexpr int FacePanelBottom = 396;
    constexpr int FaceInset = 4;
    constexpr float FaceX =
        static_cast<float>(FacePanelLeft + FaceInset);
    constexpr float FaceY =
        static_cast<float>(FacePanelTop + FaceInset);
    constexpr int FaceFrameSize = 192;
    constexpr int AnimationFrames = 16;
    constexpr int ExpressionRows = 15;
    constexpr int AnimationFrameMs = 120;

    constexpr int MessageLeft = 16;
    constexpr int MessageTop = 400;
    constexpr int MessageRight = 624;
    constexpr int MessageBottom = 468;

    constexpr const char* DifficultyImageKeys[] = {
        "easy",
        "normal",
        "hard",
    };

    constexpr const char* DefaultSheets[] = {
        "data/img/faceEasy.bmp",
        "data/img/faceNormal.bmp",
        "data/img/faceHard.bmp",
    };

    constexpr const char* ExpressionKeys[] = {
        "normal",
        "happy",
        "serious",
        "troubled",
    };

    constexpr const char* MessageKeys[] = {
        "start",
        "thinking",
        "player_move",
        "ai_move",
        "advantage",
        "disadvantage",
        "win",
        "lose",
        "draw",
        "player_pass",
        "ai_pass",
    };

    constexpr const char* DefaultMessages[] = {
        "Let's have a good game!",
        "Hmm... let me think.",
        "That was a good move.",
        "I will play here!",
        "I'm in a good position!",
        "This is getting difficult...",
        "I won! Let's play again.",
        "You win. That was a good game!",
        "A draw! That was close.",
        "You have no legal move. Pass!",
        "I have no legal move. Pass...",
    };
}

void CharacterPresenter::Start(
    const std::string& configPath,
    int difficultyIndex
) {
    INIDat ini(configPath);

    for (std::size_t i = 0; i < expressionRows_.size(); ++i) {
        expressionRows_[i] = std::clamp(
            ini.GetInt(
                "ExpressionRows",
                ExpressionKeys[i],
                expressionRows_[i]
            ),
            0,
            ExpressionRows - 1
        );
    }
    for (std::size_t i = 0; i < messages_.size(); ++i) {
        messages_[i] = ini.GetStr(
            "Dialogue",
            MessageKeys[i],
            DefaultMessages[i]
        );
    }

    const int selected = std::clamp(difficultyIndex, 0, 2);
    const std::string sheetPath = ini.GetStr(
        "Sheets",
        DifficultyImageKeys[selected],
        DefaultSheets[selected]
    );

    auto& images = ImageManager::GetInstance();
    images.SetTrans(255, 255, 255);
    images.LoadDiv(
        ImageID::MainChara,
        FaceFrameSize,
        FaceFrameSize,
        AnimationFrames,
        ExpressionRows,
        sheetPath
    );

    expression_ = Expression::Normal;
    reaction_ = CharacterReaction::Start;
    loaded_ = true;
    Show(CharacterReaction::Start);
}

void CharacterPresenter::End() {
    if (!loaded_) return;
    ImageManager::GetInstance().Destroy(ImageID::MainChara);
    loaded_ = false;
}

void CharacterPresenter::Show(CharacterReaction reaction) {
    if (!loaded_) return;
    reaction_ = reaction;
    expression_ = expressionFor(reaction);
}

void CharacterPresenter::Draw() const {
    if (!loaded_) return;

    const int animationFrame =
        (GetNowCount() / AnimationFrameMs) % AnimationFrames;
    const int expressionRow = expressionRows_[indexOf(expression_)];
    const int imageIndex =
        expressionRow * AnimationFrames + animationFrame;

    auto& images = ImageManager::GetInstance();

    const int frameDark = GetColor(92, 42, 12);
    const int frameGold = GetColor(224, 145, 18);
    const int faceBackground = GetColor(255, 255, 255);

    DrawBox(
        FacePanelLeft,
        FacePanelTop,
        FacePanelRight,
        FacePanelBottom,
        frameDark,
        TRUE
    );
    DrawBox(
        FacePanelLeft + 2,
        FacePanelTop + 2,
        FacePanelRight - 2,
        FacePanelBottom - 2,
        frameGold,
        FALSE
    );
    DrawBox(
        FacePanelLeft + FaceInset,
        FacePanelTop + FaceInset,
        FacePanelRight - FaceInset,
        FacePanelBottom - FaceInset,
        faceBackground,
        TRUE
    );
    DrawBox(
        FacePanelLeft + FaceInset,
        FacePanelTop + FaceInset,
        FacePanelRight - FaceInset,
        FacePanelBottom - FaceInset,
        frameDark,
        FALSE
    );
    images.Draw(FaceX, FaceY, ImageID::MainChara, imageIndex);

    const int panel = GetColor(20, 24, 36);
    const int border = GetColor(170, 180, 200);
    const int text = GetColor(255, 255, 255);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 225);
    DrawBox(
        MessageLeft, MessageTop, MessageRight, MessageBottom,
        panel, TRUE
    );
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(
        MessageLeft, MessageTop, MessageRight, MessageBottom,
        border, FALSE
    );

    const std::string& message = messages_[indexOf(reaction_)];
    if (!message.empty()) {
        DrawString(
            MessageLeft + 16,
            MessageTop + 24,
            message.c_str(),
            text
        );
    }
}

CharacterPresenter::Expression CharacterPresenter::expressionFor(
    CharacterReaction reaction
) noexcept {
    switch (reaction) {
    case CharacterReaction::Thinking:
        return Expression::Serious;
    case CharacterReaction::Advantage:
    case CharacterReaction::Win:
    case CharacterReaction::PlayerPass:
        return Expression::Happy;
    case CharacterReaction::Disadvantage:
    case CharacterReaction::Lose:
    case CharacterReaction::AiPass:
        return Expression::Troubled;
    default:
        return Expression::Normal;
    }
}

std::size_t CharacterPresenter::indexOf(Expression expression) noexcept {
    return static_cast<std::size_t>(expression);
}

std::size_t CharacterPresenter::indexOf(
    CharacterReaction reaction
) noexcept {
    return static_cast<std::size_t>(reaction);
}
