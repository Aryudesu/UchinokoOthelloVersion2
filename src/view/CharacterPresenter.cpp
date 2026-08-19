#define NOMINMAX
#include "view/CharacterPresenter.h"

#include "manager/ImageManager.h"
#include "util/LoadIni.h"
#include "DxLib.h"
#include <Windows.h>

#include <string_view>


namespace {
    constexpr float FaceX = 16.0f;
    constexpr float FaceY = 220.0f;
    constexpr int MessageLeft = 16;
    constexpr int MessageTop = 400;
    constexpr int MessageRight = 624;
    constexpr int MessageBottom = 468;

    std::string utf8ToLocal(std::string_view text) {
        if (text.empty()) return {};

        const int byteCount = static_cast<int>(text.size());
        const int wideCount = MultiByteToWideChar(
            CP_UTF8, 0, text.data(), byteCount, nullptr, 0
        );
        if (wideCount <= 0) return std::string(text);

        std::wstring wide(static_cast<std::size_t>(wideCount), L'\0');
        MultiByteToWideChar(
            CP_UTF8, 0, text.data(), byteCount, wide.data(), wideCount
        );

        const int localCount = WideCharToMultiByte(
            CP_ACP, 0, wide.data(), wideCount, nullptr, 0, nullptr, nullptr
        );
        if (localCount <= 0) return std::string(text);

        std::string local(static_cast<std::size_t>(localCount), '\0');
        WideCharToMultiByte(
            CP_ACP, 0, wide.data(), wideCount,
            local.data(), localCount, nullptr, nullptr
        );
        return local;
    }

    constexpr const char* DefaultImages[] = {
        "data/img/faceNormal.bmp",
        "data/img/faceEasy.bmp",
        "data/img/faceHard.bmp",
        "data/img/face.bmp",
    };

    constexpr const char* ImageKeys[] = {
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

void CharacterPresenter::Start(const std::string& configPath) {
    INIDat ini(configPath);

    for (std::size_t i = 0; i < imagePaths_.size(); ++i) {
        imagePaths_[i] = ini.GetStr(
            "Images",
            ImageKeys[i],
            DefaultImages[i]
        );
    }
    for (std::size_t i = 0; i < messages_.size(); ++i) {
        messages_[i] = utf8ToLocal(ini.GetStr(
            "Dialogue",
            MessageKeys[i],
            DefaultMessages[i]
        ));
    }

    loaded_ = true;
    Show(CharacterReaction::Start);
}

void CharacterPresenter::End() {
    if (!loaded_) return;
    ImageManager::GetInstance().Destroy(ImageID::MainChara);
    message_.clear();
    loaded_ = false;
}

void CharacterPresenter::Show(CharacterReaction reaction) {
    if (!loaded_) return;

    const Expression nextExpression = expressionFor(reaction);
    loadExpression(nextExpression);
    message_ = messages_[indexOf(reaction)];
}

void CharacterPresenter::Draw() const {
    if (!loaded_) return;

    auto& images = ImageManager::GetInstance();
    images.Draw(FaceX, FaceY, ImageID::MainChara);

    const int panel = GetColor(20, 24, 36);
    const int border = GetColor(170, 180, 200);
    const int text = GetColor(255, 255, 255);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 225);
    DrawBox(
        MessageLeft,
        MessageTop,
        MessageRight,
        MessageBottom,
        panel,
        TRUE
    );
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(
        MessageLeft,
        MessageTop,
        MessageRight,
        MessageBottom,
        border,
        FALSE
    );

    if (!message_.empty()) {
        DrawString(
            MessageLeft + 16,
            MessageTop + 24,
            message_.c_str(),
            text
        );
    }
}

void CharacterPresenter::loadExpression(Expression expression) {
    if (expression == expression_ && loaded_) {
        int width = 0;
        int height = 0;
        ImageManager::GetInstance().Size(
            ImageID::MainChara,
            width,
            height
        );
        if (width > 0 && height > 0) return;
    }

    expression_ = expression;
    ImageManager::GetInstance().Load(
        ImageID::MainChara,
        imagePaths_[indexOf(expression)]
    );
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
    case CharacterReaction::Start:
    case CharacterReaction::Draw:
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
