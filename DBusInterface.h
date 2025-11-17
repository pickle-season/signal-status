#ifndef SIGNAL_STATUS_SIGNALSTATUS_H
#define SIGNAL_STATUS_SIGNALSTATUS_H
#include <QDBusConnectionInterface>

#include "Player.h"

namespace SignalStatus {
    class DBusInterface {
        public:
            DBusInterface() : iface(QDBusConnection::sessionBus().interface()) {
            }

            [[nodiscard]] bool isValid() const {
                return iface->isValid();
            }

            [[nodiscard]] std::vector<Player> getMprisPlayers() const {

                QStringList names = this->iface->registeredServiceNames().value();

                std::vector<Player> players;

                int count = 0;
                for (const QString &name : names) {
                    if (name.startsWith("org.mpris.MediaPlayer2.")) {

                        players.emplace_back(
                                name,
                                count
                        );
                        count++;
                    }
                }

                return players;
            }
        private:
            QDBusConnectionInterface *iface;
    };
} // SignalStatus

#endif //SIGNAL_STATUS_SIGNALSTATUS_H