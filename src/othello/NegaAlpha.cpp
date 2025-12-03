#include "OthelloAI.h"

//NegaAlpha法（αβ法をスッキリさせたものだけど結局使ってない）
int OthelloAI::NegaAlpha(BBoard b, BBoard w, int Alp, int Bet, int Depth) {
	int Alpha = Alp, Beta = Bet;
	if (Depth == 0) {
		NodeNum++;
		if (NodeNum % 50000 == 0)ReDraw();
		return CalcValue(b, w);
	}
	if (!CanPut(b, w)) {
		if (!CanPut(w, b)) {
			NodeNum++;
			if (NodeNum % 50000 == 0)ReDraw();
			return (GetMyNum(w) != 0) ? (GetMyNum(b) - GetMyNum(w)) * 1000 : INFTY;
		}
		return -NegaAlpha(w, b, -Beta, -Alpha, Depth);
	}
	int Value = -INFTY;
	BBoard BestPut = 0;
	for (BBoard m = 0x8000000000000000; m != 0; m >>= 1) {
		BBoard tp = 0;
		if ((tp = GetTurnPattern(b, w, m)) != 0) {
			int ChildValue = -NegaAlpha(w ^ tp, b ^ (m | tp), -Beta, -Alpha, Depth - 1);
			if (ChildValue > Value || (ChildValue == Value && rand() % 100 > 50)) {
				Value = ChildValue;
				Alpha = Value;
				BestPut = m;
			}
			if (Value >= Beta)break;
		}
	}
	if (TreeDepth == Depth)Put = BestPut;
	return Alpha;
}

//NegaAlpha法（αβ法をスッキリさせたものだけど結局使ってない）
int OthelloAI::NegaAlpha_NS(BBoard b, BBoard w, int Alp, int Bet, int Depth) {
	int Alpha = Alp, Beta = Bet;
	if (Depth == 0) {return CalcValue(b, w);}
	if (!CanPut(b, w)) {
		if (!CanPut(w, b)) {
			return (GetMyNum(w) != 0) ? (GetMyNum(b) - GetMyNum(w)) * 1000 : INFTY;
		}
		return -NegaAlpha_NS(w, b, -Beta, -Alpha, Depth);
	}
	for (BBoard m = 0x8000000000000000; m != 0; m >>= 1) {
		BBoard tp = 0;
		if ((tp = GetTurnPattern(b, w, m)) != 0) {
			int ChildValue = -NegaAlpha_NS(w ^ tp, b ^ (m | tp), -Beta, -Alpha, Depth - 1);
			if (ChildValue > Alpha)Alpha = ChildValue;
			if (Alpha >= Beta)return Alpha;
		}
	}
	return Alpha;
}