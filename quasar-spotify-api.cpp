#include <memory>

#include <extension_api.h>
#include <extension_support.hpp>

#include "spotifyapi.h"

#include <QThread>

#include <fmt/core.h>

// These correspond to the Spotify Player API's endpoint names
// https://developer.spotify.com/documentation/web-api/reference-beta/
quasar_data_source_t sources[] = {
    {"currently-playing", QUASAR_POLLING_CLIENT, 2000, 0}, // GET
    {           "volume", QUASAR_POLLING_CLIENT,    0, 0}, // PUT
    {           "player", QUASAR_POLLING_CLIENT, 2000, 0}, // GET
    {         "previous", QUASAR_POLLING_CLIENT,    0, 0}, // POST
    {  "recently-played", QUASAR_POLLING_CLIENT, 2000, 0}, // GET
    {             "next", QUASAR_POLLING_CLIENT,    0, 0}, // POST
    {            "pause", QUASAR_POLLING_CLIENT,    0, 0}, // PUT
    {           "repeat", QUASAR_POLLING_CLIENT,    0, 0}, // PUT
    {             "play", QUASAR_POLLING_CLIENT,    0, 0}, // PUT
    {             "seek", QUASAR_POLLING_CLIENT,    0, 0}, // PUT
    {          "shuffle", QUASAR_POLLING_CLIENT,    0, 0}, // PUT
    {          "devices", QUASAR_POLLING_CLIENT, 2000, 0}  // GET
};

namespace
{
    quasar_ext_handle                               extHandle      = nullptr;
    QString                                         m_clientid     = "";
    QString                                         m_clientsecret = "";

    SpotifyAPI*                                     api;
    std::unordered_map<size_t, SpotifyAPI::Command> commandMap;
}  // namespace

bool quasar_spotify_init(quasar_ext_handle handle)
{
    // init sources
    commandMap[sources[0].uid]  = SpotifyAPI::CURRENTLY_PLAYING;
    commandMap[sources[1].uid]  = SpotifyAPI::VOLUME;
    commandMap[sources[2].uid]  = SpotifyAPI::PLAYER;
    commandMap[sources[3].uid]  = SpotifyAPI::PREVIOUS;
    commandMap[sources[4].uid]  = SpotifyAPI::RECENTLY_PLAYED;
    commandMap[sources[5].uid]  = SpotifyAPI::NEXT;
    commandMap[sources[6].uid]  = SpotifyAPI::PAUSE;
    commandMap[sources[7].uid]  = SpotifyAPI::REPEAT;
    commandMap[sources[8].uid]  = SpotifyAPI::PLAY;
    commandMap[sources[9].uid]  = SpotifyAPI::SEEK;
    commandMap[sources[10].uid] = SpotifyAPI::SHUFFLE;
    commandMap[sources[11].uid] = SpotifyAPI::DEVICES;

    api                         = new SpotifyAPI(handle, m_clientid, m_clientsecret);

    QMetaObject::invokeMethod(api, [] {
        api->grant();
    });

    return (api != nullptr);
}

bool quasar_spotify_shutdown(quasar_ext_handle handle)
{
    api->deleteLater();
    api = nullptr;

    return true;
}

bool quasar_spotify_get_data(size_t srcUid, quasar_data_handle hData, char* args)
{
    if (!api)
    {
        crit("Tried to call get_data after failed initialization.");
        return false;
    }

    if (!api->IsAuthenticated())
    {
        warn("SpotifyAPI is not authenticated.");
        return false;
    }

    bool ret = false;

    // Run this stuff on the main thread
    QMetaObject::invokeMethod(
        api,
        [=, cmd = commandMap[srcUid]] {
            return api->Execute(cmd, hData, args);
        },
        Qt::BlockingQueuedConnection,
        &ret);

    return ret;
}

quasar_settings_t* quasar_spotify_create_settings(quasar_ext_handle handle)
{
    extHandle     = handle;

    auto settings = quasar_create_settings(handle);

    quasar_add_string_setting(handle, settings, "clientid", "Spotify API Client ID", "", true);
    quasar_add_string_setting(handle, settings, "clientsecret", "Spotify API Client Secret", "", true);

    return settings;
}

void quasar_spotify_update_settings(quasar_settings_t* settings)
{
    auto    cid          = quasar_get_string_setting_hpp(extHandle, settings, "clientid");
    QString clientid     = QString::fromStdString(std::string{cid});

    auto    csc          = quasar_get_string_setting_hpp(extHandle, settings, "clientsecret");
    QString clientsecret = QString::fromStdString(std::string{csc});

    if (clientid != m_clientid || clientsecret != m_clientsecret)
    {
        m_clientid     = clientid;
        m_clientsecret = clientsecret;

        if (api)
        {
            api->SetClientIds(m_clientid, m_clientsecret);

            if (!api->IsAuthenticated())
            {
                QMetaObject::invokeMethod(api, [] {
                    api->grant();
                });
            }
        }
    }
}

quasar_ext_info_fields_t fields = {.version = "3.0",
    .author                                 = "r52",
    .description                            = "Provides Spotify API endpoints for Quasar",
    .url                                    = "https://github.com/r52/quasar-spotify-api"};

quasar_ext_info_t        info   = {
    QUASAR_API_VERSION,
    &fields,

    std::size(sources),
    sources,

    quasar_spotify_init,             // init
    quasar_spotify_shutdown,         // shutdown
    quasar_spotify_get_data,         // data
    quasar_spotify_create_settings,  // create setting
    quasar_spotify_update_settings   // update setting
};

quasar_ext_info_t* quasar_ext_load(void)
{
    quasar_strcpy(fields.name, sizeof(fields.name), EXT_NAME.data(), EXT_NAME.size());
    quasar_strcpy(fields.fullname, sizeof(fields.fullname), EXT_FULLNAME.data(), EXT_FULLNAME.size());
    return &info;
}

void quasar_ext_destroy(quasar_ext_info_t* info)
{
    // does nothing; info is on stack
}
