#include "manager/ImageManager.h"
#include "Dxlib.h"
#include <vector>
#include <string>

//透過色設定
void ImageManager::SetTrans(int R, int G, int B) { SetTransColor(R, G, B); }

//画像読み込み
//ID : オブジェクトID
//sizex,sizey　縦横切り取るピクセルサイズ
//CutX,CutY カット数
//FileName　画像ファイル名
void ImageManager::Load(int ID, int sizex, int sizey, int CutX, int CutY, std::string FileName) {
	imgs_[ID].resize(CutX * CutY);
	LoadDivGraph(FileName.c_str(), CutX * CutY, CutX, CutY, sizex, sizey, &imgs_[ID][0]);
}

//画像読み込み
//ID : オブジェクトID
//FileName　画像ファイル名
void ImageManager::Load(int ID, std::string FileName) {
	imgs_[ID].resize(1);
	imgs_[ID][0] = LoadGraph(FileName.c_str());
}

//画像読み込み
//ID : オブジェクトID
//CutX,CutY カット数
//FileName　画像ファイル名
void ImageManager::LoadSheet(int ID, int CutX, int CutY, std::string FileName) {
	imgs_[ID].resize(CutX * CutY);
	LoadDivGraph(FileName.c_str(), CutX * CutY, CutX, CutY, 32, 32, &imgs_[ID][0]);
}


//画像サイズ
void ImageManager::Size(int ID, int num, int& width, int& height) {
	int x, y;
	GetGraphSize(imgs_[ID][num], &x, &y);
	width = x;
	height = y;
}

//画像サイズ
void ImageManager::Size(int ID, int& width, int& height) {
	int x, y;
	GetGraphSize(imgs_[ID][0], &x, &y);
	width = x;
	height = y;
}


//IDのオブジェクトの画像破棄
void ImageManager::Destroy(int ID) {
	for (int i = 0; i < imgs_[ID].size(); i++) {
		DeleteGraph(imgs_[ID][i]);
	}
}

//画像描画
//x,y　画面位置
//ID,num　オブジェクトIDと描画番号
//TransFlag　透過するかしないか　TRUE/FALSE
void ImageManager::Draw(float x, float y, int ID, int num, int TransFlag, int TurnY) { DrawRotaGraph2(x, y, 0, 0, 1, 0, imgs_[ID][num], TransFlag, FALSE, TurnY); }

//画像描画（0,0に描画）
//ID　オブジェクトID
//TransFlag　透過するかしないか　TRUE/FALSE
void ImageManager::Draw(int ID, int TransFlag) { DrawGraph(0, 0, imgs_[ID][0], TransFlag); }

//画像描画
//x,y　画面位置
//ID,num　オブジェクトIDと描画番号
//TransFlag　透過するかしないか　TRUE/FALSE
void ImageManager::Draw(float x, float y, int ID, int TransFlag) { DrawGraph(x, y, imgs_[ID][0], TransFlag); }


void ImageManager::DeleteAll() {
	InitGraph();
	for (int i = 0; i < imgs_.size(); i++) {
		for (int j = 0; j < imgs_[i].size(); j++) {
			imgs_[i].erase(imgs_[i].begin() + j);
			imgs_[i].clear();
		}
	}
	imgs_.clear();
	imgs_.resize(32);
}