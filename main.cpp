#include <complex>
#include <iostream>
#include <memory>
#include <QDBusMetaType>
#include <QDebug>
#include <vector>
#include <print>
#include <QProcess>

#include "DBusInterface.h"

namespace SignalStatus {
    // TODO: Redo everything with QStrings
    QString runCommand(const QString &executable, const QStringList &args) {
        QProcess p;
        p.start(executable, args);
        p.waitForFinished();

        return p.readAll();
    }

    void updateProfile(const QString &about, const QString &emoji) {
        runCommand(
            "signal-cli", {"updateProfile", "--about", about});
    }

    QVariant getValue(QVariantMap map, const QString &key) {
        if (map.keys().contains(key)) {
            return map[key];
        }
        return QString("");
    }

    QString formatTime(long long totalSeconds) {
        long long hours = totalSeconds / 3600;
        long long minutes = totalSeconds % 3600 / 60;
        long long seconds = totalSeconds % 60;

        if (!hours)
            return QString::asprintf("%lld:%02lld", minutes, seconds);
        return QString::asprintf("%lld:%02lld:%02lld", hours, minutes, seconds);
    }

    Player *findPlayerByName(const QString &name, std::vector<Player> &players) {
        for (Player &player: players) {
            if (player.name == name) {
                return &player;
            }
        }
        return nullptr;
    }

    QVector<QString> getPlayerNames(std::vector<Player> *players) {
        QVector<QString> playerNames;
        for (Player &player: *players) {
            playerNames.append(player.name);
        }

        return playerNames;
    }

    QString buildAbout(const Player *player) {
        QString title = getValue(player->Metadata, "xesam:title").toString();
        QString artist = getValue(player->Metadata, "xesam:artist").toString();
        QString album = getValue(player->Metadata, "xesam:album").toString();
        long long length = getValue(player->Metadata, "mpris:length").toLongLong();
        long long position = player->Position;

        QString playPauseEmoji = player->PlaybackStatus == "Playing" ? "▶️" : "⏸️";
        return QString("Listening to:\n\n%1 %2 — %3\n\n[%4/%5]").arg(
            playPauseEmoji,
            artist == "" ? album : artist,
            title,
            formatTime(position / 1e+6),
            formatTime(length / 1e+6)
        );
    }

    void run() {
        // TODO: Remove ugly foreach loops :(

        setbuf(stdout,nullptr);

        std::println("Initializing signal-status");
        DBusInterface interface = DBusInterface();

        std::vector<Player> players = interface.getMprisPlayers();

        std::vector<Player> oldPlayers = players;
        QString selectedPlayer;
        while (true) {
            sleep(1);
            std::vector<Player> newPlayers = interface.getMprisPlayers();
            QVector<QString> playerNames = getPlayerNames(&players);

            // TODO: DO THIS WITH RANGES
            // bool need_refresh = std::ranges::any_of(players, [&](const Player &p) { return p.name == some_name; });
            //bool need_refresh =
            bool refresh = false;
            for (Player &player: newPlayers) {
                if (!playerNames.contains(player.name)) {
                    std::println("Found new player: {}, refreshing...", player.name.toStdString());

                    refresh = true;
                    break;
                }
            }
            if (refresh) {
                selectedPlayer = "";
                players = interface.getMprisPlayers();
            }

            int maxPriority = 0;
            refresh = false;
            for (Player &player: players) {
                player.poll();

                if (!player.isValid) {
                    std::println("Player {} is invalid, refreshing...", player.name.toStdString());
                    selectedPlayer = "";

                    // TODO: Find better way than player names
                    refresh = true;
                    break;
                }

                if (player.PlaybackStatus == "Playing" && player.priority > maxPriority) {
                    maxPriority = player.priority;
                }
            }

            if (refresh)
                players = interface.getMprisPlayers();

            if (oldPlayers == players) {
                continue;
            }

            oldPlayers = players;

            // Try to select a player
            for (Player &player: players) {
                if (player.PlaybackStatus == "Playing" && player.name != selectedPlayer && player.priority == maxPriority) {
                    std::println("selecting player: {}", player.name.toStdString());
                    selectedPlayer = player.name;
                }
            }

            if (selectedPlayer == "") continue;

            // Get pointer to selected player by name
            const Player *actualPlayer = findPlayerByName(selectedPlayer, players);

            if (actualPlayer == nullptr) continue;

            updateProfile(buildAbout(actualPlayer), "🎧");
        }
    }
}


int main() {
    SignalStatus::run();
    return 0;
}
