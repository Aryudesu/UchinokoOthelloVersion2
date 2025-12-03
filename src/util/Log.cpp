#include "util/Log.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
    std::ofstream g_log_ofs;
}

Log& Log::I() {
    static Log inst;
    return inst;
}

void Log::init(const std::string& filename,
    Level minLevel,
    bool also_stdout,
    std::size_t overlay_capacity) {
    std::lock_guard<std::mutex> lk(mu_);

    min_level_ = minLevel;
    stdout_ = also_stdout;
    ring_cap_ = overlay_capacity;

    // --- ファイル名を決定 ---
    if (filename.empty()) {
        // 日付ベースでログファイルを自動生成
        using namespace std::chrono;
        auto now = system_clock::now();
        auto tt = system_clock::to_time_t(now);

        std::tm tm{};
#if defined(_MSC_VER)
        localtime_s(&tm, &tt);
#else
        tm = *std::localtime(&tt);
#endif

        std::ostringstream oss;
        oss << "logs/log_"
            << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S")
            << ".txt";

        path_ = oss.str();
    }
    else {
        path_ = filename;
    }

    // --- フォルダを作成してファイルを開く ---
    std::filesystem::create_directories(
        std::filesystem::path(path_).parent_path()
    );

    if (g_log_ofs.is_open()) {
        g_log_ofs.close();
    }
    g_log_ofs.open(path_, std::ios::out | std::ios::app);
}

Log::~Log() {
    std::lock_guard<std::mutex> lk(mu_);
    if (g_log_ofs.is_open()) {
        g_log_ofs.flush();
        g_log_ofs.close();
    }
}

void Log::setMinLevel(Level lv) {
    std::lock_guard<std::mutex> lk(mu_);
    min_level_ = lv;
}

void Log::write(Level lv, const char* file, int line, const std::string& msg) {
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (lv < min_level_) return;
    }

    // 時刻文字列作成
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto tt = system_clock::to_time_t(now);
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm{};
#if defined(_MSC_VER)
    localtime_s(&tm, &tt);
#else
    tm = *std::localtime(&tt);
#endif

    std::ostringstream oss;
    oss << "[" << levelToString_(lv) << "] "
        << std::put_time(&tm, "%F %T")
        << "." << std::setw(3) << std::setfill('0') << ms.count()
        << " (" << file << ":" << line << ") "
        << msg;

    std::string lineStr = oss.str();

    std::lock_guard<std::mutex> lk(mu_);
    writeUnlocked_(lv, lineStr);
}

void Log::writeUnlocked_(Level lv, const std::string& line) {
    // ファイル
    if (g_log_ofs.is_open()) {
        g_log_ofs << line << '\n';
    }

    // 標準出力
    if (stdout_) {
        std::cout << line << std::endl;
    }

#ifdef _WIN32
    // VS の Output 窓
    OutputDebugStringA((line + "\n").c_str());
#endif

    // リングバッファ（オーバーレイ用）
    ring_.push_back({ lv, line });
    if (ring_.size() > ring_cap_) {
        ring_.pop_front();
    }
}

std::vector<Log::Entry> Log::latestEntries() const {
    std::lock_guard<std::mutex> lk(mu_);
    return std::vector<Entry>(ring_.begin(), ring_.end());
}

const char* Log::levelToString_(Level lv) {
    switch (lv) {
    case Level::Debug: return "DEBUG";
    case Level::Info:  return "INFO ";
    case Level::Warn:  return "WARN ";
    case Level::Error: return "ERROR";
    }
    return "UNKWN";
}
