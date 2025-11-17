#include "DBusInterface.h"
#include <QDBusMetaType>
#include <QDebug>
#include <QProcess>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <print>
#include <time.h>
#include <vector>

namespace SignalStatus {
    // TODO: Redo everything with QStrings
    QString runCommand(const QString& executable, const QStringList& args) {
        QProcess p;
        p.start(executable, args);
        p.waitForFinished();

        // TODO: Read stderr separately and print if it's not empty
        return p.readAll();
    }

    void updateProfile(const QString& about, const QString& emoji) {
        runCommand(
            "signal-cli",
            {
                "updateProfile",
                "--about", about,
                "--about-emoji", emoji
            }
        );
    }

    QVariant getValue(QVariantMap map, const QString& key) {
        if (map.keys().contains(key)) {
            return map[key];
        }
        return QString("");
    }

    QString formatTime(long long totalSeconds) {
        long long hours = totalSeconds / 3600;
        long long minutes = totalSeconds % 3600 / 60;
        long long seconds = totalSeconds % 60;

        // TODO use --:-- when unknown time or 0:00
        if (!hours) {
            return QString::asprintf("%lld:%02lld", minutes, seconds);
        }
        return QString::asprintf("%lld:%02lld:%02lld", hours, minutes, seconds);
    }

    Player* findPlayerByName(const QString& name, std::vector<Player>& players) {
        for (Player& player : players) {
            if (player.name == name) {
                return &player;
            }
        }
        return nullptr;
    }

    QVector<QString> getPlayerNames(std::vector<Player>* players) {
        QVector<QString> playerNames;
        for (Player& player : *players) {
            playerNames.append(player.name);
        }

        return playerNames;
    }

    QString buildAbout(const Player* player) {
        QString title = getValue(player->Metadata, "xesam:title").toString();
        QString artist = getValue(player->Metadata, "xesam:artist").toString();
        QString album = getValue(player->Metadata, "xesam:album").toString();
        long long length = getValue(player->Metadata, "mpris:length").toLongLong();
        long long position = player->Position;

        QString playPauseEmoji = player->PlaybackStatus == "Playing" ? "▶️" : "⏸️";
        return QString("Playing media:\n\n%1 %2 — %3\n\n[%4/%5]").arg(
            playPauseEmoji,
            artist == "" ? album : artist,
            title,
            formatTime(position / 1e+6),
            formatTime(length / 1e+6)
        );
    }

    int getMaxPriority(std::vector<Player>& players) {
        int maxPriority = 0;
        for (Player& player : players) {
            if (player.PlaybackStatus == "Playing" && player.priority > maxPriority) {
                maxPriority = player.priority;
            }
        }

        return maxPriority;
    }

    Player* selectPlayer(std::vector<Player>& players) {
        int maxPriority = getMaxPriority(players);

        for (Player& player : players) {
            if (player.PlaybackStatus == "Playing" && player.priority == maxPriority) {
                return &player;
            }
        }
        return nullptr;
    }

    bool needsRefresh(std::vector<Player>& players, const DBusInterface& interface) {
        // needs refresh if there is any new player whose name is not in the names of players
        return std::ranges::any_of(
            interface.getMprisPlayers(),
            [&](const Player& newPlayer) {
                return !std::ranges::any_of(
                    players,
                    [&](const Player& player) {
                        return player.name == newPlayer.name;
                    }
                );
            } // or if any player is invalid
        ) || std::ranges::any_of(
            players,
            [&](Player& player) {
                player.poll();
                return !player.isValid;
            }
        );
    }

    void onExit(int code) {
        std::println("exiting... please wait");

        updateProfile("No activity detected", "☕");

        std::println("done");
        exit(0);
    }

    void run() {
        // TODO: Add check if signal-cli is installed
        // TODO: Add signal-cli linking

        setbuf(stdout, nullptr);
        signal(SIGTERM, onExit);
        signal(SIGINT, onExit);

        std::println("Initializing signal-status");
        const auto interface = DBusInterface();

        std::vector<Player> players = interface.getMprisPlayers();

        std::vector<Player> oldPlayers = players;
        const Player* selectedPlayer = nullptr;

        // Main service loop
        while (true) {
            sleep(1);

            if (needsRefresh(players, interface)) {
                std::println("Refreshing players...");
                selectedPlayer = nullptr;
                players = interface.getMprisPlayers();
            }

            if (oldPlayers == players) {
                continue;
            }
            oldPlayers = players;

            if (
                const Player* newSelectedPlayer = selectPlayer(players);
                newSelectedPlayer != nullptr
            ) {
                if (newSelectedPlayer->name != (selectedPlayer == nullptr ? "" : selectedPlayer->name)) {
                    std::println("selecting player: {}", newSelectedPlayer->name.toStdString());
                    selectedPlayer = newSelectedPlayer;
                }
            }
            else
                continue;

            updateProfile(buildAbout(selectedPlayer), "🎧");
        }
    }
} // namespace SignalStatus

int main() {
    SignalStatus::run();
    return 0;
}
