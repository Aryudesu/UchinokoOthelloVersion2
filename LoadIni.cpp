#include "LoadIni.h"
#include "DxLib.h"
#include <cctype>
#include <cstdlib>
#include <sstream>

std::string INIDat::trim_copy(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace((unsigned char)s[b])) ++b;
    while (e > b && std::isspace((unsigned char)s[e - 1])) --e;
    return s.substr(b, e - b);
}
std::vector<std::string> INIDat::split_eq(const std::string& s) {
    auto pos = s.find('=');
    if (pos == std::string::npos) return { trim_copy(s) };
    return { trim_copy(s.substr(0,pos)),
             trim_copy(s.substr(pos + 1)) };
}
std::vector<std::string> INIDat::split_csv(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) out.push_back(trim_copy(item));
    return out;
}
bool INIDat::is_array_rhs(const std::string& s) {
    return s.find(',') != std::string::npos;
}
bool INIDat::iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    }
    return true;
}

INIDat::INIDat() {}
INIDat::INIDat(std::string FileName) { DataInput(FileName); }
INIDat::~INIDat() { DataDelete(); }

void INIDat::DataInput(std::string FileName) {
    int fp = FileRead_open(FileName.c_str());
    if (fp == 0) {
        // é∏îséûèàóù
        return;
    }

    std::vector<std::vector<std::string>> tmpList;
    bool FirstSect = false;

    char buf[1024];

    while (FileRead_eof(fp) == 0) {
        if (FileRead_gets(buf, (int)sizeof(buf), fp) == -1) break;
        std::string str = trim_copy(buf);
        if (str.empty()) continue;
        if (str[0] == '#') continue;

        auto cpos = str.find_first_of("#;");
        if (cpos != std::string::npos) str = trim_copy(str.substr(0, cpos));
        if (str.empty()) continue;

        if (str[0] == '>') {
            str.erase(str.begin());
            Section.push_back(trim_copy(str));
            if (FirstSect) {
                SDList.push_back(tmpList);
                tmpList.clear();
            }
            FirstSect = true;
            continue;
        }

        auto kv = split_eq(str);
        if (kv.size() <= 1) continue;
        std::vector<std::string> row;
        row.push_back(kv[0]); // ç∂ÅÅÉLÅ[
        if (is_array_rhs(kv[1])) {
            auto items = split_csv(kv[1]);
            row.insert(row.end(), items.begin(), items.end());
        }
        else {
            row.push_back(kv[1]);
        }
        tmpList.push_back(std::move(row));
    }
    SDList.push_back(tmpList);
    FileRead_close(fp);
}

void INIDat::DataDelete() {
    for (auto& sec : SDList) {
        for (auto& row : sec) {
            row.clear(); row.shrink_to_fit();
        }
        sec.clear(); sec.shrink_to_fit();
    }
    SDList.clear(); SDList.shrink_to_fit();
    Section.clear(); Section.shrink_to_fit();
}

int INIDat::GetSecNum(std::string Sec) {
    for (int i = 0; i < (int)Section.size(); i++) {
        if (Section[i] == Sec) return i;
    }
    return -1;
}

bool INIDat::CheckSec(std::string Sec) {
    return GetSecNum(Sec) >= 0;
}

bool INIDat::CheckElem(std::string Sec, std::string Elem) {
    int num = GetSecNum(Sec);
    if (num < 0) return false;
    for (int i = 0; i < (int)SDList[num].size(); i++) {
        if (SDList[num][i][0] == Elem) return true;
    }
    return false;
}

std::vector<std::string> INIDat::GetData(std::string Sec, std::string Elem) {
    int num = GetSecNum(Sec);
    if (num < 0) return {};
    for (int i = 0; i < (int)SDList[num].size(); i++) {
        if (SDList[num][i][0] == Elem) {
            std::vector<std::string> result;
            for (int j = 1; j < (int)SDList[num][i].size(); j++) {
                result.push_back(SDList[num][i][j]);
            }
            return result;
        }
    }
    return {};
}

//void INIDat::ShowAllData() {
//    for (int i = 0; i < (int)Section.size(); i++) {
//        Message(Section[i].c_str());
//        for (int j = 0; j < (int)SDList[i].size(); j++) {
//            for (int k = 0; k < (int)SDList[i][j].size(); k++) Message(SDList[i][j][k].c_str());
//        }
//    }
//}

std::optional<std::vector<std::string>>
INIDat::TryGet(const std::string& sec, const std::string& key) const {
    int idx = -1;
    for (int i = 0; i < (int)Section.size(); ++i) if (Section[i] == sec) { idx = i; break; }
    if (idx < 0) return std::nullopt;
    for (auto& row : SDList[idx]) {
        if (!row.empty() && row[0] == key) {
            if (row.size() <= 1) return std::vector<std::string>{};
            return std::vector<std::string>(row.begin() + 1, row.end());
        }
    }
    return std::nullopt;
}

std::string INIDat::GetStr(const std::string& sec, const std::string& key, const std::string& def) const {
    auto v = TryGet(sec, key);
    if (!v || v->empty()) return def;
    return (*v)[0];
}
int INIDat::GetInt(const std::string& sec, const std::string& key, int def) const {
    auto v = TryGet(sec, key);
    if (!v || v->empty()) return def;
    try { return std::stoi((*v)[0]); }
    catch (...) { return def; }
}
float INIDat::GetFloat(const std::string& sec, const std::string& key, float def) const {
    auto v = TryGet(sec, key);
    if (!v || v->empty()) return def;
    try { return std::stof((*v)[0]); }
    catch (...) { return def; }
}
bool INIDat::GetBool(const std::string& sec, const std::string& key, bool def) const {
    auto v = TryGet(sec, key);
    if (!v || v->empty()) return def;
    const auto s = (*v)[0];
    if (iequals(s, "true") || s == "1" || iequals(s, "on"))  return true;
    if (iequals(s, "false") || s == "0" || iequals(s, "off")) return false;
    return def;
}
std::vector<std::string> INIDat::GetStrList(const std::string& sec, const std::string& key) const {
    auto v = TryGet(sec, key);
    return v ? *v : std::vector<std::string>{};
}
std::vector<int> INIDat::GetIntList(const std::string& sec, const std::string& key) const {
    std::vector<int> out;
    auto v = TryGet(sec, key); if (!v) return out;
    for (auto& s : *v) { try { out.push_back(std::stoi(s)); } catch (...) { out.push_back(0); } }
    return out;
}
std::vector<float> INIDat::GetFloatList(const std::string& sec, const std::string& key) const {
    std::vector<float> out;
    auto v = TryGet(sec, key); if (!v) return out;
    for (auto& s : *v) { try { out.push_back(std::stof(s)); } catch (...) { out.push_back(0.0f); } }
    return out;
}
