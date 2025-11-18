#ifndef SIGNAL_STATUS_PLAYER_H
#define SIGNAL_STATUS_PLAYER_H
#include <QDBusInterface>
#include <print>
#include <qdbusargument.h>
#include <qdbusreply.h>
#include <utility>
#include<compare>
#include "Utils.h"


namespace SignalStatus {
    class Player {
        public:
            explicit Player(QString name) : name(std::move(name)) {}

            // TODO: Change PlaybackStatus to Enum
            QString name;
            bool isValid = true;
            QString PlaybackStatus;
            QVariantMap Metadata;
            long long Position = 0;


            bool operator==(const Player& right) const {
                return name == right.name
                    && PlaybackStatus == right.PlaybackStatus
                    && Metadata == right.Metadata
                    && Position == right.Position;
            }

            bool operator<(const Player& other) const {
                if (PlaybackStatus != "Playing" && other.PlaybackStatus == "Playing") return true;
                if (other.PlaybackStatus != "Playing" && PlaybackStatus == "Playing") return false;

                return name.length() > other.name.length();
            }

            auto operator<=>(const Player& other) const {
                if (PlaybackStatus == "Playing" && other.PlaybackStatus != "Playing")
                    return std::strong_ordering::greater;
                if (PlaybackStatus != "Playing" && other.PlaybackStatus == "Playing")
                    return std::strong_ordering::less;

                return name.length() <=> other.name.length();
            }

            void poll() {
                Metadata.clear();
                const QVariant metadataVariant = getProperty("Metadata");

                if (!isValid) {
                    return;
                }

                const auto metadataMap = metadataVariant.value<QDBusArgument>();

                metadataMap >> Metadata;

                PlaybackStatus = getProperty("PlaybackStatus").toString();
                Position = getProperty("Position").toLongLong();

                // set position to 0 when length is also 0
                if (!Metadata["mpris:length"].toLongLong())
                    Position = 0;
            }

        private:
            QVariant getProperty(const std::string& property) {
                QDBusInterface interface(
                    name,
                    "/org/mpris/MediaPlayer2",
                    "org.freedesktop.DBus.Properties",
                    QDBusConnection::sessionBus()
                );

                QDBusReply<QDBusVariant> reply =
                    interface.call("Get", "org.mpris.MediaPlayer2.Player", QString::fromStdString(property));
                if (!reply.isValid()) {
                    const QDBusError* error = &reply.error();
                    qWarning() << name.toStdString() << ":"
                        << error->name().toStdString() << ":"
                        << error->message().toStdString();

                    isValid = false;

                    return {};
                }
                return reply.value().variant();
            }
    };
} // namespace SignalStatus

namespace std {
    template <>
    struct hash<SignalStatus::Player> {
        std::size_t operator()(const SignalStatus::Player& player) const noexcept {
            const std::size_t h_name = std::hash<QString>{}(player.name);
            const std::size_t h_isValid = std::hash<bool>{}(player.isValid);
            const std::size_t h_PlaybackStatus = std::hash<QString>{}(player.PlaybackStatus);
            const std::size_t h_Position = std::hash<long long>{}(player.Position);

            std::size_t result = h_name ^ (h_isValid << 1) ^ (h_PlaybackStatus << 2) ^ (h_Position << 3);

            std::size_t shift = 4;
            for (QVariant const& data : player.Metadata) {
                result ^= (std::hash<QString>{}(data.toString()) << shift);
                shift++;
            }

            return result;
        }
    };

    template <>
    struct hash<vector<SignalStatus::Player>> {
        std::size_t operator()(const std::vector<SignalStatus::Player>& playerVector) const noexcept {
            std::size_t result = 0;

            std::size_t shift = 0;
            for (const SignalStatus::Player& player : playerVector) {
                result ^= (std::hash<SignalStatus::Player>{}(player) << shift);
                shift++;
            }

            return result;
        }
    };
}

#endif // SIGNAL_STATUS_PLAYER_H
