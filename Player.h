#pragma once

#include <QDBusInterface>
#include <qdbusargument.h>
#include <qdbusreply.h>
#include <utility>
#include<compare>
#include "Utils.h"


namespace SignalStatus {
    enum PlaybackStatus {
        STOPPED,
        PAUSED,
        PLAYING,
    };

    class Player {
        public:
            explicit Player(QString name) : name(std::move(name)) {}

            std::map<std::string, PlaybackStatus> statusMap{
                {"Stopped", STOPPED},
                {"Paused", PAUSED},
                {"Playing", PLAYING},
            };

            // TODO: Change PlaybackStatus to Enum
            QString name;
            bool isValid = true;
            PlaybackStatus playbackStatus = STOPPED;
            QVariantMap metadata;
            long long position = 0;


            bool operator==(const Player& right) const {
                return name == right.name
                    && playbackStatus == right.playbackStatus
                    && metadata == right.metadata
                    && position == right.position;
            }

            bool operator<(const Player& other) const {
                if (playbackStatus != other.playbackStatus)
                    return playbackStatus < other.playbackStatus;

                return name.length() > other.name.length();
            }

            auto operator<=>(const Player& other) const {
                // TODO: Find a better way to do this maybe?
                if (playbackStatus > other.playbackStatus)
                    return std::strong_ordering::greater;
                if (playbackStatus < other.playbackStatus)
                    return std::strong_ordering::less;

                return name.length() <=> other.name.length();
            }

            void poll() {
                metadata.clear();
                const QVariant metadataVariant = getProperty("Metadata");

                if (!isValid) {
                    qInfo() << "Deselecting player:" << name;
                    return;
                }

                const auto metadataMap = metadataVariant.value<QDBusArgument>();

                metadataMap >> metadata;

                playbackStatus = statusMap[getProperty("PlaybackStatus").toString().toStdString()];
                position = getProperty("Position").toLongLong();

                // set position to 0 when length is also 0
                if (!metadata["mpris:length"].toLongLong()) {
                    position = 0;
                }
            }

        private:
            // TODO: Refactor out remaining std::string
            QVariant getProperty(const QString& property) {
                QDBusInterface interface(
                    name,
                    "/org/mpris/MediaPlayer2",
                    "org.freedesktop.DBus.Properties",
                    QDBusConnection::sessionBus()
                );

                QDBusReply<QDBusVariant> reply =
                    interface.call("Get", "org.mpris.MediaPlayer2.Player", property);
                if (!reply.isValid()) {
                    const QDBusError* error = &reply.error();
                    qDebug() << name.toStdString() << ":"
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
            const std::size_t h_PlaybackStatus = std::hash<SignalStatus::PlaybackStatus>{}(player.playbackStatus);
            const std::size_t h_Position = std::hash<long long>{}(player.position);

            std::size_t result = h_name ^ (h_isValid << 1) ^ (h_PlaybackStatus << 2) ^ (h_Position << 3);

            std::size_t shift = 4;
            for (QVariant const& data : player.metadata) {
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
