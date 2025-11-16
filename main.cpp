#include <iostream>
#include <memory>
#include <QDBusMetaType>
#include <QDebug>
#include <vector>
#include <print>

#include "DBusInterface.h"

namespace SignalStatus {
    std::string exec(const char *cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    void updateProfile(std::string about, std::string emoji) {
        exec(
            std::format("signal-cli updateProfile --about \"{}\" --about-emoji {} 2>&1 >/dev/null", about, emoji).c_str());
    }

    QVariant getValue(QVariantMap map, std::string key) {
        if (map.keys().contains(QString::fromStdString(key))) {
            return map[QString::fromStdString(key)];
        }
        return QString::fromStdString("");
    }

    std::string formatTime(int totalSeconds) {
        int hours = totalSeconds / 3600;
        int minutes = totalSeconds % 3600 / 60;
        int seconds = totalSeconds % 60;

        if (!hours)
            return std::format("{}:{:02}", minutes, seconds);
        return std::format("{}:{:02}:{:02}", hours, minutes, seconds);
    }

    void run() {
        // TODO: Remove ugly foreach loops :(

        setbuf(stdout,NULL);

        std::println("Initializing signal-status");
        DBusInterface interface = DBusInterface();

        std::vector<Player> players = interface.getMprisPlayers();

        std::vector<Player> oldPlayers = players;
        Player *selectedPlayer = nullptr;
        while (true) {
            sleep(1);
            std::vector<Player> newPlayers = interface.getMprisPlayers();
            QVector<std::string> player_names = {};
            for (Player &player: players) {
                player_names.append(player.name);
            }

            for (Player &player: newPlayers) {
                if (!player_names.contains(player.name)) {
                    std::println("Found new player: {}, refreshing...", player.name);
                    selectedPlayer = nullptr;
                    players = newPlayers;
                }
            }
            int maxPriority = 0;
            for (Player &player: players) {
                player.poll();

                if (!player.isValid) {
                    std::println("Player {} is invalid, refreshing...", player.name);
                    selectedPlayer = nullptr;
                    players = interface.getMprisPlayers();
                }

                if (player.PlaybackStatus == "Playing" && player.priority > maxPriority) {
                    maxPriority = player.priority;
                }
            }

            if (oldPlayers == players) {
                continue;
            }

            oldPlayers = players;

            for (Player &player: players) {
                std::string playerName = "";

                if (selectedPlayer != nullptr) {
                    playerName = selectedPlayer->name;
                }

                if (player.PlaybackStatus == "Playing" && player.name != playerName && player.priority == maxPriority) {
                    std::println("selecting player: {}", player.name);
                    selectedPlayer = &player;
                }
            }

            if (selectedPlayer == nullptr) continue;

            std::string title = getValue(selectedPlayer->Metadata, "xesam:title").toString().toStdString();
            std::string artist = getValue(selectedPlayer->Metadata, "xesam:artist").toString().toStdString();
            std::string album = getValue(selectedPlayer->Metadata, "xesam:album").toString().toStdString();
            long long length = getValue(selectedPlayer->Metadata, "mpris:length").toLongLong();
            long long position = selectedPlayer->Position;

            std::string playPauseEmoji = selectedPlayer->PlaybackStatus == "Playing" ? "▶️" : "⏸️";
            std::string about = std::format(
                "Listening to:\n\n{} {} — {}\n\n[{}/{}]",
                playPauseEmoji,
                artist=="" ? album : artist,
                title,
                formatTime(position / 1e+6),
                formatTime(length / 1e+6)
            );
            updateProfile(about, "🎧");
        }
    }
}


int main() {
    SignalStatus::run();
    return 0;
}
