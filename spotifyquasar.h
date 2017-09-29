#pragma once

#include <QCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>

#include <plugin_api.h>
#include <plugin_support.h>

#define PLUGIN_NAME "Spotify Plugin"
#define PLUGIN_CODE "spotify"

#define qlog(l, f, ...)                                                \
    {                                                                  \
        char msg[256];                                                 \
        snprintf(msg, sizeof(msg), PLUGIN_CODE ": " f, ##__VA_ARGS__); \
        quasar_log(l, msg);                                            \
    }

#define debug(f, ...) qlog(QUASAR_LOG_DEBUG, f, ##__VA_ARGS__)
#define info(f, ...) qlog(QUASAR_LOG_INFO, f, ##__VA_ARGS__)
#define warn(f, ...) qlog(QUASAR_LOG_WARNING, f, ##__VA_ARGS__)

class QNetworkAccessManager;

class SpotifyQuasar : public QObject
{
    Q_OBJECT;

public:
    SpotifyQuasar(quasar_plugin_handle handle);

    bool isAvailable() { return m_available; }

    QString getResponse(QString loc, QList<QPair<QString, QString>> params = QList<QPair<QString, QString>>(), QString resource = "remote");

private slots:
    void handleResponse(QNetworkReply* reply);

private:
    bool call(QString loc, QList<QPair<QString, QString>> params = QList<QPair<QString, QString>>(), QString resource = "remote");

    bool tryConnect();
    bool getCSRF();
    bool getOAuth();

private:
    QString m_url = "http://localhost:";
    QString m_csrf;
    QString m_oauth;
    quint16 m_port      = 4381;
    bool    m_available = false;

    QNetworkAccessManager*     m_manager = nullptr;
    QNetworkRequest            m_defaultrequest;
    QMap<QString, QJsonObject> m_response;
    QCache<QString, QString>   m_albumcovercache;
    quasar_plugin_handle       m_handle;
};
