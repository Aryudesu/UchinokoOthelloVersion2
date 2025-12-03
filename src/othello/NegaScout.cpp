#include "OthelloAI.h"
#include "function.h"
#include <stdlib.h>
#include <vector>
#include <algorithm>

//NegaScout法
int OthelloAI::NegaScout(BBoard b, BBoard w, int Alp, int Bet, int Depth) {
	//Depthが0
	NodeNum++;
	if (NodeNum % RedrawNum == 0)ReDraw();
	if (Depth == 0) {
		return FinalSearch ? Noob * (GetMyNum(b) - GetMyNum(w)) * 1000 : CalcValue(b, w);
	}
	//パス
	if (!CanPut(b, w)) {
		//相手もパス
		if (!CanPut(w, b)) {
			//終端ノードは個数の差を返す
			return Noob * (GetMyNum(b) - GetMyNum(w)) * 1000;
		}
		return -NegaScout(w, b, -Bet, -Alp, Depth);
	}
	int Alpha, Beta, t;
	int score_max = -INFTY;
	Alpha = Alp;
	Beta = Bet;
	bool f = false;
	BBoard CanPutList = GetSelectList(b, w);

	for (BBoard m = 0x8000000000000000; m != 0; m >>= 1) {
		if ((CanPutList & m) != 0) {
		BBoard Turn = GetTurnPattern(b, w, m);
		t = -NegaScout(w ^ Turn, b ^ (m | Turn), -Beta, -Alpha, Depth - 1);
		if (t > Alpha && t < Bet && f && Depth > 2)t = -NegaScout(w ^ Turn, b ^ (m | Turn), -Bet, -t, Depth - 1);
		if (t > score_max) {
			if (t >= Bet)return t;
			score_max = t;
			if (t > Alpha)Alpha = t;
		}
		f = true;
		Beta = Alpha + 1;
		}
	}
	return score_max;
}


//ソートなしNegaScout法
int OthelloAI::NegaScout_WO_Sort(BBoard b, BBoard w, int Alp, int Bet, int Depth) {
	//Depthが0
	if (Depth == 0) {
		if (NodeNum++ % RedrawNum == 0)ReDraw();
		return FinalSearch ? (GetMyNum(b) - GetMyNum(w)) * 1000 : CalcValue(b, w);
	}
	//パス
	if (!CanPut(b, w)) {
		//相手もパス
		if (!CanPut(w, b)) {
			if (NodeNum++ % RedrawNum == 0)ReDraw();
			//終端ノードは個数の差を返す
			return (GetMyNum(b) - GetMyNum(w)) * 1000;
		}
		return -NegaScout_WO_Sort(w, b, -Bet, -Alp, Depth);
	}
	int Alpha, Beta, t;
	int score_max = -INFTY;
	Alpha = Alp;
	Beta = Bet;
	bool f = false;
	BBoard CanPutList = GetSelectList(b, w);
	for (BBoard m = 0x8000000000000000; m != 0; m >>= 1) {
		if ((CanPutList & m) != 0) {
			BBoard Turn = GetTurnPattern(b, w, m);
			t = -NegaScout_WO_Sort(w ^ Turn, b ^ (m | Turn), -Beta, -Alpha, Depth - 1);
			if (t > Alpha && t < Bet && f && Depth > 2)t = -NegaScout_WO_Sort(w ^ Turn, b ^ (m | Turn), -Bet, -t, Depth - 1);
			if (t > score_max) {
				if (t >= Bet)return t;
				score_max = t;
				if (t > Alpha)Alpha = t;
			}
			f = true;
			Beta = Alpha + 1;
		}
	}
	return score_max;
}

std::vector<Point> OthelloAI::GetPutEval(BBoard b, BBoard w) {
	BBoard CanPutList = GetSelectList(b, w);
	int PutAbleNum = GetMyNum(CanPutList);
	std::vector<Point> Result(PutAbleNum);
	int count = 0;
	for (BBoard m = 0x8000000000000000; m != 0; m >>= 1) {
		if ((CanPutList & m) != 0) {
			Result[count].Pos = m;
			Result[count].Turn = GetTurnPattern(b, w, m);
			Result[count].Eval = -NegaScout_WO_Sort(w ^ Result[count].Turn, b ^ (m | Result[count].Turn), -INFTY, INFTY, 3);
			count++;
		}
	}
	std::sort(Result.begin(), Result.end(), [](const Point& a, const  Point& b) { return a.Eval > b.Eval; });
	return Result;
}