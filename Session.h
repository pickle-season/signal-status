#ifndef SIGNAL_STATUS_SESSION_H
#define SIGNAL_STATUS_SESSION_H
#include <QDBusConnectionInterface>

#include "Player.h"

namespace SignalStatus {
    class Session {
        public:
            Session() = default;

            // returns true if succeeded, false if not
            bool selectPlayer() {
                Player* newSelectedPlayer = nullptr;
                Player& maxPlayer = *std::ranges::max_element(players);

                if (maxPlayer.playbackStatus == Player::PLAYING || selectedPlayer != nullptr)
                    newSelectedPlayer = &maxPlayer;

                if (selectedPlayer != nullptr && maxPlayer.playbackStatus != Player::PLAYING)
                    return true;

                if (newSelectedPlayer != nullptr) {
                    if (newSelectedPlayer->name != (selectedPlayer == nullptr ? "" : selectedPlayer->name)) {
                        qInfo() << "Selecting player:" << newSelectedPlayer->name;
                        selectedPlayer = newSelectedPlayer;
                    }
                    if (selectedPlayer != nullptr)
                        return true;
                }
                return false;
            }

            void refreshPlayers() {
                qInfo() << "Refreshing players...";
                selectedPlayer = nullptr;
                players = getMprisPlayers();
            }

            bool needsRefresh() {
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
                return std::hash<std::vector<Player>>{}(players);
            };

            [[nodiscard]] QString buildAbout() const {
                QString title = selectedPlayer->metadata["xesam:title"].toString();
                QString artist = selectedPlayer->metadata["xesam:artist"].toString();
                QString album = selectedPlayer->metadata["xesam:album"].toString();
                const long long length = selectedPlayer->metadata["mpris:length"].toLongLong();
                const long long position = selectedPlayer->position;

                QString playPauseEmoji = selectedPlayer->playbackStatus == Player::PLAYING ? "▶️" : "⏸️";

                // TODO: Do not show time if --:--/--:--
                return QString("Playing media:\n\n%1 %2 — %3\n\n[%4/%5]").arg(
                    playPauseEmoji,
                    artist == "" ? album : artist,
                    title,
                    Utils::formatTime(position / 1e+6),
                    Utils::formatTime(length / 1e+6)
                );
            }

            void updateProfile() const {
                Utils::updateProfile(buildAbout(), "🎧");
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

            QDBusConnectionInterface* interface = QDBusConnection::sessionBus().interface();
            Player* selectedPlayer = nullptr;
    };
} // SignalStatus

#endif //SIGNAL_STATUS_SESSION_H
