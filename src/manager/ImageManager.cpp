#include "manager/ImageManager.h"

#include "DxLib.h"

#include <cassert>
#include <utility>
#include <vector>

namespace {
    constexpr int InvalidHandle = -1;
    constexpr int DefaultSheetFrameSize = 32;
}

ImageManager::~ImageManager() {
    DeleteAll();
}

std::size_t ImageManager::indexOf(ImageID id) noexcept {
    const auto index = static_cast<std::size_t>(id);
    assert(index < SlotCount);
    return index;
}

ImageManager::Slot& ImageManager::getSlot(ImageID id) noexcept {
    return images_[indexOf(id)];
}

const ImageManager::Slot& ImageManager::getSlot(ImageID id) const noexcept {
    return images_[indexOf(id)];
}

void ImageManager::clearSlot(Slot& slot) noexcept {
    for (auto& image : slot) {
        if (image.handle != InvalidHandle) {
            DeleteGraph(image.handle);
        }
    }
    slot.clear();
}

void ImageManager::SetTrans(int r, int g, int b) {
    SetTransColor(r, g, b);
}

void ImageManager::Load(ImageID id, const std::string& filename) {
    auto& slot = getSlot(id);
    clearSlot(slot);

    const Handle handle = LoadGraph(filename.c_str());
    if (handle == InvalidHandle) return;

    int width = 0;
    int height = 0;
    if (GetGraphSize(handle, &width, &height) == -1) {
        DeleteGraph(handle);
        return;
    }

    slot.push_back({ handle, width, height });
}

void ImageManager::LoadDiv(
    ImageID id,
    int sizeX,
    int sizeY,
    int cutX,
    int cutY,
    const std::string& filename
) {
    auto& slot = getSlot(id);
    clearSlot(slot);

    if (sizeX <= 0 || sizeY <= 0 || cutX <= 0 || cutY <= 0) return;

    const int count = cutX * cutY;
    std::vector<Handle> handles(
        static_cast<std::size_t>(count),
        InvalidHandle
    );

    if (
        LoadDivGraph(
            filename.c_str(),
            count,
            cutX,
            cutY,
            sizeX,
            sizeY,
            handles.data()
        ) == -1
    ) {
        for (Handle handle : handles) {
            if (handle != InvalidHandle) DeleteGraph(handle);
        }
        return;
    }

    slot.reserve(static_cast<std::size_t>(count));
    for (Handle handle : handles) {
        slot.push_back({ handle, sizeX, sizeY });
    }
}

void ImageManager::LoadSheet(
    ImageID id,
    int cutX,
    int cutY,
    const std::string& filename
) {
    LoadDiv(
        id,
        DefaultSheetFrameSize,
        DefaultSheetFrameSize,
        cutX,
        cutY,
        filename
    );
}

void ImageManager::Size(
    ImageID id,
    int num,
    int& width,
    int& height
) const {
    width = 0;
    height = 0;

    const auto& slot = getSlot(id);
    if (num < 0 || static_cast<std::size_t>(num) >= slot.size()) return;

    const auto& image = slot[static_cast<std::size_t>(num)];
    width = image.width;
    height = image.height;
}

void ImageManager::Size(ImageID id, int& width, int& height) const {
    Size(id, 0, width, height);
}

void ImageManager::Draw(
    float x,
    float y,
    ImageID id,
    int num,
    bool transFlag,
    bool turnY
) const {
    const auto& slot = getSlot(id);
    if (num < 0 || static_cast<std::size_t>(num) >= slot.size()) return;

    const auto& image = slot[static_cast<std::size_t>(num)];
    if (image.handle == InvalidHandle) return;

    const int useTransparency = transFlag ? TRUE : FALSE;
    if (turnY) {
        DrawExtendGraphF(
            x,
            y + static_cast<float>(image.height),
            x + static_cast<float>(image.width),
            y,
            image.handle,
            useTransparency
        );
        return;
    }

    DrawGraphF(x, y, image.handle, useTransparency);
}

void ImageManager::Destroy(ImageID id) {
    clearSlot(getSlot(id));
}

void ImageManager::DeleteAll() {
    for (auto& slot : images_) {
        clearSlot(slot);
    }
}
