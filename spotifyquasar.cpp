#include "spotifyquasar.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

#define SPOTIFY_COMMAND QNetworkRequest::User

#define SPOTIFY_DEFAULT_PORT 4381
#define SPOTIFY_MIN_PORT 4370
#define SPOTIFY_MAX_PORT 4390

SpotifyQuasar::SpotifyQuasar(quasar_plugin_handle handle)
    : m_handle(handle)
{
    m_manager = new QNetworkAccessManager(this);

    m_defaultrequest.setRawHeader("Origin", "https://embed.spotify.com");

    // handlers
    using namespace std::placeholders;
    m_respcallmap["status"]           = std::bind(&SpotifyQuasar::handleStatus, this, _1);
    m_respcallmap["album_art_status"] = std::bind(&SpotifyQuasar::handleAlbumArt, this, _1);

    if (connectSpotify() &&
        getCSRF() &&
        getOAuth())
    {
        info("Connected to Spotify");

        connect(m_manager, &QNetworkAccessManager::finished, this, &SpotifyQuasar::handleResponse);
    }
}

bool SpotifyQuasar::call(QString loc, QList<QPair<QString, QString>> params, QString resource)
{
    if (!m_available)
    {
        warn("Spotify server not available");
        return false;
    }

    QUrl url(m_url + "/" + resource + "/" + loc + ".json");

    QUrlQuery query;
    query.setQueryItems(params);
    query.addQueryItem("csrf", m_csrf);
    query.addQueryItem("oauth", m_oauth);

    url.setQuery(query);

    QNetworkRequest request = m_defaultrequest;
    request.setUrl(url);
    request.setAttribute(SPOTIFY_COMMAND, loc);

    m_manager->get(request);

    return true;
}

QString SpotifyQuasar::getResponse(QString loc, QList<QPair<QString, QString>> params, QString resource)
{
    if (m_response.contains(loc))
    {
        QJsonDocument doc(m_response[loc]);

        QString response = QString::fromUtf8(doc.toJson());

        // consume the response
        m_response.remove(loc);

        return response;
    }

    if (!call(loc, params, resource))
    {
        warn("Unable to connect to Spotify server");
    }

    return QString();
}

void SpotifyQuasar::handleResponse(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        auto        dat  = reply->readAll();
        QJsonObject json = QJsonDocument::fromJson(dat).object();

        if (json.isEmpty())
        {
            warn("Invalid JSON response");
        }
        else
        {
            QString cmd = reply->request().attribute(SPOTIFY_COMMAND).toString();

            if (cmd.isEmpty())
            {
                warn("Invalid command in response");
            }
            else if (!m_respcallmap.contains(cmd))
            {
                warn("Unsupported command '%s' in response", cmd.toStdString().c_str());
            }
            else
            {
                m_respcallmap[cmd](json);
            }
        }
    }

    reply->deleteLater();
}

bool SpotifyQuasar::connectSpotify()
{
    // Try default port first
    if (testSpotifyConnection(SPOTIFY_DEFAULT_PORT))
    {
        return true;
    }

    // Otherwise try to find the port
    quint16 port = SPOTIFY_MIN_PORT;

    while (!testSpotifyConnection(port) && port < SPOTIFY_MAX_PORT)
    {
        port++;
    }

    if (!m_available)
    {
        warn("Failed to connect to Spotify app");
    }

    return m_available;
}

bool SpotifyQuasar::getCSRF()
{
    if (!m_available)
    {
        warn("Spotify connection not available");
        return false;
    }

    QEventLoop eventLoop;
    connect(m_manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);

    QNetworkRequest req = m_defaultrequest;
    req.setUrl(QUrl(m_url + "/simplecsrf/token.json"));

    QNetworkReply* reply = m_manager->get(req);
    eventLoop.exec();

    if (reply->error() == QNetworkReply::NoError)
    {
        auto          resp = reply->readAll();
        QJsonDocument jsondat(QJsonDocument::fromJson(resp));
        QJsonObject   dat = jsondat.object();

        m_csrf = dat["token"].toString();
    }
    else
    {
        //failure
        warn("Failed to acquire csrf");
        m_available = false;
    }

    reply->deleteLater();

    return m_available;
}

bool SpotifyQuasar::getOAuth()
{
    if (!m_available)
    {
        warn("Spotify connection not available");
        return false;
    }

    QEventLoop eventLoop;
    connect(m_manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);

    QNetworkRequest req;
    req.setUrl(QUrl("https://open.spotify.com/token"));

    QNetworkReply* reply = m_manager->get(req);
    eventLoop.exec();

    if (reply->error() == QNetworkReply::NoError)
    {
        auto          resp = reply->readAll();
        QJsonDocument jsondat(QJsonDocument::fromJson(resp));
        QJsonObject   dat = jsondat.object();

        m_oauth = dat["t"].toString();
    }
    else
    {
        //failure
        warn("Failed to acquire oauth");
        m_available = false;
    }

    reply->deleteLater();

    return m_available;
}

bool SpotifyQuasar::testSpotifyConnection(quint16 port)
{
    QEventLoop eventLoop;
    connect(m_manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);

    QNetworkRequest request = m_defaultrequest;
    QString         url     = "http://localhost:" + QString::number(port);
    request.setUrl(QUrl(url));

    QNetworkReply* reply = m_manager->get(request);
    eventLoop.exec();

    // Expect the reply to be 404
    if (reply->error() == QNetworkReply::ContentNotFoundError)
    {
        m_available = true;
        m_url       = url;
    }

    reply->deleteLater();

    return m_available;
}

void SpotifyQuasar::handleStatus(QJsonObject& json)
{
    auto track = json["track"].toObject();
    auto album = track["album_resource"].toObject();

    // Only make the album cover request if not cached
    if (m_albumcovercache.contains(album["uri"].toString()))
    {
        album["thumbnail_url"]  = *(m_albumcovercache.take(album["uri"].toString()));
        track["album_resource"] = album;
        json["track"]           = track;
        m_response["status"]    = json;

        quasar_signal_data_ready(m_handle, "status");
    }
    else
    {
        m_response["album_art_status"] = json;

        // Get album art
        QUrl url("https://open.spotify.com/oembed");

        QUrlQuery query;
        query.addQueryItem("url", album["uri"].toString());

        url.setQuery(query);

        QNetworkRequest request;
        request.setUrl(url);
        request.setAttribute(SPOTIFY_COMMAND, "album_art_status");

        m_manager->get(request);
    }
}

void SpotifyQuasar::handleAlbumArt(QJsonObject& json)
{
    QJsonObject dat = m_response["album_art_status"];

    if (json["thumbnail_url"].isNull())
    {
        warn("Error processing album art");
    }
    else
    {
        auto track = dat["track"].toObject();
        auto album = track["album_resource"].toObject();

        // Cache the album cover
        QString* cacheentry = new QString(json["thumbnail_url"].toString());
        m_albumcovercache.insert(album["uri"].toString(), cacheentry);
        cacheentry = nullptr;

        // Populate thumbnail
        album["thumbnail_url"]  = json["thumbnail_url"];
        track["album_resource"] = album;
        dat["track"]            = track;
    }

    m_response["status"] = dat;

    m_response.remove("album_art_status");

    quasar_signal_data_ready(m_handle, "status");
}
