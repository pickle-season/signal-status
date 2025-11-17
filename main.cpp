#include "DBusInterface.h"
#include <QDBusMetaType>
#include <QDebug>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <print>
#include <ctime>
#include <vector>
#include <algorithm>

namespace SignalStatus {
    Player* findPlayerByName(const QString& name, std::vector<Player>& players) {
        for (Player& player : players) {
            if (player.name == name) {
                return &player;
            }
        }
        return nullptr;
    }

    QVector<QString> getPlayerNames(std::vector<Player>& players) {
        QVector<QString> playerNames;
        for (Player& player : players) {
            playerNames.append(player.name);
        }

        return playerNames;
    }

    QString buildAbout(const Player* player) {
        QString title = Utils::getValue(player->Metadata, "xesam:title").toString();
        QString artist = Utils::getValue(player->Metadata, "xesam:artist").toString();
        QString album = Utils::getValue(player->Metadata, "xesam:album").toString();
        const long long length = Utils::getValue(player->Metadata, "mpris:length").toLongLong();
        const long long position = player->Position;

        QString playPauseEmoji = player->PlaybackStatus == "Playing" ? "▶️" : "⏸️";

        // TODO: Do not show time if --:--/--:--
        return QString("Playing media:\n\n%1 %2 — %3\n\n[%4/%5]").arg(
            playPauseEmoji,
            artist == "" ? album : artist,
            title,
            Utils::formatTime(position / 1e+6),
            Utils::formatTime(length / 1e+6)
        );
    }

    Player* selectPlayer(std::vector<Player>& players) {
        Player& maxPlayer = *std::ranges::max_element(players);

        if (maxPlayer.PlaybackStatus == "Playing")
            return &maxPlayer;
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

        Utils::updateProfile("No activity detected", "☕");

        std::println("done");
        exit(0);
    }

    [[noreturn]] void run() {
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

            Utils::updateProfile(buildAbout(selectedPlayer), "🎧");
        }
    }
} // namespace SignalStatus

int main() {
    SignalStatus::run();
}
