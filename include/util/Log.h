#pragma once
#include <string>
#include <vector>
#include <deque>
#include <mutex>

class Log {
public:
    enum class Level {
        Debug,
        Info,
        Warn,
        Error
    };

    struct Entry {
        Level level;
        std::string text;
    };

    // シングルトンインスタンス取得
    static Log& I();

    // 初期化（起動時に1回呼ぶ）
    // filename: 出力ファイルパス
    // minLevel: このレベル以上だけ出力
    // also_stdout: 標準出力にも出すかどうか
    void init(const std::string& filename = "",
        Level minLevel = Level::Debug,
        bool also_stdout = true,
        std::size_t overlay_capacity = 300);

    void setMinLevel(Level lv);

    // ログ出力本体（マクロ経由で使う想定）
    void write(Level lv, const char* file, int line, const std::string& msg);

    // オーバーレイ描画用に、最新ログのスナップショットを取得
    std::vector<Entry> latestEntries() const;

private:
    Log() = default;
    ~Log();

    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    mutable std::mutex mu_;
    std::string path_;
    bool stdout_ = true;
    Level min_level_ = Level::Info;

    std::deque<Entry> ring_;       // オーバーレイ用リングバッファ
    std::size_t ring_cap_ = 300;

    void writeUnlocked_(Level lv, const std::string& line);

    static const char* levelToString_(Level lv);
};

// 便利マクロ
#define LOG_DEBUG(msg) ::Log::I().write(::Log::Level::Debug, __FILE__, __LINE__, (msg))
#define LOG_INFO(msg)  ::Log::I().write(::Log::Level::Info , __FILE__, __LINE__, (msg))
#define LOG_WARN(msg)  ::Log::I().write(::Log::Level::Warn , __FILE__, __LINE__, (msg))
#define LOG_ERROR(msg) ::Log::I().write(::Log::Level::Error, __FILE__, __LINE__, (msg))
