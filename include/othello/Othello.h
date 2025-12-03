#pragma once
#include "Joseki.h"
#include <vector>
typedef unsigned long long BBoard;
typedef std::vector<std::vector<int>> IBoard;
#define BLACK (true)
#define WHITE (false)

#define FRAMESIZE 8
#define BOADULX 40
#define BOADULY 40
#define BOADSIZEX 40
#define BOADSIZEY 40

#define STR_INFO_X (BOADULX + 8 * BOADSIZEX + 2 * FRAMESIZE + 32)

#define INFTY (10000000)

class Othello {
protected:
	BBoard Black, White;
	bool Player, Turn;
	bool End;
	int PutCount;
	BBoard NewPut;
	BBoard TurnBoardMemo;
	int SE1, SE2;

	//メインシステム
	void Init();
	bool IsBoardFull();
	void TurnBoard(BBoard& b, BBoard& w, BBoard put, BBoard re);
	BBoard GetTurnPattern(BBoard b, BBoard w, BBoard m);
	bool CanPut(BBoard b, BBoard w);
	int CountNum(bool turn);
	int GetMyNum(BBoard b);
	void PlayBlack(BBoard p);
	void PlayWhite(BBoard p);
	void ChangeTurn();
	BBoard GetKakutei(BBoard b, BBoard w);
	IBoard IBInit();
	IBoard BB2IB(BBoard b, BBoard w);
	IBoard Pos2IB(BBoard s);
	
	//音関連設定
	void SetSE();
	void SetSE(int num);
	void SetImage();

	//描画関係
	void BoardDraw();
	BBoard GetSelectList(BBoard b, BBoard w);

	//UI関係
	bool PlayPerson(bool P);
	BBoard InputXY();
	BBoard XY2BB(int x, int y);
	bool BB2XY(BBoard B, int& x, int& y);
	bool MouseLeft();

	//定石表示できたら面白そう
	Joseki* Jo;
public:
	~Othello();
	Othello();
	Othello(bool P);
	void Draw();
	void Update();
	void GetNum(int &b, int &w);
};