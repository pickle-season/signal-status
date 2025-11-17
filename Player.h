#ifndef SIGNAL_STATUS_PLAYER_H
#define SIGNAL_STATUS_PLAYER_H
#include <QDBusInterface>
#include <print>
#include <qdbusargument.h>
#include <qdbusreply.h>
#include <utility>

namespace SignalStatus {
    class Player {

    public:
        Player(QString name, const int priority) : name(std::move(name)), priority(priority) {}

        // TODO: Maybe change these to QStrings?
        QString name;
        int priority;
        bool isValid = true;
        QString PlaybackStatus;
        QVariantMap Metadata;
        long long Position = 0;


        bool operator==(const Player& right) const {
            return name == right.name && PlaybackStatus == right.PlaybackStatus && Metadata == right.Metadata
                && Position == right.Position;
        }

        void poll() {
            Metadata.clear();
            const QVariant metadataVariant = getProperty("Metadata");

            if (!isValid) {
                return;
            }

            auto metadataMap = metadataVariant.value<QDBusArgument>();

            metadataMap >> Metadata;

            PlaybackStatus = getProperty("PlaybackStatus").toString();
            // TODO: Check if position is valid
            Position = getProperty("Position").toLongLong();
        }

    private:
        QVariant getProperty(const std::string& property) {
            QDBusInterface interface(
                name, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus());

            QDBusReply<QDBusVariant> reply =
                interface.call("Get", "org.mpris.MediaPlayer2.Player", QString::fromStdString(property));
            if (!reply.isValid()) {
                const QDBusError* error = &reply.error();
                std::println(
                    "{}: {}: {}", name.toStdString(), error->name().toStdString(), error->message().toStdString());
                isValid = false;

                return QVariant{};
            }
            return reply.value().variant();
        }
    };
} // namespace SignalStatus

#endif // SIGNAL_STATUS_PLAYER_H
