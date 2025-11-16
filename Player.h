#ifndef SIGNAL_STATUS_PLAYER_H
#define SIGNAL_STATUS_PLAYER_H
#include <memory>
#include <qdbusargument.h>
#include <QDBusInterface>

namespace SignalStatus {
    class Player {

    public:
        Player(QString name) {
            this->name = name.toStdString();
        }

        // TODO: Maybe change these to QStrings?
        std::string name;
        std::string Shuffle;
        std::string PlaybackStatus;
        QVariantMap Metadata;
        long long Position;



        bool operator== (const Player &right) const {
            return name == right.name &&
                   Shuffle == right.Shuffle &&
                   PlaybackStatus == right.PlaybackStatus &&
                   Metadata == right.Metadata &&
                   Position == right.Position;
        }

        void poll() {
            Metadata = getProperty<QVariantMap>("Metadata");
            PlaybackStatus = getProperty<std::string>("PlaybackStatus");
            Shuffle = getProperty<std::string>("Shuffle");
            Position = getProperty<long long>("Position");
        }

    private:
        template <typename T> T getProperty(std::string property) {
            QDBusMessage msg = QDBusMessage::createMethodCall(
                QString::fromStdString(name),
                "/org/mpris/MediaPlayer2",
                "org.freedesktop.DBus.Properties",
                "Get"
            );
            msg << "org.mpris.MediaPlayer2.Player" << property.c_str();

            QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
            QDBusVariant dbusVariant = reply.arguments().first().value<QDBusVariant>();

            if constexpr (std::is_same_v<T, std::string>) {
                return dbusVariant.variant().toString().toStdString();
            }

            else if constexpr (std::is_same_v<T, QVariantMap>) {
                QDBusArgument arg = dbusVariant.variant().value<QDBusArgument>();

                // Convert a{sv} into a QMap
                QVariantMap result;
                arg >> result;
                return result;
            }

            else if constexpr (std::is_same_v<T, long long>) {
                return dbusVariant.variant().toLongLong();
                //return dbusVariant.variant().toString().toStdString();
            }
        }
    };
} // SignalStatus

#endif //SIGNAL_STATUS_PLAYER_H