#include <functional>
#include <map>

#include <plugin_api.h>
#include <plugin_support.h>

#include "spotifyquasar.h"

using GetDataFnType = std::function<bool(quasar_data_handle hData)>;
using DataCallTable = std::map<size_t, GetDataFnType>;

static DataCallTable calltable;

static std::unique_ptr<SpotifyQuasar> spotify;

enum SpotifyDataSources
{
    SPOT_SRC_STATUS = 0
};

quasar_data_source_t sources[1] =
    {
        { "status", 0, 0 }
    };

bool spotify_quasar_get_status(quasar_data_handle hData)
{
    if (!spotify || !spotify->isAvailable())
    {
        return false;
    }

    QString resp = spotify->getResponse("status");

    if (!resp.isEmpty())
    {
        quasar_set_data_json(hData, resp.toStdString().c_str());
    }

    return true;
}

bool spotify_quasar_init(quasar_plugin_handle handle)
{
    // Process uid entries.
    if (sources[SPOT_SRC_STATUS].uid != 0)
    {
        calltable[sources[SPOT_SRC_STATUS].uid] = spotify_quasar_get_status;
    }

    spotify.reset(new SpotifyQuasar(handle));

    return true;
}

bool spotify_quasar_shutdown(quasar_plugin_handle handle)
{
    spotify.reset();

    return true;
}

bool spotify_quasar_get_data(size_t srcUid, quasar_data_handle hData)
{
    if (calltable.count(srcUid) == 0)
    {
        warn("Unknown source %Iu", srcUid);
    }
    else
    {
        return calltable[srcUid](hData);
    }

    return false;
}

quasar_plugin_info_t info =
    {
        QUASAR_API_VERSION,
        PLUGIN_NAME,
        PLUGIN_CODE,
        "v1",
        "me",
        "Queries Spotify player for current status",

        _countof(sources),
        sources,

        spotify_quasar_init,
        spotify_quasar_shutdown,
        spotify_quasar_get_data,
        nullptr,
        nullptr
    };

quasar_plugin_info_t* quasar_plugin_load(void)
{
    return &info;
}

void quasar_plugin_destroy(quasar_plugin_info_t* info)
{
    // does nothing; info is on stack
}
