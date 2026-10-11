#include "FileManager.h"
#include "Utilities.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <ctime>
#include <cctype>

using namespace std;
namespace fs = std::filesystem;

// =============================================================================
// HELPERS
// =============================================================================

// Current local time as YYYYMMDD_HHMMSS (sortable, filesystem safe)
static string fileTimestamp() {
    string stamp = getCurrentDateTime();   // YYYY-MM-DD HH:MM:SS
    string compact;
    for (char c : stamp) {
        if (isdigit(static_cast<unsigned char>(c))) compact += c;
        else if (c == ' ') compact += '_';
    }
    return compact;
}

// Parse a YYYYMMDD_HHMMSS prefix into time_t (-1 if it does not match)
static time_t parseFileTimestamp(const string& text) {
    if (text.size() < 15 || text[8] != '_') return -1;
    for (size_t i = 0; i < 15; i++) {
        if (i == 8) continue;
        if (!isdigit(static_cast<unsigned char>(text[i]))) return -1;
    }
    tm t{};
    t.tm_year = stoi(text.substr(0, 4)) - 1900;
    t.tm_mon = stoi(text.substr(4, 2)) - 1;
    t.tm_mday = stoi(text.substr(6, 2));
    t.tm_hour = stoi(text.substr(9, 2));
    t.tm_min = stoi(text.substr(11, 2));
    t.tm_sec = stoi(text.substr(13, 2));
    t.tm_isdst = -1;
    return mktime(&t);
}

// =============================================================================
// CONSTRUCTOR AND DIRECTORIES
// =============================================================================

FileManager::FileManager(string dataDir, string backupDir, string logDir) {
    dataDirectory = dataDir;
    backupDirectory = backupDir;
    logDirectory = logDir;
    maxLogSize = 10ULL * 1024 * 1024;
}

bool FileManager::ensureDirectories() const {
    try {
        fs::create_directories(dataDirectory);
        fs::create_directories(backupDirectory);
        fs::create_directories(logDirectory);
        return true;
    } catch (const exception&) {
        return false;
    }
}

string FileManager::getDataDirectory() const {
    return dataDirectory;
}

string FileManager::getBackupDirectory() const {
    return backupDirectory;
}

string FileManager::getLogDirectory() const {
    return logDirectory;
}

string FileManager::getDataFilePath(const string& filename) const {
    return (fs::path(dataDirectory) / filename).string();
}

// =============================================================================
// RECORD FILES
// =============================================================================

// Write records to a temp file and rename it over the target, so a crash
// mid-write never leaves a half-written data file behind
bool FileManager::writeRecords(const string& filename, const string& header,
                               const vector<vector<string>>& records) const {
    try {
        fs::create_directories(dataDirectory);
        string path = getDataFilePath(filename);
        string tempPath = path + ".tmp";
        {
            ofstream file(tempPath);
            if (!file.is_open()) return false;
            if (!header.empty()) file << "# " << header << '\n';
            for (const auto& record : records) {
                file << joinRecord(record) << '\n';
            }
            if (!file.good()) return false;
        }
        fs::rename(tempPath, path);
        return true;
    } catch (const exception&) {
        return false;
    }
}

bool FileManager::readRecords(const string& filename, vector<vector<string>>& records) const {
    records.clear();
    ifstream file(getDataFilePath(filename));
    if (!file.is_open()) return false;
    string line;
    while (getline(file, line)) {
        if (trim(line).empty() || line[0] == '#') continue;
        records.push_back(splitRecord(line));
    }
    return true;
}

bool FileManager::dataFileExists(const string& filename) const {
    return fs::exists(getDataFilePath(filename));
}

// =============================================================================
// KEY=VALUE FILES
// =============================================================================

bool FileManager::loadKeyValueFile(const string& path, map<string, string>& values) {
    ifstream file(path);
    if (!file.is_open()) return false;
    string line;
    while (getline(file, line)) {
        string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        size_t eq = trimmed.find('=');
        if (eq == string::npos) continue;
        values[trim(trimmed.substr(0, eq))] = trim(trimmed.substr(eq + 1));
    }
    return true;
}

