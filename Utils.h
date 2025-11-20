#ifndef SIGNAL_STATUS_UTILS_H
#define SIGNAL_STATUS_UTILS_H
#include <QProcess>
#include <QDebug>
#include <filesystem>
#include <fstream>
#include <optional>

#include "SteamProcess.h"

namespace SignalStatus::Utils {
    enum LogLevel {
        DEBUG,
        INFO,
        WARNING,
        CRITICAL,
        FATAL,
    };

    inline std::map<LogLevel, QString> levelMap = {
        {DEBUG, "DEBUG"},
        {INFO, "INFO"},
        {WARNING, "WARNING"},
        {CRITICAL, "CRITICAL"},
        {FATAL, "FATAL"},
    };
    inline std::map<QtMsgType, LogLevel> Qt2SS = {
        {QtDebugMsg, DEBUG},
        {QtInfoMsg, INFO},
        {QtWarningMsg, WARNING},
        {QtCriticalMsg, CRITICAL},
        {QtFatalMsg, FATAL},
    };

    inline LogLevel LOG_LEVEL;

    inline void messageOutput(const QtMsgType type, const QMessageLogContext& context, const QString& msg) {
        const LogLevel logLevel = Qt2SS[type];
        if (logLevel < LOG_LEVEL) return;

        QTextStream cout(stdout, QIODevice::WriteOnly);
        cout << levelMap[logLevel] << ": " << msg << Qt::endl;
    }

    inline QString runCommand(const QString& executable, const QStringList& args) {
        QProcess p;
        p.start(executable, args);
        p.waitForFinished();

        //QString std_out = p.readAllStandardOutput();

        if (const QString stdErr = p.readAllStandardError(); stdErr != "")
            qWarning() << "Command returned stderr:" << stdErr;
        return p.readAll();
    }

    inline void updateProfile(const QString& about, const QString& emoji) {
        qDebug() << "Updating profile: " << about << " Emoji: " << emoji;
        runCommand(
            "signal-cli",
            {
                "updateProfile",
                "--about", about,
                "--about-emoji", emoji
            }
        );
    }

    inline QString formatTime(const long long totalSeconds) {
        const long long hours = totalSeconds / 3600;
        const long long minutes = totalSeconds % 3600 / 60;
        const long long seconds = totalSeconds % 60;

        // TODO: use --:-- when unknown time or 0:00

        if (!(hours || minutes || seconds)) {
            return {"--:--"};
        }

        if (!hours) {
            return QString::asprintf("%lld:%02lld", minutes, seconds);
        }
        return QString::asprintf("%lld:%02lld:%02lld", hours, minutes, seconds);
    }

    inline LogLevel getLogLevel(const int argc, char* argv[]) {
        for (int i = 1; i < argc; i++) {
            if (!strcmp(argv[i], "--log-level")) {
                if (i + 1 < argc) {
                    return (*std::ranges::find_if(
                        SignalStatus::Utils::levelMap,
                        [argv, i](const auto& element) { return argv[i + 1] == element.second; }
                    )).first;
                }
            }
        }
        return INFO;
    }

    inline void onExit(int code) {
        qInfo() << "Exiting... please wait";

        updateProfile("No activity detected", "☕");

        qInfo() << "Done";
        exit(0);
    }

    bool isNumber(const std::string& s) {
        return !s.empty() && std::find_if(
            s.begin(),
            s.end(),
            [](unsigned char c) { return !std::isdigit(c); }
        ) == s.end();
    }

    // TODO: MAYBE implement selecting between multiple steam processes
    inline std::optional<SteamProcess> getSteamProcess() {
        std::string searchString = "SteamLaunch AppId=";

        // For every process
        for (const auto& dirEntry : std::filesystem::directory_iterator("/proc/")) {
            if (!dirEntry.is_directory() || !isNumber(dirEntry.path().filename()))
                continue;

            int processId = stoi(dirEntry.path().filename().string());
            std::string commandLine;
            std::ifstream commandLineStream(std::format("/proc/{}/cmdline", processId));
            getline(commandLineStream, commandLine);

            std::ranges::replace(commandLine, '\0', ' ');
            if (size_t idPosition = commandLine.find(searchString); idPosition != std::string::npos) {
                // I'm convinced this can be done easier with string views or ranges
                std::string appId;

                for (char curChar : commandLine.substr(idPosition + searchString.length())) {
                    if (curChar == ' ') break;
                    appId.push_back(curChar);
                }
                qDebug() << processId << ": " << appId << ": " << commandLine;
                SteamProcess process{stoi(appId), processId};
                qInfo() << "Selecting SteamProcess:" << process.name << "PID:" << processId << "AID:" << appId;
                return process;
            }
        }
        return std::nullopt;
    }
}


#endif //SIGNAL_STATUS_UTILS_H
