#include <functional>
#include <map>
#include <memory>

#include <plugin_api.h>
#include <plugin_support.h>

#include "spotifyquasar.h"

using DataCallTable = std::unordered_map<size_t, size_t>;

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

bool spotify_quasar_init(quasar_plugin_handle handle)
{
    // Process uid entries.
    if (sources[SPOT_SRC_STATUS].uid != 0)
    {
        calltable[sources[SPOT_SRC_STATUS].uid] = SPOT_SRC_STATUS;
    }

    spotify.reset(new SpotifyQuasar(handle));

    return spotify->isAvailable();
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
        return false;
    }

    if (!spotify || !spotify->isAvailable())
    {
        return false;
    }

    QString resp = spotify->getResponse(sources[calltable[srcUid]].dataSrc);

    if (!resp.isEmpty())
    {
        quasar_set_data_json(hData, resp.toStdString().c_str());
    }

    return true;
}

quasar_plugin_info_t info =
    {
        QUASAR_API_VERSION,
        PLUGIN_NAME,
        PLUGIN_CODE,
        "v1",
        "me",
        "Queries Spotify player for current status",

        std::size(sources),
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
