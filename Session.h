#pragma once

#include <QDBusConnectionInterface>

#include "Player.h"
#include "SteamProcess.h"

namespace SignalStatus {
    class Session {
        public:
            Session() = default;

            void updateSelectedPlayer() {
                Player& maxPlayer = *std::ranges::max_element(players);

                // If nothing changed, return
                if (selectedPlayer == &maxPlayer)
                    return;

                // If the newly selected player is stopped or paused, we do not update because
                // we want to remember the last player that was playing media.
                // In other words, if no players are playing, selectedPlayer will not update
                if (maxPlayer.playbackStatus == PAUSED || maxPlayer.playbackStatus == STOPPED)
                    return;

                qInfo() << "Selecting player:" << maxPlayer.name;
                selectedPlayer = &maxPlayer;
            }

            void refreshPlayers() {
                qInfo() << "Refreshing players...";
                selectedPlayer = nullptr;
                players = getMprisPlayers();
            }

            void refreshSteamProcesses() {
                //qDebug() << "Refreshing steam processes...";
                steamProcess = Utils::getSteamProcess();
            }

            bool processesNeedRefresh() {
                return steamProcess ? !steamProcess->isValid() : true;
            }

            bool playersNeedRefresh() {
                // needs refresh if there is any new player whose name is not in the names of players
                return std::ranges::any_of(
                    getMprisPlayers(),
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

            [[nodiscard]] std::size_t getHash() const {
                return std::hash<std::vector<Player>>{}(players) ^ ((steamProcess ? std::hash<SteamProcess>{}(*steamProcess) : 0) << 1);
            };

            // TODO: Add time tracking?
            [[nodiscard]] QString buildGameAbout() const {
                return QString("Playing: %1").arg(
                    steamProcess->name
                );
            }

            [[nodiscard]] QString buildMediaAbout() const {
                QString title = selectedPlayer->metadata["xesam:title"].toString();
                QString artist = selectedPlayer->metadata["xesam:artist"].toString();
                QString album = selectedPlayer->metadata["xesam:album"].toString();
                const long long length = selectedPlayer->metadata["mpris:length"].toLongLong();
                const long long position = selectedPlayer->position;

                QString playPauseEmoji = selectedPlayer->playbackStatus == PLAYING ? "▶️" : "⏸️";

                // TODO: Do not show time if --:--/--:--
                return QString("Playing media:\n\n%1 %2 — %3\n\n[%4/%5]").arg(
                    playPauseEmoji,
                    artist == "" ? album : artist,
                    title,
                    Utils::formatTime(position / 1e+6),
                    Utils::formatTime(length / 1e+6)
                );
            }

            void updateProfile() {
                updateSelectedPlayer();

                if (steamProcess) {
                    Utils::updateProfile(buildGameAbout(), "🎮");
                    return;
                }

                if (selectedPlayer)
                    Utils::updateProfile(buildMediaAbout(), "🎧");
            }


            [[nodiscard]] std::vector<Player> getMprisPlayers() const {
                QStringList names = interface->registeredServiceNames().value();

                std::vector<Player> foundPlayers;

                std::ranges::sort(names);

                for (const QString& name : names) {
                    if (name.startsWith("org.mpris.MediaPlayer2.")) {
                        foundPlayers.emplace_back(name);
                    }
                }

                return foundPlayers;
            }

        private:
            std::vector<Player> players;
            std::optional<SteamProcess> steamProcess;

            QCoreApplication* app = QCoreApplication::instance();
            QDBusConnectionInterface* interface = QDBusConnection::sessionBus().interface();
            Player* selectedPlayer = nullptr;
    };
} // SignalStatus