bool FileManager::writeKeyValueFile(const string& path, const map<string, string>& values, const string& header) {
    try {
        fs::path target(path);
        if (target.has_parent_path()) fs::create_directories(target.parent_path());
        string tempPath = path + ".tmp";
        {
            ofstream file(tempPath);
            if (!file.is_open()) return false;
            if (!header.empty()) file << "# " << header << '\n';
            for (const auto& [key, value] : values) {
                file << key << "=" << value << '\n';
            }
            if (!file.good()) return false;
        }
        fs::rename(tempPath, path);
        return true;
    } catch (const exception&) {
        return false;
    }
}

// Replace (or append) a single key, leaving comments and other lines untouched
bool FileManager::updateKeyValueFile(const string& path, const string& key, const string& value) {
    vector<string> lines;
    bool replaced = false;
    {
        ifstream file(path);
        string line;
        while (file.is_open() && getline(file, line)) {
            string trimmed = trim(line);
            size_t eq = trimmed.find('=');
            if (!trimmed.empty() && trimmed[0] != '#' && eq != string::npos &&
                trim(trimmed.substr(0, eq)) == key) {
                lines.push_back(key + "=" + value);
                replaced = true;
            } else {
                lines.push_back(line);
            }
        }
    }
    if (!replaced) lines.push_back(key + "=" + value);

    try {
        fs::path target(path);
        if (target.has_parent_path()) fs::create_directories(target.parent_path());
        string tempPath = path + ".tmp";
        {
            ofstream out(tempPath);
            if (!out.is_open()) return false;
            for (const string& line : lines) out << line << '\n';
            if (!out.good()) return false;
        }
        fs::rename(tempPath, path);
        return true;
    } catch (const exception&) {
        return false;
    }
}

// =============================================================================
// BACKUPS
// =============================================================================

// Copy every data file into backup/<YYYYMMDD_HHMMSS>/
bool FileManager::createBackup(string& backupName) const {
    try {
        if (!fs::exists(dataDirectory)) return false;
        backupName = fileTimestamp();
        fs::path folder = fs::path(backupDirectory) / backupName;
        for (int suffix = 2; fs::exists(folder); suffix++) {
            folder = fs::path(backupDirectory) / (backupName + "_" + to_string(suffix));
        }
        backupName = folder.filename().string();
        fs::create_directories(folder);
        int copied = 0;
        for (const auto& entry : fs::directory_iterator(dataDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dat") {
                fs::copy_file(entry.path(), folder / entry.path().filename(), fs::copy_options::overwrite_existing);
                copied++;
            }
        }
        if (copied == 0) {
            fs::remove_all(folder);
            return false;
        }
        return true;
    } catch (const exception&) {
        return false;
    }
}

// Copy a backup's data files back into the data directory
bool FileManager::restoreBackup(const string& backupName) const {
    try {
        // Reject names that could escape the backup directory
        if (backupName.empty() || backupName.find("..") != string::npos ||
            backupName.find('/') != string::npos || backupName.find('\\') != string::npos) {
            return false;
        }
        fs::path folder = fs::path(backupDirectory) / backupName;
        if (!fs::is_directory(folder)) return false;
        fs::create_directories(dataDirectory);
        int restored = 0;
        for (const auto& entry : fs::directory_iterator(folder)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dat") {
                fs::copy_file(entry.path(), fs::path(dataDirectory) / entry.path().filename(),
                              fs::copy_options::overwrite_existing);
                restored++;
            }
        }
        return restored > 0;
    } catch (const exception&) {
        return false;
    }
}

// List backup folders, newest first
vector<string> FileManager::listBackups() const {
    vector<string> backups;
    try {
        if (!fs::exists(backupDirectory)) return backups;
        for (const auto& entry : fs::directory_iterator(backupDirectory)) {
            string name = entry.path().filename().string();
            if (entry.is_directory() && parseFileTimestamp(name) != -1) {
                backups.push_back(name);
            }
        }
    } catch (const exception&) {
        return backups;
    }
    sort(backups.rbegin(), backups.rend());
    return backups;
}

