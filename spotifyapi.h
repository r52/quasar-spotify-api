#pragma once

#include <mutex>
#include <unordered_map>

#include <QOAuth2AuthorizationCodeFlow>
#include <QObject>

#include <extension_api.h>

class SpotifyAPI : public QObject
{
    Q_OBJECT

public:
    enum Protocol
    {
        GET,
        PUT,
        POST
    };

    enum Command
    {
        CURRENTLY_PLAYING, // GET
        VOLUME,            // PUT
        PLAYER,            // GET
        PREVIOUS,          // PUT
        RECENTLY_PLAYED,   // GET
        NEXT,              // PUT
        PAUSE,             // PUT
        REPEAT,            // PUT
        PLAY,              // PUT
        SEEK,              // PUT
        SHUFFLE,           // PUT
        DEVICES            // GET
    };

    SpotifyAPI(quasar_ext_handle handle, QString clientid, QString clientsecret);

    void setClientIds(QString clientid, QString clientsecret);

    bool authenticated() { return m_authenticated; };

    bool execute(SpotifyAPI::Command cmd, quasar_data_handle output, QString args);

public slots:
    void grant();

private:
    bool checkArgsForKey(const QJsonObject& args, const QString& key, const QString& cmd, quasar_data_handle output);
    void convertArgToQuery(QJsonObject& args, QUrlQuery& query, const QString& convert);

    struct cmd_info_t
    {
        Protocol ptcl;
        QString  src;
        QString  api;
    };

    struct cmd_data_t
    {
        QByteArray  data;
        QStringList errs;

        bool data_ready;
        bool processing;
    };

    std::unordered_map<Command, cmd_info_t> m_infomap = {{CURRENTLY_PLAYING, {GET, "currently-playing", "/currently-playing"}},
                                                         {VOLUME, {PUT, "volume", "/volume"}},
                                                         {PLAYER, {GET, "player", ""}},
                                                         {PREVIOUS, {POST, "previous", "/previous"}},
                                                         {RECENTLY_PLAYED, {GET, "recently-played", "/recently-played"}},
                                                         {NEXT, {POST, "next", "/next"}},
                                                         {PAUSE, {PUT, "pause", "/pause"}},
                                                         {REPEAT, {PUT, "repeat", "/repeat"}},
                                                         {PLAY, {PUT, "play", "/play"}},
                                                         {SEEK, {PUT, "seek", "/seek"}},
                                                         {SHUFFLE, {PUT, "shuffle", "/shuffle"}},
                                                         {DEVICES, {GET, "devices", "/devices"}}};

    std::unordered_map<Command, cmd_data_t> m_queue;

    quasar_ext_handle m_handle;
    QString           m_clientid;
    QString           m_clientsecret;
    QString           m_refreshtoken;

    bool m_authenticated;
    bool m_granting;
    bool m_expired;

    QNetworkAccessManager*        m_manager;
    QOAuth2AuthorizationCodeFlow* m_oauth2;

    Q_DISABLE_COPY(SpotifyAPI)
};
