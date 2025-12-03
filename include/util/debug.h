#pragma once
#include "util/Log.h"
#include <iostream>

// デバッグメッセージ出力マクロ
#ifndef NDEBUG
#define DBG_MSG(msg) (std::clog << "[DBG] " << msg << '\n')
#else
#define DBG_MSG(msg) ((void)0)
#endif
