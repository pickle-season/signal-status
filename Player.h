#ifndef SIGNAL_STATUS_PLAYER_H
#define SIGNAL_STATUS_PLAYER_H
#include <memory>
#include <qdbusargument.h>
#include <QDBusInterface>
#include <qdbusreply.h>
#include <print>

namespace SignalStatus {
    class Player {

    public:
        Player(QString name, int priority) : priority(priority) {
            this->name = name.toStdString();
        }

        // TODO: Maybe change these to QStrings?
        std::string name;
        int priority;
        bool isValid = true;
        //std::string Shuffle;
        std::string PlaybackStatus;
        QVariantMap Metadata;
        long long Position;



        bool operator== (const Player &right) const {
            return name == right.name &&
                   //Shuffle == right.Shuffle &&
                   PlaybackStatus == right.PlaybackStatus &&
                   Metadata == right.Metadata &&
                   Position == right.Position;
        }

        void poll() {
            Metadata.clear();
            const QDBusArgument metadataMap = getProperty("Metadata").value<QDBusArgument>();

            if (!isValid)
                return;

            metadataMap >> Metadata;

            PlaybackStatus = getProperty("PlaybackStatus").toString().toStdString();
            //Shuffle = getProperty("Shuffle").toString().toStdString();
            Position = getProperty("Position").toLongLong();
        }

    private:
        QVariant getProperty(std::string property) {
            QDBusInterface interface(
                QString::fromStdString(name),
            "/org/mpris/MediaPlayer2",
            "org.freedesktop.DBus.Properties",
                QDBusConnection::sessionBus()
                );

            QDBusReply<QDBusVariant> reply = interface.call("Get", "org.mpris.MediaPlayer2.Player", QString::fromStdString(property));
            if (!reply.isValid()) {
                QDBusError error = reply.error();
                std::println("{}: {}: {}", name, error.name().toStdString(), error.message().toStdString());
                isValid = false;

                return QVariant();
            }
            return reply.value().variant();
        }
    };
} // SignalStatus

#endif //SIGNAL_STATUS_PLAYER_H