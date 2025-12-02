#pragma once
#include <type_traits>

enum class ImageID : int {
    MainChara = 0,
    Enemy = 1,
    Item = 2,
};

enum class SoundID : int {
    BGM1 = 1
};

enum class SceneID : int {
    Title = 0,
    Game = 1,
    Result = 2,
    Quit = 3,
};

constexpr int to_i(ImageID id) noexcept { return static_cast<int>(id); }

constexpr int to_i(SoundID id) noexcept { return static_cast<int>(id); }

