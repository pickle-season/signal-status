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
                if (!Utils::getValue(Metadata, "mpris:length").toLongLong())
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
                    std::println(
                        "{}: {}: {}",
                        name.toStdString(),
                        error->name().toStdString(),
                        error->message().toStdString()
                    );
                    isValid = false;

                    return {};
                }
                return reply.value().variant();
            }
    };
} // namespace SignalStatus

#endif // SIGNAL_STATUS_PLAYER_H
