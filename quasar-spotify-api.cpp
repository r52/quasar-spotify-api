#include <memory>

#include <extension_api.h>
#include <extension_support.h>

#include "spotifyapi.h"

#define EXT_FULLNAME "Quasar Spotify API"
#define EXT_NAME "spotify-api"

#define qlog(l, f, ...)                                             \
    {                                                               \
        char msg[256];                                              \
        snprintf(msg, sizeof(msg), EXT_NAME ": " f, ##__VA_ARGS__); \
        quasar_log(l, msg);                                         \
    }

#define debug(f, ...) qlog(QUASAR_LOG_DEBUG, f, ##__VA_ARGS__)
#define info(f, ...) qlog(QUASAR_LOG_INFO, f, ##__VA_ARGS__)
#define warn(f, ...) qlog(QUASAR_LOG_WARNING, f, ##__VA_ARGS__)
#define crit(f, ...) qlog(QUASAR_LOG_CRITICAL, f, ##__VA_ARGS__)

// These correspond to the Spotify Player API's endpoint names
// https://developer.spotify.com/documentation/web-api/reference-beta/
quasar_data_source_t sources[] = {{"currently-playing", QUASAR_POLLING_CLIENT, 2000, 0}, // GET
                                  {"volume", QUASAR_POLLING_CLIENT, 0, 0},               // PUT
                                  {"player", QUASAR_POLLING_CLIENT, 2000, 0},            // GET
                                  {"previous", QUASAR_POLLING_CLIENT, 0, 0},             // POST
                                  {"recently-played", QUASAR_POLLING_CLIENT, 2000, 0},   // GET
                                  {"next", QUASAR_POLLING_CLIENT, 0, 0},                 // POST
                                  {"pause", QUASAR_POLLING_CLIENT, 0, 0},                // PUT
                                  {"repeat", QUASAR_POLLING_CLIENT, 0, 0},               // PUT
                                  {"play", QUASAR_POLLING_CLIENT, 0, 0},                 // PUT
                                  {"seek", QUASAR_POLLING_CLIENT, 0, 0},                 // PUT
                                  {"shuffle", QUASAR_POLLING_CLIENT, 0, 0},              // PUT
                                  {"devices", QUASAR_POLLING_CLIENT, 2000, 0}};          // GET

namespace
{
    quasar_ext_handle m_handle       = nullptr;
    QString           m_clientid     = "";
    QString           m_clientsecret = "";

    std::unique_ptr<SpotifyAPI>                     m_api;
    std::unordered_map<size_t, SpotifyAPI::Command> m_cmdmap;
}

bool quasar_spotify_init(quasar_ext_handle handle)
{
    // init sources
    m_cmdmap[sources[0].uid]  = SpotifyAPI::CURRENTLY_PLAYING;
    m_cmdmap[sources[1].uid]  = SpotifyAPI::VOLUME;
    m_cmdmap[sources[2].uid]  = SpotifyAPI::PLAYER;
    m_cmdmap[sources[3].uid]  = SpotifyAPI::PREVIOUS;
    m_cmdmap[sources[4].uid]  = SpotifyAPI::RECENTLY_PLAYED;
    m_cmdmap[sources[5].uid]  = SpotifyAPI::NEXT;
    m_cmdmap[sources[6].uid]  = SpotifyAPI::PAUSE;
    m_cmdmap[sources[7].uid]  = SpotifyAPI::REPEAT;
    m_cmdmap[sources[8].uid]  = SpotifyAPI::PLAY;
    m_cmdmap[sources[9].uid]  = SpotifyAPI::SEEK;
    m_cmdmap[sources[10].uid] = SpotifyAPI::SHUFFLE;
    m_cmdmap[sources[11].uid] = SpotifyAPI::DEVICES;

    m_handle = handle;

    m_api.reset(new SpotifyAPI(handle, m_clientid, m_clientsecret));

    m_api->grant();

    return (m_api.get() != nullptr);
}

bool quasar_spotify_shutdown(quasar_ext_handle handle)
{
    if (m_api)
    {
        m_api.reset();
    }

    return true;
}

bool quasar_spotify_get_data(size_t srcUid, quasar_data_handle hData, char* args)
{
    if (!m_api)
    {
        crit("Tried to call get_data after failed initialization.");
        return false;
    }

    if (!m_api->authenticated())
    {
        warn("SpotifyAPI is not authenticated.");
        return false;
    }

    return m_api->execute(m_cmdmap[srcUid], hData, args);
}

quasar_settings_t* quasar_spotify_create_settings()
{
    auto settings = quasar_create_settings();

    quasar_add_string(settings, "clientid", "Client ID", "", true);
    quasar_add_string(settings, "clientsecret", "Client Secret", "", true);

    return settings;
}

void quasar_spotify_update_settings(quasar_settings_t* settings)
{
    char buf[512];

    quasar_get_string(settings, "clientid", buf, sizeof(buf));
    QString clientid = buf;

    quasar_get_string(settings, "clientsecret", buf, sizeof(buf));
    QString clientsecret = buf;

    if (clientid != m_clientid || clientsecret != m_clientsecret)
    {
        m_clientid     = clientid;
        m_clientsecret = clientsecret;

        if (m_api)
        {
            m_api->setClientIds(m_clientid, m_clientsecret);

            if (!m_api->authenticated())
            {
                m_api->grant();
            }
        }
    }
}

quasar_ext_info_fields_t fields =
    {EXT_NAME, EXT_FULLNAME, "2.0", "r52", "Provides Spotify API endpoints for Quasar", "https://github.com/r52/quasar-spotify-api"};

quasar_ext_info_t info = {
    QUASAR_API_VERSION,
    &fields,

    std::size(sources),
    sources,

    quasar_spotify_init,            // init
    quasar_spotify_shutdown,        // shutdown
    quasar_spotify_get_data,        // data
    quasar_spotify_create_settings, // create setting
    quasar_spotify_update_settings  // update setting
};

quasar_ext_info_t* quasar_ext_load(void)
{
    return &info;
}

void quasar_ext_destroy(quasar_ext_info_t* info)
{
    // does nothing; info is on stack
}
