#pragma once
#ifndef NDEBUG
#define DBG_MSG(msg) (std::clog << "[DBG] " << msg << '\n')
#else
#define DBG_MSG(msg) ((void)0)
#endif
