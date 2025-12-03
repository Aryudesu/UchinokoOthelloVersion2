#include "othello/OthelloAI.h"
#include "template/SoundManager.h"
#include "template/ImageManager.h"
#include "DxLib.h"
#include <random>
#include <chrono>

//コンストラクタ．変数の初期設定を行っている
OthelloAI::OthelloAI() {
	Player = BLACK;
	Turn = BLACK;
	End = false;
	Ending = ENDINGSTART;
	PutCount = 0;
	KakuteiPos = 0;
	ThinkTime = 0;
	AnimationCount = 0;
	StrCount = 0;
	EndCount = 0;
	Init(0);
}

//コンストラクタ．変数の初期設定を行っている．
OthelloAI::OthelloAI(bool P, bool P2) {
	Player = P;		//プレイヤーの黒と白
	Com2 = P2;		//コンピュータの黒と白
	Turn = BLACK;	//現在どっちのターンか
	End = false;	//終わっているかどうか
	Ending = ENDINGSTART;	//残り何手で終盤扱いにするか
	PutCount = 0;			//置いた手数
	KakuteiPos = 0;			//確定石の場所
	ThinkTime = 0;			//思考？時間？
	AnimationCount = 0;
	StrCount = 0;
	EndCount = 0;
	Init(0);					//その他初期設定
}

//コンストラクタ．変数の初期設定を行っている．
OthelloAI::OthelloAI(bool P, int Difficality, int Music) {
	Player = P;		//プレイヤーの黒と白
	Turn = BLACK;	//現在どっちのターンか
	End = false;	//終わっているかどうか
	Ending = ENDINGSTART;	//残り何手で終盤扱いにするか
	PutCount = 0;			//置いた手数
	KakuteiPos = 0;			//確定石の場所
	ThinkTime = 0;			//思考？時間？
	AnimationCount = 0;

	StrCount = 0;
	EndCount = 0;
	SetBGM(Music);
	SoundManager::GetInstance().PlayBGM(BGM1);
	//探索深度
	if (Difficality == 0)TreeDepth = 1;
	if (Difficality == 1)TreeDepth = 5;
	if (Difficality == 2)TreeDepth = 9;
	if (Difficality == 3) {
		TreeDepth = 9;
		Noob = -1;
	}
	Init(Difficality);					//その他初期設定
}

void OthelloAI::SetBGM(int num) {
	if (num == 0)SoundManager::GetInstance().SetBGM(BGM1, 12850, "dat/sound/SomethingDoing.wav");
	if (num == 1)SoundManager::GetInstance().SetBGM(BGM1, 23780, "dat/sound/OriginalRags.wav");
	if (num == 2)SoundManager::GetInstance().SetBGM(BGM1, 24887, "dat/sound/PineappleRag.wav");
	if (num == 3)SoundManager::GetInstance().SetBGM(BGM1,  9644, "dat/sound/PeacherineRag.wav");
	if (num == 4)SoundManager::GetInstance().SetBGM(BGM1, 14541, "dat/sound/EliteSyncopations.wav");
	if (num == 5)SoundManager::GetInstance().SetBGM(BGM1, 12659, "dat/sound/WallStreetRag.wav");
	SoundManager::GetInstance().ChangeBGMVolume(0);

}

OthelloAI::~OthelloAI() {
	SoundManager::GetInstance().StopBGM(BGM1);
	SoundManager::GetInstance().DeleteBGM(BGM1);
	ImageManager::GetInstance().DeleteAll();
}

void OthelloAI::Init(int Difficality) {
	srandom((unsigned)time(NULL));
	FinalSearch = false;
	IsWait = false;
	//デバッグ用．探索した評価値の結果．もともとNegaAlpha法を使っていた名残．
	NAValue = 0;
	FaceAnimeCount = 0;
	WaitCount = 0;
	//評価用盤面
	int tmp[8][8] = {
		{200,-32,  0, -1, -1,  0,-32,200},
		{-32,-45, -3, -3, -3, -3,-45,-32},
		{  0, -3,  0, -1, -1,  0, -3,  0},
		{ -1, -3, -1, -1, -1, -1, -3, -1},
		{ -1, -3, -1, -1, -1, -1, -3, -1},
		{  0, -3,  0, -1, -1,  0, -3,  0},
		{-32,-45, -3, -3, -3, -3,-45,-32},
		{200,-32,  0, -1, -1,  0,-32,200}
	};
	//評価用盤面はBoadEvalにおさめておく
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++)BoadEval[x + y * 8] = tmp[x][y];
	}
	SetFaceImage(Difficality);
	FontH = LoadFontDataToHandle("dat/font/07にくまるフォント32.dft", 0);
	OneMoreButton = new Button(
		BOADULX - FRAMESIZE - 8,
		BOADULY + BOADSIZEY * 8 + FRAMESIZE * 4,
		BOADSIZEX * 4 + FRAMESIZE + 8,
		32 + 16,
		{ "もう1回" },
		FontH,
		FontSize);
	EndButton = new Button(
		BOADULX + BOADSIZEX * 4,
		BOADULY + BOADSIZEY * 8 + FRAMESIZE * 4,
		BOADSIZEX * 4 + FRAMESIZE + 8,
		32 + 16,
		{ "おわる" },
		FontH,
		FontSize);
}

