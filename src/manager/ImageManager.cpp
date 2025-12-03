#include "manager/ImageManager.h"
#include "DxLib.h"
#include <cassert>

namespace {
    constexpr int INVALID_HANDLE = -1;
}

ImageManager::ImageManager() {
    // enum の数が確定しているなら reserve しておくのもアリ
    imgs_.resize(static_cast<int>(ImageID::Item) + 1); // 仮：最後のIDに合わせる
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

// 透過色設定
void ImageManager::SetTrans(int r, int g, int b) {
    SetTransColor(r, g, b);
}

// 単一画像読み込み
void ImageManager::Load(ImageID id, const std::string& filename) {
    auto& slot = getSlot(id);
    // 既存があれば削除
    for (auto& h : slot) safeDelete(h);
    slot.clear();

    slot.resize(1, INVALID_HANDLE);
    slot[0] = LoadGraph(filename.c_str());
}

// 分割画像読み込み
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

// シート読み込み（32x32固定）
void ImageManager::LoadSheet(ImageID id, int cutX, int cutY, const std::string& filename) {
    LoadDiv(id, 32, 32, cutX, cutY, filename);
}

// サイズ取得（num指定）
void ImageManager::Size(ImageID id, int num, int& width, int& height) const {
    const auto& slot = getSlot(id);
    assert(num >= 0 && num < static_cast<int>(slot.size()));
    int w = 0, h = 0;
    GetGraphSize(slot[num], &w, &h);
    width = w;
    height = h;
}

// 最初の画像のサイズ
void ImageManager::Size(ImageID id, int& width, int& height) const {
    Size(id, 0, width, height);
}

// 描画
void ImageManager::Draw(float x, float y,
    ImageID id, int num,
    bool transFlag, bool turnY) const {
    const auto& slot = getSlot(id);
    if (num < 0 || num >= static_cast<int>(slot.size())) return; // 安全側
    int h = slot[num];
    if (h == INVALID_HANDLE) return;

    // 必要なら Rota じゃなく普通の DrawGraph でもOK（用途で選んで）
    DrawGraphF(x, y, h, transFlag?TRUE:FALSE);
    // もし上下反転や回転を使いたいなら DrawRotaGraph2F などに差し替え
}

// IDのオブジェクトの画像破棄
void ImageManager::Destroy(ImageID id) {
    auto& slot = getSlot(id);
    for (auto& h : slot) {
        safeDelete(h);
    }
    slot.clear();
}

// 全部破棄
void ImageManager::DeleteAll() {
    for (auto& slot : imgs_) {
        for (auto& h : slot) {
            safeDelete(h);
        }
        slot.clear();
    }
    // 必要ならサイズを維持してもいいし、クリアしてもいい
    // imgs_.clear();
}
