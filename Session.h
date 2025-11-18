#ifndef SIGNAL_STATUS_SESSION_H
#define SIGNAL_STATUS_SESSION_H
#include "DBusInterface.h"

namespace SignalStatus {
    class Session {
        public:
            Session() = default;

            // returns true if succeeded, false if not
            bool selectPlayer() {
                Player* newSelectedPlayer = nullptr;
                Player& maxPlayer = *std::ranges::max_element(players);

                if (maxPlayer.PlaybackStatus == "Playing")
                    newSelectedPlayer = &maxPlayer;

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
                players = interface.getMprisPlayers();
            }

            bool needsRefresh() {
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

            [[nodiscard]] std::size_t getHash() const {
                return std::hash<std::vector<Player>>{}(players);
            };

            [[nodiscard]] QString buildAbout() const {
                QString title = selectedPlayer->Metadata["xesam:title"].toString();
                QString artist = selectedPlayer->Metadata["xesam:artist"].toString();
                QString album = selectedPlayer->Metadata["xesam:album"].toString();
                const long long length = selectedPlayer->Metadata["mpris:length"].toLongLong();
                const long long position = selectedPlayer->Position;

                QString playPauseEmoji = selectedPlayer->PlaybackStatus == "Playing" ? "▶️" : "⏸️";

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

        private:
            std::vector<Player> players;
            const DBusInterface interface = DBusInterface();
            Player* selectedPlayer = nullptr;
    };
} // SignalStatus

#endif //SIGNAL_STATUS_SESSION_H
