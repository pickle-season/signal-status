#include "DBusInterface.h"
#include <QDBusMetaType>
#include <QDebug>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <print>
#include <ctime>

#include "Session.h"

namespace SignalStatus {
    void onExit(int code) {
        std::println("exiting... please wait");

        Utils::updateProfile("No activity detected", "☕");

        std::println("done");
        exit(0);
    }

    [[noreturn]] void run() {
        // TODO: Add check if signal-cli is installed
        // TODO: Add signal-cli linking

        setbuf(stdout, nullptr);
        signal(SIGTERM, onExit);
        signal(SIGINT, onExit);

        std::println("Initializing signal-status");

        Session session;

        std::size_t oldHash = session.getHash();

        // Main service loop
        while (true) {
            sleep(1);

            // refresh players if needed
            if (session.needsRefresh())
                session.refreshPlayers();

            // if nothing changed, continue
            if (oldHash == session.getHash())
                continue;

            oldHash = session.getHash();

            // try to select player, if no player available, continue
            if (!session.selectPlayer())
                continue;

            // finally, update profile
            session.updateProfile();
        }
    }
} // namespace SignalStatus

int main() {
    SignalStatus::run();
}
