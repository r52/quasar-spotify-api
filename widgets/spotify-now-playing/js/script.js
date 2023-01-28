let websocket = null;
let device_id = "";
let current_id = "";

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function query(cmd, args) {
  let msg = {
    method: "query",
    params: {
      topics: [`spotify-api/${cmd}`],
    },
  };

  if (args) {
    msg.params["args"] = args;
  }

  websocket.send(JSON.stringify(msg));
}

function poll_current() {
  query(
    "currently-playing",
    device_id === ""
      ? null
      : {
          device_id: device_id,
        },
  );
}

async function initialize(socket) {
  quasar_authenticate(socket);
  sleep(10);
  query("devices");

  setInterval(poll_current, 5000);
}

function parseMsg(msg) {
  var data = JSON.parse(msg);

  if ("spotify-api/devices" in data) {
    // parse devices
    // just take the first computer?

    const dev_list = data["spotify-api/devices"]["devices"];
    dev_list.forEach((e) => {
      if (e.type == "Computer") {
        device_id = e.id;
        return true;
      }

      return false;
    });
  }

  if ("spotify-api/currently-playing" in data) {
    const entry = data["spotify-api/currently-playing"];

    if (current_id != entry["item"]["id"]) {
      const track = entry["item"]["name"];
      const artist = entry["item"]["artists"].reduce((acc, cur) => {
        return acc + (acc === "" ? "" : ", ") + cur.name;
      }, "");
      const album_cover =
        "url(" + entry["item"]["album"]["images"][1]["url"] + ")";

      $(".track").text(track);
      $(".artist").text(artist);
      $(".album_cover").css("background-image", album_cover);

      current_id = entry["item"]["id"];
    }
  }
}

$(function () {
  try {
    if (websocket && websocket.readyState == 1) websocket.close();
    websocket = quasar_create_websocket();
    websocket.onopen = function (evt) {
      initialize(websocket);
    };
    websocket.onmessage = function (evt) {
      parseMsg(evt.data);
    };
    websocket.onerror = function (evt) {
      console.log("ERROR: " + evt.data);
    };
  } catch (exception) {
    console.log("Exception: " + exception);
  }
});
