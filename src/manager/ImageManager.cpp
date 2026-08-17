#include "manager/ImageManager.h"
#include "DxLib.h"
#include <cassert>

namespace {
    constexpr int INVALID_HANDLE = -1;
}

ImageManager::ImageManager() {
    imgs_.resize(static_cast<int>(ImageID::Count));
}

ImageManager::~ImageManager() {
    DeleteAll();
}

void ImageManager::safeDelete(Handle& h) {
    if (h != INVALID_HANDLE) {
        DeleteGraph(h);
        h = INVALID_HANDLE;
    }
}

std::vector<ImageManager::Handle>& ImageManager::getSlot(ImageID id) {
    auto idx = static_cast<int>(id);
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(imgs_.size())) {
        imgs_.resize(idx + 1);
    }
    return imgs_[idx];
}

const std::vector<ImageManager::Handle>& ImageManager::getSlot(ImageID id) const {
    auto idx = static_cast<int>(id);
    assert(idx >= 0 && idx < static_cast<int>(imgs_.size()));
    return imgs_[idx];
}

void ImageManager::SetTrans(int r, int g, int b) {
    SetTransColor(r, g, b);
}

void ImageManager::Load(ImageID id, const std::string& filename) {
    auto& slot = getSlot(id);
    for (auto& h : slot) safeDelete(h);
    slot.clear();

    slot.resize(1, INVALID_HANDLE);
    slot[0] = LoadGraph(filename.c_str());
}

void ImageManager::LoadDiv(ImageID id, int sizex, int sizey,
    int cutX, int cutY,
    const std::string& filename) {
    auto& slot = getSlot(id);
    for (auto& h : slot) safeDelete(h);
    slot.clear();

    const int num = cutX * cutY;
    slot.resize(num, INVALID_HANDLE);
    LoadDivGraph(filename.c_str(), num, cutX, cutY, sizex, sizey, slot.data());
}

void ImageManager::LoadSheet(ImageID id, int cutX, int cutY, const std::string& filename) {
    LoadDiv(id, 32, 32, cutX, cutY, filename);
}

void ImageManager::Size(ImageID id, int num, int& width, int& height) const {
    const auto& slot = getSlot(id);
    assert(num >= 0 && num < static_cast<int>(slot.size()));
    int w = 0, h = 0;
    GetGraphSize(slot[num], &w, &h);
    width = w;
    height = h;
}

void ImageManager::Size(ImageID id, int& width, int& height) const {
    Size(id, 0, width, height);
}

void ImageManager::Draw(float x, float y,
    ImageID id, int num,
    bool transFlag, bool turnY) const {
    const auto& slot = getSlot(id);
    if (num < 0 || num >= static_cast<int>(slot.size())) return;
    int h = slot[num];
    if (h == INVALID_HANDLE) return;

    DrawGraphF(x, y, h, transFlag ? TRUE : FALSE);
    (void)turnY;
}

void ImageManager::Destroy(ImageID id) {
    auto& slot = getSlot(id);
    for (auto& h : slot) {
        safeDelete(h);
    }
    slot.clear();
}

void ImageManager::DeleteAll() {
    for (auto& slot : imgs_) {
        for (auto& h : slot) {
            safeDelete(h);
        }
        slot.clear();
    }
}
