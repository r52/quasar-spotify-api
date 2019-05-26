# quasar-spotify-api
[![Build status](https://ci.appveyor.com/api/projects/status/ms3bgy8srhw3mxm0?svg=true)](https://ci.appveyor.com/project/r52/spotify-quasar)

Spotify API extension and sample widget for Quasar.

# Setup

Since the Spotify API is rate limited for each App, this extension does not provide its own Client ID/Secret. [Register your own App](https://developer.spotify.com/documentation/general/guides/app-settings/#register-your-app) on the Spotify site and set up your Client ID and Secret in Quasar settings. Make sure whitelisted Redirect URIs is set to `http://127.0.0.1:1337/callback`

# Usage

quasar-spotify-api implements most of Spotify's [Player API](https://developer.spotify.com/documentation/web-api/reference-beta/#category-player) as Quasar Data Sources. See the [Spotify API Reference](https://developer.spotify.com/documentation/web-api/reference-beta/) for more details.

These include:

- `currently-playing`
- `volume`
- `player`
- `previous`
- `recently-played`
- `next`
- `pause`
- `repeat`
- `play`
- `seek`
- `shuffle`
- `devices`

## Sample Queries/Outputs

Query:
```json
{
  "method": "query",
  "params": {
    "target": "spotify-api",
    "params": "currently-playing"
  }
}
```

Response:
```json
{
  "data": {
    "spotify-api": {
      "currently-playing": {
        "actions": {
          "disallows": {
            "resuming": true,
            "skipping_prev": true
          }
        },
        "context": {
          "external_urls": {
            "spotify": "https://open.spotify.com/artist/1uNFoZAHBGtllmzznpCI3s"
          },
          "href": "https://api.spotify.com/v1/artists/1uNFoZAHBGtllmzznpCI3s",
          "type": "artist",
          "uri": "spotify:artist:1uNFoZAHBGtllmzznpCI3s"
        },
        "currently_playing_type": "track",
        "is_playing": true,
        "item": {
          "album": {
            "album_type": "album",
            "artists": [
              {
                "external_urls": {
                  "spotify": "https://open.spotify.com/artist/1uNFoZAHBGtllmzznpCI3s"
                },
                "href": "https://api.spotify.com/v1/artists/1uNFoZAHBGtllmzznpCI3s",
                "id": "1uNFoZAHBGtllmzznpCI3s",
                "name": "Justin Bieber",
                "type": "artist",
                "uri": "spotify:artist:1uNFoZAHBGtllmzznpCI3s"
              }
            ],
            "available_markets": [
              "AD",
              "AE",
              "AR",
              "AT",
              "AU",
              "BE",
              "BG",
              "BH",
              "BO",
              "BR",
              "CA",
              "CH",
              "CL",
              "CO",
              "CR",
              "CY",
              "CZ",
              "DE",
              "DK",
              "DO",
              "DZ",
              "EC",
              "EE",
              "EG",
              "ES",
              "FI",
              "FR",
              "GB",
              "GR",
              "GT",
              "HK",
              "HN",
              "HU",
              "ID",
              "IE",
              "IL",
              "IN",
              "IS",
              "IT",
              "JO",
              "JP",
              "KW",
              "LB",
              "LI",
              "LT",
              "LU",
              "LV",
              "MA",
              "MC",
              "MT",
              "MX",
              "MY",
              "NI",
              "NL",
              "NO",
              "NZ",
              "OM",
              "PA",
              "PE",
              "PH",
              "PL",
              "PS",
              "PT",
              "PY",
              "QA",
              "RO",
              "SA",
              "SG",
              "SK",
              "SV",
              "TH",
              "TN",
              "TR",
              "TW",
              "US",
              "UY",
              "VN",
              "ZA"
            ],
            "external_urls": {
              "spotify": "https://open.spotify.com/album/3BmcYMh0KYsimWL6p2gPa9"
            },
            "href": "https://api.spotify.com/v1/albums/3BmcYMh0KYsimWL6p2gPa9",
            "id": "3BmcYMh0KYsimWL6p2gPa9",
            "images": [
              {
                "height": 640,
                "url": "https://i.scdn.co/image/4a334e8018ca1df508e1ba39e6bf84af656f3476",
                "width": 640
              },
              {
                "height": 300,
                "url": "https://i.scdn.co/image/cb0d278295235a24110f48ba10b080d2b5e46fdf",
                "width": 300
              },
              {
                "height": 64,
                "url": "https://i.scdn.co/image/f1292cc92e5da710eb92c4ab531c2250966df2bc",
                "width": 64
              }
            ],
            "name": "My World 2.0",
            "release_date": "2010-01-01",
            "release_date_precision": "day",
            "total_tracks": 10,
            "type": "album",
            "uri": "spotify:album:3BmcYMh0KYsimWL6p2gPa9"
          },
          "artists": [
            {
              "external_urls": {
                "spotify": "https://open.spotify.com/artist/1uNFoZAHBGtllmzznpCI3s"
              },
              "href": "https://api.spotify.com/v1/artists/1uNFoZAHBGtllmzznpCI3s",
              "id": "1uNFoZAHBGtllmzznpCI3s",
              "name": "Justin Bieber",
              "type": "artist",
              "uri": "spotify:artist:1uNFoZAHBGtllmzznpCI3s"
            },
            {
              "external_urls": {
                "spotify": "https://open.spotify.com/artist/3ipn9JLAPI5GUEo4y4jcoi"
              },
              "href": "https://api.spotify.com/v1/artists/3ipn9JLAPI5GUEo4y4jcoi",
              "id": "3ipn9JLAPI5GUEo4y4jcoi",
              "name": "Ludacris",
              "type": "artist",
              "uri": "spotify:artist:3ipn9JLAPI5GUEo4y4jcoi"
            }
          ],
          "available_markets": [
            "AD",
            "AE",
            "AR",
            "AT",
            "AU",
            "BE",
            "BG",
            "BH",
            "BO",
            "BR",
            "CA",
            "CH",
            "CL",
            "CO",
            "CR",
            "CY",
            "CZ",
            "DE",
            "DK",
            "DO",
            "DZ",
            "EC",
            "EE",
            "EG",
            "ES",
            "FI",
            "FR",
            "GB",
            "GR",
            "GT",
            "HK",
            "HN",
            "HU",
            "ID",
            "IE",
            "IL",
            "IN",
            "IS",
            "IT",
            "JO",
            "JP",
            "KW",
            "LB",
            "LI",
            "LT",
            "LU",
            "LV",
            "MA",
            "MC",
            "MT",
            "MX",
            "MY",
            "NI",
            "NL",
            "NO",
            "NZ",
            "OM",
            "PA",
            "PE",
            "PH",
            "PL",
            "PS",
            "PT",
            "PY",
            "QA",
            "RO",
            "SA",
            "SG",
            "SK",
            "SV",
            "TH",
            "TN",
            "TR",
            "TW",
            "US",
            "UY",
            "VN",
            "ZA"
          ],
          "disc_number": 1,
          "duration_ms": 214240,
          "explicit": false,
          "external_ids": {
            "isrc": "USUM70919263"
          },
          "external_urls": {
            "spotify": "https://open.spotify.com/track/6epn3r7S14KUqlReYr77hA"
          },
          "href": "https://api.spotify.com/v1/tracks/6epn3r7S14KUqlReYr77hA",
          "id": "6epn3r7S14KUqlReYr77hA",
          "is_local": false,
          "name": "Baby",
          "popularity": 73,
          "preview_url": "https://p.scdn.co/mp3-preview/a7457c94f24ced0115c865b325e031ea6fb2a964?cid=ec6cd44b9caa4e91a2eff04ad431dcdd",
          "track_number": 1,
          "type": "track",
          "uri": "spotify:track:6epn3r7S14KUqlReYr77hA"
        },
        "progress_ms": 8097,
        "timestamp": 1558398525866
      }
    }
  }
}
```

Query:
```json
{
  "method": "query",
  "params": {
    "target": "spotify-api",
    "params": "play",
    "args": {
      "device_id": "yourdeviceidhere",
      "context_uri": "spotify:album:6pYNEn4tMc6gdv5fIZf5yn",
      "offset": {
        "position": 9
      }
    }
  }
}
```

Response:

No response.
