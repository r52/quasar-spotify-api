#pragma once

#include <mutex>
#include <unordered_map>

#include <QOAuth2AuthorizationCodeFlow>
#include <QObject>

#include <extension_api.h>

constexpr std::string_view EXT_FULLNAME = "Quasar Spotify API";
constexpr std::string_view EXT_NAME     = "spotify-api";

#define qlog(l, ...)                                                      \
  {                                                                       \
    auto msg = fmt::format("{}: {}", EXT_NAME, fmt::format(__VA_ARGS__)); \
    quasar_log(l, msg.c_str());                                           \
  }

#define debug(...) qlog(QUASAR_LOG_DEBUG, __VA_ARGS__)
#define info(...)  qlog(QUASAR_LOG_INFO, __VA_ARGS__)
#define warn(...)  qlog(QUASAR_LOG_WARNING, __VA_ARGS__)
#define crit(...)  qlog(QUASAR_LOG_CRITICAL, __VA_ARGS__)

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
        CURRENTLY_PLAYING,  // GET
        VOLUME,             // PUT
        PLAYER,             // GET
        PREVIOUS,           // PUT
        RECENTLY_PLAYED,    // GET
        NEXT,               // PUT
        PAUSE,              // PUT
        REPEAT,             // PUT
        PLAY,               // PUT
        SEEK,               // PUT
        SHUFFLE,            // PUT
        DEVICES             // GET
    };

    SpotifyAPI(quasar_ext_handle exthandle, QString cid, QString csc);

    void SetClientIds(QString cid, QString csc);

    bool IsAuthenticated() { return authenticated; };

    bool Execute(SpotifyAPI::Command cmd, quasar_data_handle output, char* args);

public slots:
    void grant();

private:
    struct cmd_info_t
    {
        Protocol    ptcl;
        std::string src;
        QString     api;
    };

    struct cmd_data_t
    {
        QByteArray  data;
        QStringList errs;

        bool        data_ready;
        bool        processing;
    };

    const std::unordered_map<Command, cmd_info_t> m_infomap = {
        {CURRENTLY_PLAYING, {GET, "currently-playing", "/currently-playing"}},
        {           VOLUME,                       {PUT, "volume", "/volume"}},
        {           PLAYER,                              {GET, "player", ""}},
        {         PREVIOUS,                  {POST, "previous", "/previous"}},
        {  RECENTLY_PLAYED,     {GET, "recently-played", "/recently-played"}},
        {             NEXT,                          {POST, "next", "/next"}},
        {            PAUSE,                         {PUT, "pause", "/pause"}},
        {           REPEAT,                       {PUT, "repeat", "/repeat"}},
        {             PLAY,                           {PUT, "play", "/play"}},
        {             SEEK,                           {PUT, "seek", "/seek"}},
        {          SHUFFLE,                     {PUT, "shuffle", "/shuffle"}},
        {          DEVICES,                     {GET, "devices", "/devices"}}
    };

    std::unordered_map<Command, cmd_data_t> m_queue;

    quasar_ext_handle                       handle;
    QString                                 clientid;
    QString                                 clientsecret;
    QString                                 refreshtoken;

    bool                                    authenticated;
    bool                                    granting;
    bool                                    expired;

    QOAuth2AuthorizationCodeFlow*           oauth2;

    Q_DISABLE_COPY(SpotifyAPI)
};
