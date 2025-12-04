#pragma once

#include <QtNetwork/QNetworkAccessManager>
#include <QCoreApplication>
#include <QCoreApplication>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace SignalStatus {
    QByteArray httpGet(QUrl url) {
        QNetworkAccessManager mgr;
        QNetworkRequest req(url);
        QNetworkReply* reply = mgr.get(req);

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        QByteArray data;
        if (reply->error() == QNetworkReply::NoError)
            data = reply->readAll();

        reply->deleteLater();
        return data;
    }

    // https://store.steampowered.com/api/appdetails?appids=220
    class SteamProcess {
        public:
            QString name;

            explicit SteamProcess(const int appId, const int processId) : appId(appId), processId(processId) {
                // TODO: Add some error handling
                const QByteArray data = httpGet(
                    QUrl(QString("https://store.steampowered.com/api/appdetails?appids=%1").arg(appId))
                );

                const auto gameInfo = QJsonDocument::fromJson(data).object();
                //auto foobar = gameInfo.value(QString::number(appId)).toObject().value("success").toBool();
                name = gameInfo[QString::number(appId)]["data"]["name"].toString();

                qDebug() << data;
            }

           bool isValid() {
                auto dir = std::filesystem::path(std::format("/proc/{}/", processId));
                if (is_directory(dir)) {
                    return true;
                }
                qDebug() << "SteamProcess " << name << "is not valid.";
                return false;
            }

        private:
            int appId;
            int processId;
    };
}

namespace std {
    template <>
    struct hash<SignalStatus::SteamProcess> {
        std::size_t operator()(const SignalStatus::SteamProcess& steamProcess) const noexcept {
            return std::hash<QString>{}(steamProcess.name);
        }
    };
}
