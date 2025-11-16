#ifndef SIGNAL_STATUS_SIGNALSTATUS_H
#define SIGNAL_STATUS_SIGNALSTATUS_H
#include <QDBusConnection>
#include <QDBusConnectionInterface>

#include "Player.h"

namespace SignalStatus {
    class DBusInterface {
        public:
            DBusInterface() : iface(QDBusConnection::sessionBus().interface()) {
            }

            bool isValid() {
                return iface->isValid();
            }

            std::vector<Player> getMprisPlayers()
            {

                QStringList names = this->iface->registeredServiceNames().value();

                std::vector<Player> players;

                for (const QString &name : names) {
                    if (name.startsWith("org.mpris.MediaPlayer2.")) {

                        players.push_back(
                            Player(
                                name
                            )
                        );
                    }
                }

                return players;
            }
        private:
            QDBusConnectionInterface *iface;
    };
} // SignalStatus

#endif //SIGNAL_STATUS_SIGNALSTATUS_H