#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <vector>
#include <map>

using namespace std;

// Log destinations
enum class LogType {
    SYSTEM,
    TRANSACTION,
    ERRORS
};

// Handles everything that touches the disk: record files, key=value config
// files, backups and log files. It knows nothing about banking objects; the
// Bank turns its objects into records and back.
class FileManager {
private:
    string dataDirectory;
    string backupDirectory;
    string logDirectory;
    unsigned long long maxLogSize;      // Bytes before a log file is rotated

    void rotateLogIfNeeded(const string& path) const;

public:
    // Constructor
    FileManager(string dataDir = "data", string backupDir = "backup", string logDir = "logs");

    // Directories
    bool ensureDirectories() const;
    string getDataDirectory() const;
    string getBackupDirectory() const;
    string getLogDirectory() const;
    string getDataFilePath(const string& filename) const;

    // Record files ('|' separated, '#' comment lines). Writes are atomic.
    bool writeRecords(const string& filename, const string& header,
                      const vector<vector<string>>& records) const;
    bool readRecords(const string& filename, vector<vector<string>>& records) const;
    bool dataFileExists(const string& filename) const;

    // key=value files (comments with '#'); update preserves comments and order
    static bool loadKeyValueFile(const string& path, map<string, string>& values);
    static bool writeKeyValueFile(const string& path, const map<string, string>& values, const string& header = "");
    static bool updateKeyValueFile(const string& path, const string& key, const string& value);

    // Backups: each backup is a timestamped folder under the backup directory
    bool createBackup(string& backupName) const;
    bool restoreBackup(const string& backupName) const;
    vector<string> listBackups() const;            // Newest first
    bool isBackupDue(int intervalHours) const;

    // Logging
    void setMaxLogSize(unsigned long long bytes);
    static unsigned long long parseSize(const string& text, unsigned long long fallback);
    void log(LogType type, const string& message) const;
    void logSystem(const string& message) const;
    void logTransaction(const string& message) const;
    void logError(const string& message) const;
    int cleanupOldLogs(int retentionDays) const;  // Removes rotated logs older than retentionDays
};

#endif // FILEMANAGER_H
