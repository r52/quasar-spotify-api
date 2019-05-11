#pragma once

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
        PUT
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
    struct cmd_info_t
    {
        Protocol ptcl;
        QString  api;
    };

    std::unordered_map<Command, cmd_info_t> m_infomap = {{CURRENTLY_PLAYING, {GET, "/currently-playing"}},
                                                         {VOLUME, {PUT, "/volume"}},
                                                         {PLAYER, {GET, ""}},
                                                         {PREVIOUS, {PUT, "/previous"}},
                                                         {RECENTLY_PLAYED, {GET, "/recently-played"}},
                                                         {NEXT, {PUT, "/next"}},
                                                         {PAUSE, {PUT, "/pause"}},
                                                         {REPEAT, {PUT, "/repeat"}},
                                                         {PLAY, {PUT, "/play"}},
                                                         {SEEK, {PUT, "/seek"}},
                                                         {SHUFFLE, {PUT, "/shuffle"}},
                                                         {DEVICES, {GET, "/devices"}}};

    quasar_ext_handle m_handle;
    QString           m_clientid;
    QString           m_clientsecret;
    QString           m_refreshtoken;
    QString           m_state;

    bool m_authenticated;

    QOAuth2AuthorizationCodeFlow m_oauth2;

    Q_DISABLE_COPY(SpotifyAPI)
};
