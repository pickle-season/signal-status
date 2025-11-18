#ifndef SIGNAL_STATUS_SIGNALSTATUS_H
#define SIGNAL_STATUS_SIGNALSTATUS_H
#include "Player.h"
#include <QDBusConnectionInterface>

namespace SignalStatus {
    // TODO: Move functionality from this class to Session
    class DBusInterface {
        public:
            DBusInterface() : iface(QDBusConnection::sessionBus().interface()) {}

            [[nodiscard]] std::vector<Player> getMprisPlayers() const {
                QStringList names = this->iface->registeredServiceNames().value();

                std::vector<Player> players;

                std::ranges::sort(names);

                for (const QString& name : names) {
                    if (name.startsWith("org.mpris.MediaPlayer2.")) {
                        players.emplace_back(name);
                    }
                }

                return players;
            }

        private:
            QDBusConnectionInterface* iface;
    };
} // namespace SignalStatus

#endif // SIGNAL_STATUS_SIGNALSTATUS_H