//有利不利判断
int OthelloAI::CalcAdv() {
	int Result;
	// 有利
	if ((!Player == NAVTurn && NAValue > 0) || (!Player == !NAVTurn && NAValue < 0)) {
		Result = 1;
	}
	// 五分
	else if (NAValue == 0) {
		Result = 0;
	}
	// 不利
	else {
		Result = 2;
	}
	return Result;
}

//探索中の表情アニメーション
void OthelloAI::SearchingDraw() {
	int Adv = CalcAdv();
	int Confirm = (PutCount > 60 - Ending + 1) ? 6 : 0;
	if (!IsWait) {
		FaceAnimeCount++;
	} else {
		WaitCount++;
		if (WaitCount >= 8) {
			FaceAnimeCount++;
			WaitCount = 0;
		}
	}
	if(FaceAnimeCount >= FaceNum)FaceAnimeCount = 0;
	ImageManager::GetInstance().DrawImg(9 * BOADSIZEX + BOADULX + FRAMESIZE, 0 * BOADSIZEY + BOADULY, FACE, (Confirm + Adv * 2 + 1) * FaceNum + FaceAnimeCount, FALSE);
}

//探索中に描画する
void OthelloAI::ReDraw() {
	Fps::GetInstance().Update();
	if (ProcessMessage() != 0)exit(0);		//ウィンドウの×ボタンが押されたら終わり
	ClearDrawScreen();						// 画面を消す
	BBDraw();
	ScoreDraw();
	StrDraw();
	SearchingDraw();
	ScreenFlip();							//裏画面処理を表画面に反映
	Fps::GetInstance().Wait();
}

//表情画像セット
void OthelloAI::SetFaceImage(int Difficality) {
	if (Difficality == 0)ImageManager::GetInstance().LoadImg(FACE, 192, 192, FaceNum, 15, "dat/Image/faceEasy.bmp");
	else if (Difficality == 1)ImageManager::GetInstance().LoadImg(FACE, 192, 192, FaceNum, 15, "dat/Image/faceNormal.bmp");
	else ImageManager::GetInstance().LoadImg(FACE,192, 192, FaceNum, 15, "dat/Image/faceHard.bmp");
}

//ビットボードで表示
void OthelloAI::BBDraw() {
	ImageManager::GetInstance().DrawImg(BACK, FALSE);
	BBoard GSL, Kaku;
	GSL = GetSelectList(Turn ? Black : White, Turn ? White : Black);
	int count = 0;
	for (BBoard m = 0x8000000000000000; m != 0; m >>= 1) {
		int x = count % 8, y = count >> 3;
		//置けるマーク
		if ((GSL & m) != 0) {
			ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 3 + 0, FALSE);
		} else {
			if ((Black & m) != 0) {
				if ((NewPut & m) != 0)ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 3 + 1, FALSE);
				else ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 0 + 1, FALSE);
			}
			else if ((White & m) != 0) {
				if ((NewPut & m) != 0)ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 3 + 2, FALSE);
				else ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 0 + 2, FALSE);
			}
			else {
				ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 0 + 0, FALSE);
			}
		}
		count++;
	}
}