// True if there is no backup, or the newest is older than intervalHours
bool FileManager::isBackupDue(int intervalHours) const {
    if (intervalHours <= 0) return false;
    vector<string> backups = listBackups();
    if (backups.empty()) return true;
    time_t newest = parseFileTimestamp(backups.front());
    if (newest == -1) return true;
    return difftime(time(nullptr), newest) >= intervalHours * 3600.0;
}

// =============================================================================
// LOGGING
// =============================================================================

void FileManager::setMaxLogSize(unsigned long long bytes) {
    if (bytes > 0) maxLogSize = bytes;
}

// Parse sizes such as "10MB", "512KB", "1GB" or plain bytes
unsigned long long FileManager::parseSize(const string& text, unsigned long long fallback) {
    string value = toUpper(trim(text));
    if (value.empty()) return fallback;
    unsigned long long multiplier = 1;
    auto endsWith = [&value](const string& suffix) {
        return value.size() > suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    };
    if (endsWith("GB")) multiplier = 1024ULL * 1024 * 1024;
    else if (endsWith("MB")) multiplier = 1024ULL * 1024;
    else if (endsWith("KB")) multiplier = 1024ULL;
    if (multiplier > 1) value = value.substr(0, value.size() - 2);
    else if (endsWith("B")) value = value.substr(0, value.size() - 1);
    try {
        double number = stod(value);
        if (number <= 0) return fallback;
        return static_cast<unsigned long long>(number * multiplier);
    } catch (...) {
        return fallback;
    }
}

// Rename a log that has grown past maxLogSize to <name>_<timestamp>.log
void FileManager::rotateLogIfNeeded(const string& path) const {
    try {
        if (fs::exists(path) && fs::file_size(path) >= maxLogSize) {
            fs::path original(path);
            fs::path rotated = original.parent_path() /
                               (original.stem().string() + "_" + fileTimestamp() + original.extension().string());
            fs::rename(original, rotated);
        }
    } catch (const exception&) {
        // Logging must never take the application down
    }
}

void FileManager::log(LogType type, const string& message) const {
    string filename, level;
    switch (type) {
    case LogType::TRANSACTION: filename = "transactions.log"; level = "TXN"; break;
    case LogType::ERRORS:      filename = "errors.log"; level = "ERROR"; break;
    default:                   filename = "system.log"; level = "INFO"; break;
    }
    try {
        fs::create_directories(logDirectory);
    } catch (const exception&) {
        return;
    }
    string path = (fs::path(logDirectory) / filename).string();
    rotateLogIfNeeded(path);
    ofstream file(path, ios::app);
    if (file.is_open()) {
        file << "[" << getCurrentDateTime() << "] [" << level << "] " << message << '\n';
    }
}

void FileManager::logSystem(const string& message) const {
    log(LogType::SYSTEM, message);
}

void FileManager::logTransaction(const string& message) const {
    log(LogType::TRANSACTION, message);
}

void FileManager::logError(const string& message) const {
    log(LogType::ERRORS, message);
}

// Remove rotated log files (name_YYYYMMDD_HHMMSS.log) older than retentionDays
int FileManager::cleanupOldLogs(int retentionDays) const {
    if (retentionDays <= 0) return 0;
    int removed = 0;
    try {
        if (!fs::exists(logDirectory)) return 0;
        time_t cutoff = time(nullptr) - static_cast<time_t>(retentionDays) * 24 * 3600;
        for (const auto& entry : fs::directory_iterator(logDirectory)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".log") continue;
            string stem = entry.path().stem().string();
            if (stem.size() < 16) continue;
            time_t stamp = parseFileTimestamp(stem.substr(stem.size() - 15));
            if (stamp != -1 && stamp < cutoff) {
                fs::remove(entry.path());
                removed++;
            }
        }
    } catch (const exception&) {
        return removed;
    }
    return removed;
}
