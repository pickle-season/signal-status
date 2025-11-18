#ifndef SIGNAL_STATUS_UTILS_H
#define SIGNAL_STATUS_UTILS_H
#include <QProcess>
#include <qvariant.h>

namespace SignalStatus::Utils {
    inline QVariant getValue(QVariantMap map, const QString& key) {
        if (map.keys().contains(key)) {
            return map[key];
        }
        return QString("");
    }

    inline QString runCommand(const QString& executable, const QStringList& args) {
        QProcess p;
        p.start(executable, args);
        p.waitForFinished();

        //QString std_out = p.readAllStandardOutput();
        QString std_err = p.readAllStandardError();

        if (std_err != "") std::println("WARNING: command returned stderr: {}", std_err.toStdString());
        return p.readAll();
    }

    inline void updateProfile(const QString& about, const QString& emoji) {
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

        // TODO use --:-- when unknown time or 0:00

        if (!(hours || minutes || seconds)) {
            return {"--:--"};
        }

        if (!hours) {
            return QString::asprintf("%lld:%02lld", minutes, seconds);
        }
        return QString::asprintf("%lld:%02lld:%02lld", hours, minutes, seconds);
    }
}


#endif //SIGNAL_STATUS_UTILS_H
