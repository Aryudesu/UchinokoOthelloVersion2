#pragma once
#include <DxLib.h>
#include <mutex>
#include <fstream>
#include <string>
#include <deque>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

class Logger {
public:
    enum class Level { Trace, Debug, Info, Warn, Error, Fatal };

    static Logger& I() {
        static Logger inst;
        return inst;
    }

    // 初期化：ログファイル名、最大サイズ(バイト)でローテーション
    void init(const std::string& filename = "game.log",
        size_t rotate_bytes = 5 * 1024 * 1024,
        Level min_level = Level::Debug,
        size_t overlay_capacity = 300) {
        std::lock_guard<std::mutex> lk(mu_);
        path_ = filename;
        rotate_bytes_ = rotate_bytes;
        min_level_ = min_level;
        overlay_cap_ = overlay_capacity;
        openFileLocked_();
        overlay_on_ = false;
    }

    void setMinLevel(Level lv) { std::lock_guard<std::mutex> lk(mu_); min_level_ = lv; }
    Level minLevel() const { return min_level_; }

    void log(Level lv, const char* file, int line, const std::string& msg) {
        if (lv < min_level_) return;
        const auto now = std::chrono::system_clock::now();
        const auto tt = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        const auto tm = safeLocaltime_(tt);
        std::ostringstream oss;
        oss << "[" << levelToChar_(lv) << "] "
            << std::put_time(&tm, "%F %T")
            << "." << std::setw(3) << std::setfill('0') << ms.count()
            << " (" << file << ":" << line << ") "
            << msg;

        const std::string lineStr = oss.str();

        {
            std::lock_guard<std::mutex> lk(mu_);
            // ファイル出力
            if (ofs_.is_open()) {
                ofs_ << lineStr << '\n';
                bytes_ += lineStr.size() + 1;
                if (bytes_ >= rotate_bytes_) rotateLocked_();
            }
            // オーバーレイ保存（リング）
            overlay_.push_back({ lv, lineStr });
            if (overlay_.size() > overlay_cap_) overlay_.pop_front();
        }

        // Visual Studio 等へ
#ifdef _WIN32
        OutputDebugStringA((lineStr + "\n").c_str());
#endif
    }

    // 毎フレーム呼ぶ：F1トグル、描画
    void UpdateAndDrawOverlay(int toggleKey = KEY_INPUT_F1) {
        // F1で表示トグル
        if (CheckHitKey(toggleKey) && !prev_toggle_) {
            overlay_on_ = !overlay_on_;
        }
        prev_toggle_ = CheckHitKey(toggleKey);

        if (!overlay_on_) return;

        std::deque<Entry> copy;
        {
            std::lock_guard<std::mutex> lk(mu_);
            copy = overlay_;
        }

        const int bg = GetColor(0, 0, 0);
        const int fg = GetColor(230, 230, 230);
        const int warn = GetColor(255, 220, 120);
        const int err = GetColor(255, 140, 140);
        const int trc = GetColor(160, 200, 255);
        const int dbg = GetColor(180, 255, 180);

        const int x = 12, y = 12, w = 1200, h = 320;
        DrawBoxAA((float)x, (float)y, (float)(x + w), (float)(y + h), bg, TRUE);
        DrawBoxAA((float)x, (float)y, (float)(x + w), (float)(y + h), GetColor(60, 60, 60), FALSE);

        int yy = y + 10;
        const int lineH = 16;
        DrawStringF((float)(x + 8), (float)yy, "LOG (F1: toggle)", GetColor(180, 180, 255));
        yy += 22;

        // 下から新しい順に  ※最新が手前に来る方がデバッグしやすい
        for (int i = (int)copy.size() - 1; i >= 0 && yy + lineH < y + h - 6; --i) {
            const auto& e = copy[i];
            int color = fg;
            switch (e.lv) {
            case Level::Trace: color = trc; break;
            case Level::Debug: color = dbg; break;
            case Level::Warn:  color = warn; break;
            case Level::Error:
            case Level::Fatal: color = err; break;
            default: break;
            }
            // 長すぎる行は切る
            std::string s = e.line.size() > 170 ? (e.line.substr(0, 170) + "...") : e.line;
            DrawStringF((float)(x + 8), (float)yy, s.c_str(), color);
            yy += lineH;
        }
    }

private:
    static std::tm safeLocaltime_(std::time_t tt) {
        std::tm tm{};
        #if defined(_MSC_VER)  // MSVC
                localtime_s(&tm, &tt);
        #elif defined(__unix__) || defined(__APPLE__) // POSIX系
                localtime_r(&tt, &tm);
        #else
                // フォールバック（スレッドセーフではない）
                tm = *std::localtime(&tt);
        #endif
        return tm;
    }


