$(document).ready(function() {
    var p = null;
    var websocket = null;
    var reg = {
        widget: qWidgetName,
        type: "poll",
        plugin: "spotify",
        source: "status"
    };

    var poll_msg = JSON.stringify(reg);

    var current = {};
    current["album_cover"] = $(".album_cover").css('background-image');
    current["artist"] = $(".artist").text();
    current["track"] = $(".track").text();

    function poll() {
        websocket.send(poll_msg);
    }

    function parseMsg(msg) {
        var data = JSON.parse(msg);
        data = data["data"];

        var track = data["track"]["track_resource"]["name"];
        var album_cover = "url("+data["track"]["album_resource"]["thumbnail_url"]+")";
        var artist = data["track"]["artist_resource"]["name"];

        if (current["track"] != track)
        {
            $(".track").text(track);
            current["track"] = track;
        }

        if (current["artist"] != artist)
        {
            $(".artist").text(artist);
            current["artist"] = artist;
        }

        if (current["album_cover"] != album_cover)
        {
            $(".album_cover").css('background-image', album_cover);
            current["album_cover"] = album_cover;
        }
    }

    try {
        if (websocket && websocket.readyState == 1)
            websocket.close();
        websocket = new WebSocket(qWsServerUrl);
        websocket.onopen = function(evt) {
            p = setInterval(poll, 2000);
        };
        websocket.onmessage = function(evt) {
            parseMsg(evt.data);
        };
        websocket.onerror = function(evt) {
            console.log('ERROR: ' + evt.data);
        };
    } catch (exception) {
        console.log('Exception: ' + exception);
    }
});
