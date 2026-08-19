#pragma once
#include <type_traits>

enum class ImageID : int {
    MainChara = 0,
    Enemy,
    Item,
    Stone,
    Mark,
    Board,
    Frame,
    Count,
};

enum class SoundID : int {
    BGM1 = 1
};

enum class SceneID : int {
    Title = 0,
    Game = 1,
    Result = 2,
};

template <class Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

