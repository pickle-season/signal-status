// #include <QDBusMetaType>
#include <QDebug>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <print>
#include <ctime>
#include <QCoreApplication>

#include "Session.h"

namespace SignalStatus {
    // TODO: Create a QtCoreApplication to be able to send requests

    // TODO: Move to session probably as something like runLoop()
    [[noreturn]] void run(int argc, char* argv[]) {
        Utils::LOG_LEVEL = Utils::getLogLevel(argc, argv);
        QCoreApplication app{argc, argv};

        qInstallMessageHandler(Utils::messageOutput);
        // TODO: Add check if signal-cli is installed
        // TODO: Add signal-cli linking

        setbuf(stdout, nullptr);
        signal(SIGTERM, Utils::onExit);
        signal(SIGINT, Utils::onExit);

        qInfo() << "Initializing signal-status";

        Session session;

        std::size_t oldHash = session.getHash();

        // Main service loop
        while (true) {
            sleep(1);

            // refresh players if needed
            if (session.playersNeedRefresh())
                session.refreshPlayers();

            if (session.processesNeedRefresh())
                session.refreshSteamProcesses();

            // FIXME: When SteamProcess is selected and a player is playing,
            //  the session hash always changes because of player updating location.
            //  Solution: getHash() dynamically check if hash is based on player or process

            // if nothing changed, continue
            size_t newHash = session.getHash();
            if (oldHash == newHash)
                continue;
            oldHash = newHash;

            session.updateProfile();
        }
    }
} // namespace SignalStatus

int main(int argc, char* argv[]) {
    SignalStatus::run(argc, argv);
}
