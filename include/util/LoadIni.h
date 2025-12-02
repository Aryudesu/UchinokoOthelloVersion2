#pragma once
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>

class INIDat {
    std::vector<std::string> Section;
    std::vector<std::vector<std::vector<std::string>>> SDList;

public:
    INIDat();
    INIDat(std::string FileName);
    ~INIDat();

    void DataInput(std::string FileName);
    void DataDelete();
    int  GetSecNum(std::string Sec);
    bool CheckSec(std::string Sec);
    bool CheckElem(std::string Sec, std::string Elem);
    std::vector<std::string> GetData(std::string Sec, std::string Elem);

    std::string GetStr(const std::string& sec, const std::string& key,
        const std::string& def = "") const;
    int         GetInt(const std::string& sec, const std::string& key,
        int def = 0) const;
    float       GetFloat(const std::string& sec, const std::string& key,
        float def = 0.0f) const;
    bool        GetBool(const std::string& sec, const std::string& key,
        bool def = false) const;

    std::vector<std::string> GetStrList(const std::string& sec, const std::string& key) const;
    std::vector<int>         GetIntList(const std::string& sec, const std::string& key) const;
    std::vector<float>       GetFloatList(const std::string& sec, const std::string& key) const;

    std::optional<std::vector<std::string>>
        TryGet(const std::string& sec, const std::string& key) const;

private:
    static std::string trim_copy(const std::string& s);
    static std::vector<std::string> split_eq(const std::string& s);  // '=' ‚Å1‰ñ
    static std::vector<std::string> split_csv(const std::string& s); // ',' ‚Å‘S•ªŠ„
    static bool is_array_rhs(const std::string& s); // ‰E•Ó‚É','ŠÜ‚Ş‚©‚Å”»’è
    static bool iequals(const std::string& a, const std::string& b); // ‘å¬–³‹”äŠr
};
