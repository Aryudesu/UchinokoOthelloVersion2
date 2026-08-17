#pragma once

#include "core/Ids.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

class ImageManager {
public:
    static ImageManager& GetInstance() {
        static ImageManager inst;
        return inst;
    }

    void SetTrans(int r, int g, int b);

    void Load(ImageID id, const std::string& filename);
    void LoadDiv(
        ImageID id,
        int sizeX,
        int sizeY,
        int cutX,
        int cutY,
        const std::string& filename
    );
    void LoadSheet(
        ImageID id,
        int cutX,
        int cutY,
        const std::string& filename
    );

    void Size(ImageID id, int num, int& width, int& height) const;
    void Size(ImageID id, int& width, int& height) const;

    void Draw(
        float x,
        float y,
        ImageID id,
        int num = 0,
        bool transFlag = true,
        bool turnY = false
    ) const;

    void Draw(ImageID id, bool transFlag = true) const {
        Draw(0.0f, 0.0f, id, 0, transFlag);
    }

    void Destroy(ImageID id);
    void DeleteAll();

private:
    using Handle = int;

    struct Image {
        Handle handle = -1;
        int width = 0;
        int height = 0;
    };

    static constexpr std::size_t SlotCount =
        static_cast<std::size_t>(ImageID::Count);

    using Slot = std::vector<Image>;

    ImageManager() = default;
    ~ImageManager();

    ImageManager(const ImageManager&) = delete;
    ImageManager& operator=(const ImageManager&) = delete;

    std::array<Slot, SlotCount> images_{};

    [[nodiscard]] static std::size_t indexOf(ImageID id) noexcept;
    [[nodiscard]] Slot& getSlot(ImageID id) noexcept;
    [[nodiscard]] const Slot& getSlot(ImageID id) const noexcept;

    static void clearSlot(Slot& slot) noexcept;
};
