#pragma once
#include "Othello.h"
#include "Button.h"
#include <map>

#define INITTREE_DEPTH (10)
#define ENDINGSTART (16)

#define PUBLIC false
#define PRIVATE true
#define COMPILE PUBLIC

#define DIFFICALTY 2
#define VERYEASY 0
#define EASY 1
#define NORMAL 2


class Point {
public:
	BBoard Pos, Turn;
	int Eval;
};


class OthelloAI : public Othello {
protected:
	const int RedrawNum = 800000;
	BBoard Put;
	bool Com2;
	int Ending;
	int BoadEval[64];
	int TreeDepth;
	int NodeNum;
	int Value1, Value2;
	int Kakutei1, Kakutei2;
	int NAValue;
	bool NAVTurn;
	double ThinkTime;
	BBoard KakuteiPos;
	bool FinalSearch;

	std::map<std::pair<BBoard, BBoard>, int> memo;

	uint32_t randomseed;
	uint32_t random(void);
	void srandom(uint32_t num);

	int FontH;
	int FontSize = 32;
	int JosekiDrawCount;

	int ScreenMode = 0;
	int AnimationCount;
	int StrCount;
	IBoard AnimationBoard;
	const int TurnFrame = 2;
	bool IsWait;
	int WaitCount;
	int FaceCount, FaceMax = 8, FaceNum = 16;
	int FaceAnimeCount;
	int Noob = 1;

	const int GAME = 0;
	const int BLACK_PASS = 1;
	const int WHITE_PASS = 2;
	const int JOSEKI_KAKUTEI = 3;
	const int BLACK_WIN = 4;
	const int WHITE_WIN = 5;
	const int BW_DRAW = 6;
	const int TURN = 7;
	const int END_GAME = 8;
	const int ONEMORE_GAME = 9;

	// int Difficality;
	const int Normal = 0;
	const int Hard = 1;
	const int VeryHard = 2;
	const int VeryEasy = 3;
	const int Easy = 4;

 	
	int EndCount;
	bool EndDraw();
	bool EndUpdate();
	bool IsOthelloEnd();
	bool OthelloPut();
	bool TurnAnimetion();
	bool TurnDraw();
	bool StrDrawUpdate();
	int OthelloJudge();

	//íTçı
	int NegaAlpha(BBoard b, BBoard w, int Alp, int Bet, int Depth);
	int NegaAlpha_NS(BBoard b, BBoard w, int Alp, int Bet, int Depth);
	int NegaScout(BBoard b, BBoard w, int Alp, int Bet, int Depth);
	int NegaScout_WO_Sort(BBoard b, BBoard w, int Alp, int Bet, int Depth);

	//1éËÇ≤Ç∆ÇÃï]âøíl
	std::vector<Point> GetPutEval(BBoard b, BBoard w);

	//ï`âÊ
	void ScoreDraw();
	void FaceDraw();
	void BBDraw();
	void SearchingDraw();
	void ReDraw();
	void SetFaceImage(int Difficarity);
	int  CalcAdv();
	void PassDraw();
	void JosekiDraw();
	void BorderFontDraw(int x, int y, int fontsize, int fonthandle, int thick, std::string str, int Color, int ColorThick, int ColorBorder);

	//î’ñ ï]âø
	int PutAbleNum(BBoard b, BBoard w);
	int CountKakutei(BBoard b, BBoard w);
	int CalcPos(BBoard b, BBoard w);
	int CalcValue(BBoard b, BBoard w);
	int CalcKaihoudo(BBoard b, BBoard w);
	int Mount(BBoard b, BBoard w);
	int Wing(BBoard b, BBoard w);

	//ìÆçÏ
	bool ComTurn(bool P);
	int Game();
	bool StrDraw();

	void SetBGM(int num);

	Button *OneMoreButton;
	Button *EndButton;
public:
	OthelloAI();
	OthelloAI(bool P, bool P1 = false);
	OthelloAI(bool P, int Difficality, int Music);
	~OthelloAI();
	void Init(int Difficality);
	void Draw();
	int Update();
};