    Logger() = default;
    ~Logger() { std::lock_guard<std::mutex> lk(mu_); if (ofs_.is_open()) ofs_.flush(), ofs_.close(); }

    struct Entry {
        Level lv;
        std::string line;
    };

    std::mutex mu_;
    std::ofstream ofs_;
    std::string path_;
    size_t rotate_bytes_ = 5 * 1024 * 1024;
    size_t bytes_ = 0;
    Level min_level_ = Level::Info;

    std::deque<Entry> overlay_;
    size_t overlay_cap_ = 300;
    bool overlay_on_ = false;
    bool prev_toggle_ = false;

    void openFileLocked_() {
        std::filesystem::create_directories(std::filesystem::path(path_).parent_path());
        ofs_.open(path_, std::ios::out | std::ios::app);
        if (ofs_) {
            ofs_.seekp(0, std::ios::end);
            bytes_ = (size_t)ofs_.tellp();
        }
    }

    void rotateLocked_() {
        ofs_.close();
        // 既存を .1 にローテーション（古い .1 は上書き）
        std::error_code ec;
        std::filesystem::rename(path_, path_ + ".1", ec); // 上書きしたい場合は一旦消す等の処理を追加
        openFileLocked_();
        bytes_ = 0;
    }

    static char levelToChar_(Level lv) {
        switch (lv) {
        case Level::Trace: return 'T';
        case Level::Debug: return 'D';
        case Level::Info: return 'I';
        case Level::Warn: return 'W';
        case Level::Error: return 'E';
        case Level::Fatal: return 'F';
        }
        return '?';
    }
};

// 便利マクロ（ファイル名/行番号を自動付与）
#define LOG_TRACE(msg) Logger::I().log(Logger::Level::Trace, __FILE__, __LINE__, (msg))
#define LOG_DEBUG(msg) Logger::I().log(Logger::Level::Debug, __FILE__, __LINE__, (msg))
#define LOG_INFO(msg)  Logger::I().log(Logger::Level::Info , __FILE__, __LINE__, (msg))
#define LOG_WARN(msg)  Logger::I().log(Logger::Level::Warn , __FILE__, __LINE__, (msg))
#define LOG_ERROR(msg) Logger::I().log(Logger::Level::Error, __FILE__, __LINE__, (msg))
#define LOG_FATAL(msg) Logger::I().log(Logger::Level::Fatal, __FILE__, __LINE__, (msg))

// スコープ時間計測（終了時にms出力）
class LogTimeScope {
public:
    LogTimeScope(const char* name, const char* file, int line)
        : name_(name), file_(file), line_(line), st_(std::chrono::high_resolution_clock::now()) {
    }
    ~LogTimeScope() {
        auto ed = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(ed - st_).count() / 1000.0;
        std::ostringstream oss;
        oss << "[TIME] " << name_ << " : " << ms << " ms";
        Logger::I().log(Logger::Level::Debug, file_, line_, oss.str());
    }
private:
    const char* name_;
    const char* file_;
    int line_;
    std::chrono::high_resolution_clock::time_point st_;
};
#define LOG_TIME_SCOPE(name) LogTimeScope _lts_obj_##__LINE__{(name), __FILE__, __LINE__}