//スコア描画
void OthelloAI::ScoreDraw() {
	int ScoreSize = 40 * 5;
	int SCOREL = 9 * BOADSIZEX + BOADULX + FRAMESIZE - 4;
	int SCORET = 6 * BOADSIZEY + BOADULY;
	int BNUM = GetMyNum(Black), WNUM = GetMyNum(White);
	ImageManager::GetInstance().DrawImg(SCOREL + BOADSIZEX * 0, SCORET, BOARD, 4 * 3 + BNUM / 10, FALSE);
	ImageManager::GetInstance().DrawImg(SCOREL + BOADSIZEX * 1, SCORET, BOARD, 4 * 3 + BNUM % 10, FALSE);
	ImageManager::GetInstance().DrawImg(SCOREL + BOADSIZEX * 2, SCORET, BOARD, 4 * 3 + 10, FALSE);
	ImageManager::GetInstance().DrawImg(SCOREL + BOADSIZEX * 3, SCORET, BOARD, 4 * 3 + WNUM / 10, FALSE);
	ImageManager::GetInstance().DrawImg(SCOREL + BOADSIZEX * 4, SCORET, BOARD, 4 * 3 + WNUM % 10, FALSE);
}

//顔グラ描画
void OthelloAI::FaceDraw() {
	//顔グラ用フレーム
	int FACESIZE = 192;
	int FACEL = 9 * BOADSIZEX + BOADULX + FRAMESIZE;
	int FACET = 0 * BOADSIZEY + BOADULY;
	int Adv = CalcAdv();//盤面状況取得
	//顔グラ
	FaceCount++;
	int Confirm = (PutCount > 60 - Ending + 1) ? 6 : 0;
	if (FaceCount >= FaceNum * FaceMax)FaceCount = 0;
	if (IsOthelloEnd()) {
		if (ScreenMode == BW_DRAW)
			ImageManager::GetInstance().DrawImg(FACEL, FACET, FACE, 12 * FaceNum + FaceCount / FaceMax, FALSE);
		if ((!Player && ScreenMode == BLACK_WIN) || (Player && ScreenMode == WHITE_WIN))
			ImageManager::GetInstance().DrawImg(FACEL, FACET, FACE, 13 * FaceNum + FaceCount / FaceMax, FALSE);
		if ((Player && ScreenMode == BLACK_WIN) || (!Player && ScreenMode == WHITE_WIN))
			ImageManager::GetInstance().DrawImg(FACEL, FACET, FACE, 14 * FaceNum + FaceCount / FaceMax, FALSE);
	}
	else {
		if (PutCount > 60 - Ending + 1) {
			ImageManager::GetInstance().DrawImg(FACEL, FACET, FACE, (Confirm + Adv * 2) * FaceNum + FaceCount / FaceMax, FALSE);
		}
		else {
			ImageManager::GetInstance().DrawImg(FACEL, FACET, FACE, (Confirm + Adv * 2) * FaceNum + FaceCount / FaceMax, FALSE);
		}
	}
}

//(x,y)を中心とした縁付フォントで描画
void OthelloAI::BorderFontDraw(int x,int y,int fontsize,int fonthandle, int thick, std::string str,int Color,int ColorThick,int ColorBorder) {
	int width = GetDrawStringWidthToHandle(str.c_str(), str.size(), fonthandle);
	int x_ = x - width / 2;
	int y_ = y - fontsize / 2;
	for (int i = -thick - 1; i <= thick + 1; i++) {
		for (int j = -thick - 1; j <= thick + 1; j++) {
			DrawStringToHandle(x_ + i, y_ + j, str.c_str(), ColorBorder, fonthandle);
		}
	}
	for (int i = -thick; i <= thick; i++) {
		for (int j = -thick; j <= thick; j++) {
			DrawStringToHandle(x_ + i, y_ + j, str.c_str(), ColorThick, fonthandle);
		}
	}
	DrawStringToHandle(x_, y_, str.c_str(), Color, fonthandle);
}

//BLACK PASS等の描画
void OthelloAI::PassDraw() {
	//もしパスが発生した場合
	int Thick = 3;
	int x = BOADULX + BOADSIZEX * 4;
	int y = BOADULY + BOADSIZEY * 4;
	std::string str = ScreenMode == BLACK_PASS ? "Black Pass" : "White Pass";
	BorderFontDraw(x, y, FontSize, FontH, Thick, str, GetColor(255, 0, 0), GetColor(255, 255, 255), GetColor(0, 0, 0));
}

//定石を完遂すると定石名表示
void OthelloAI::JosekiDraw() {
	int JosekiNum = Jo->GetJosekiNum();
	if (JosekiNum != -1) {
		int Thick = 3;
		int x = BOADULX + BOADSIZEX * 4;
		int y = BOADULY + BOADSIZEY * 4;
		BorderFontDraw(x, y, FontSize, FontH, Thick, Jo->GetJosekiName(JosekiNum).c_str(), GetColor(30,144,255), GetColor(255, 255, 255), GetColor(0, 0, 0));
	}
}

//描画する
void OthelloAI::Draw() {
	//盤面描画
	if (ScreenMode == TURN)TurnDraw();
	else BBDraw();
	FaceDraw();
	ScoreDraw();
	StrDraw();
	if (IsOthelloEnd())EndDraw();
}

void OthelloAI::srandom(uint32_t num) {
	randomseed = num;
}

uint32_t OthelloAI::random() {
	randomseed = randomseed ^ (randomseed << 13); randomseed = randomseed ^ (randomseed >> 17);
	return randomseed = randomseed ^ (randomseed << 5);
}

//コンピュータの番
bool OthelloAI::ComTurn(bool P){
	//時間計測
	std::chrono::system_clock::time_point start, end;
	start = std::chrono::system_clock::now();
	//探索した手数
	NodeNum = 0;
	if (Jo->GetFlag()) {
		//定石通り打つ
		Put = Jo->GetRandJoseki(PutCount);
	} else {
		//NegaScout法で探索．
		BBoard b = P ? Black : White, w = P ? White : Black;
		int maxscore = -INFTY;
		int value;
		BBoard CanPutList = GetSelectList(b, w);
		for (BBoard m = 0x8000000000000000; m != 0; m >>= 1) {
			if ((CanPutList & m) != 0) {
				BBoard Turn = GetTurnPattern(b, w, m);
				value = -NegaScout(w ^ Turn, b ^ (m | Turn), -INFTY, INFTY, TreeDepth - 1);
				if (maxscore <= value) {
					maxscore = value;
					Put = m;
				}
				else if (maxscore == value) {
					if (random() % 2 == 1)Put = m;
				}
			}
		}
		NAValue = maxscore;
		NAVTurn = P;
	}
	//時間計測
	end = std::chrono::system_clock::now();
	ThinkTime = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.);
	//ちょっと待つ
	if (ThinkTime < 300) {
		IsWait = true;
		WaitCount = 0;
		for (int i = 0; i <= 60; i++)ReDraw();
		IsWait = false;
	}
	//置いた手数カウント
	PutCount++;
	//置く
	if (P == BLACK)PlayBlack(Put);
	else PlayWhite(Put);
	//現在の手から可能性のある定石を探索
	Jo->SearchJoseki(Put, PutCount);
	//新しく置いた箇所はメモ
	NewPut = Put;
	ChangeTurn();
	return true;
}

//文字描画
bool OthelloAI::StrDraw() {
	if(ScreenMode == BLACK_PASS || ScreenMode == WHITE_PASS)PassDraw();
	if(ScreenMode == JOSEKI_KAKUTEI)JosekiDraw();
}

//文字描画アップデート
bool OthelloAI::StrDrawUpdate() {
	StrCount++;
	//文字が出てる間は待機
	if (StrCount > 50) {
		StrCount = 0;
		return true;
	}
	return false;
}

//オセロで石を置く動作
bool OthelloAI::OthelloPut() {
	bool result = false;
	//コンピュータのターンか，コンピュータ同士を戦わせている場合
	if (Turn != Player || Com2)result = ComTurn(Turn);
	else result = PlayPerson(Turn);
	//終盤まで行った場合，全て探索する
	if (PutCount >= 60 - Ending) {
		FinalSearch = true;
		if (60 - PutCount > 15) {
			TreeDepth = 60 - PutCount - 1;
		}
		else {
			TreeDepth = 60 - PutCount;
		}
	}
	return result;
}

//1手終わるごとに判定を行う
int OthelloAI::OthelloJudge() {
	////定石を完遂すると定石名表示
	if (Jo->GetJosekiNum() != -1)return JOSEKI_KAKUTEI;
	//パス
	if (!CanPut(Turn ? Black : White, Turn ? White : Black)) {
		//相手もパスなら終了
		if (!CanPut(Turn ? White : Black, Turn ? Black : White)) {
			//個数取得
			int BNum, WNum;
			GetNum(BNum, WNum);
			//どっちが勝ったかを返す
			if (BNum > WNum)return BLACK_WIN;
			if (BNum < WNum)return WHITE_WIN;
			return BW_DRAW;
		}
		ChangeTurn();
		//次の手番が黒なら白がパスしたということ
		return Turn ? WHITE_PASS : BLACK_PASS;
	}
	return GAME;
}

//ひっくり返す動作描画
bool OthelloAI::TurnDraw() {
	int FrameSize = FRAMESIZE;
	ImageManager::GetInstance().DrawImg(BACK, FALSE);
	BBoard GSL, Kaku;
	GSL = GetSelectList(Turn ? Black : White, Turn ? White : Black);
	int count = 0;
	for (BBoard m = 0x8000000000000000; m != 0; m >>= 1) {
		int x = count % 8, y = count >> 3;
		if ((Black & m) != 0) {
			if ((NewPut & m) != 0)ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 3 + 1, FALSE);
			else {
				if((TurnBoardMemo & m) != 0)ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 2 + (AnimationCount - 1) / TurnFrame, FALSE);
				else ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 0 + 1, FALSE);
			}
		}
		else if ((White & m) != 0) {
			if ((NewPut & m) != 0)ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 3 + 2, FALSE);
			else {
				if ((TurnBoardMemo & m) != 0)ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 1 + (AnimationCount - 1) / TurnFrame, FALSE);
				else ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 0 + 2, FALSE);
			}
		}
		else {
			ImageManager::GetInstance().DrawImg(x * BOADSIZEX + BOADULX, y * BOADSIZEY + BOADULY, BOARD, 3 * 0 + 0, FALSE);
		}
		count++;
	}
	return true;
}

//ひっくり返す
bool OthelloAI::TurnAnimetion() {
	AnimationCount++;
	if (AnimationCount > TurnFrame * 3) {
		AnimationCount = 0;
		return true;
	}
return false;
}

//終了したかどうか
bool OthelloAI::IsOthelloEnd() {
	return ScreenMode == BLACK_WIN || ScreenMode == WHITE_WIN || ScreenMode == BW_DRAW;
}

//終了時の描画
bool OthelloAI::EndDraw() {
	int Thick = 3;
	int x = BOADULX + BOADSIZEX * 4;
	int y = BOADULY + BOADSIZEY * 4;
	std::string str = ScreenMode == BLACK_WIN ? "Black Win" : ScreenMode == WHITE_WIN ? "White Win" : "DRAW";
	BorderFontDraw(x, y, FontSize, FontH, Thick, str, GetColor(255, 0, 0), GetColor(255, 255, 255), GetColor(0, 0, 0));
	OneMoreButton->Draw();
	EndButton->Draw();
}

//終了時の処理
bool OthelloAI::EndUpdate() {
	EndCount++;
	if (EndCount == 1) {
		SoundManager::GetInstance().StopBGM(BGM1);
		SoundManager::GetInstance().DeleteBGM(BGM1);
		if ((Player && ScreenMode == BLACK_WIN) || (!Player && ScreenMode == WHITE_WIN))SoundManager::GetInstance().SetBGM(BGM1, 25108, "dat/sound/TheEasyWinners.wav");
		if ((Player && ScreenMode == WHITE_WIN) || (!Player && ScreenMode == BLACK_WIN))SoundManager::GetInstance().SetBGM(BGM1, 23480, "dat/sound/Solace.wav");
		if (ScreenMode == BW_DRAW)SoundManager::GetInstance().SetBGM(BGM1, 11464, "dat/sound/FigLeafRag.wav");
		SoundManager::GetInstance().ChangeBGMVolume(0);
		SoundManager::GetInstance().PlayBGM(BGM1);
	}
	if (EndCount > 60) {
		if (OneMoreButton->Update())ScreenMode = ONEMORE_GAME;
		if (EndButton->Update())ScreenMode = END_GAME;
	}
	return ScreenMode;
}

//ゲーム本体
int OthelloAI::Game() {
	//ゲームモードの時
	if (ScreenMode == GAME) {
		//石を置く
		if (OthelloPut())ScreenMode = TURN;
	}
	else if (ScreenMode == TURN) {
		//石を置く
		if (TurnAnimetion())ScreenMode = OthelloJudge();
	}
	else if (IsOthelloEnd()) {
		if (EndUpdate()) {
			return ScreenMode;
		}
	} else {
		if (StrDrawUpdate())ScreenMode = GAME;
	}
	return ScreenMode;
}

//画面更新
int OthelloAI::Update() {
	//終わったかどうかを返す
	return Game();
}