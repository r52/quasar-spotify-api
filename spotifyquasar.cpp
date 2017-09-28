#include "spotifyquasar.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

#define SPOTIFY_COMMAND QNetworkRequest::User

SpotifyQuasar::SpotifyQuasar(quasar_plugin_handle handle)
    : m_handle(handle)
{
    m_manager = new QNetworkAccessManager(this);

    m_defaultrequest.setRawHeader("Origin", "https://embed.spotify.com");

    if (tryConnect() &&
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

            // TODO: refactor this ugly pos
            if (cmd.isEmpty())
            {
                warn("Invalid command in response");
            }
            else if (cmd == "status")
            {
                m_response["album_art_status"] = json;

                // Get album art
                QUrl url("https://open.spotify.com/oembed");

                QUrlQuery query;

                auto track = json["track"].toObject();
                auto album = track["album_resource"].toObject();
                query.addQueryItem("url", album["uri"].toString());

                url.setQuery(query);

                QNetworkRequest request;
                request.setUrl(url);
                request.setAttribute(SPOTIFY_COMMAND, "album_art_status");

                m_manager->get(request);
            }
            else if (cmd == "album_art_status")
            {
                if (json["thumbnail_url"].isNull())
                {
                    warn("Error processing album art");
                }
                else
                {
                    QJsonObject dat = m_response["album_art_status"];

                    auto track             = dat["track"].toObject();
                    auto album             = track["album_resource"].toObject();
                    album["thumbnail_url"] = json["thumbnail_url"];

                    track["album_resource"] = album;
                    dat["track"]            = track;
                    m_response["status"]    = dat;

                    m_response.remove("album_art_status");
                }

                quasar_signal_data_ready(m_handle, "status");
            }
            else
            {
                warn("Unsupported command '%s' in response", cmd.toStdString().c_str());
            }
        }
    }

    reply->deleteLater();
}

bool SpotifyQuasar::tryConnect()
{
    QEventLoop eventLoop;
    connect(m_manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);

    QNetworkRequest request = m_defaultrequest;
    request.setUrl(QUrl(m_url + QString::number(m_port)));

    QNetworkReply* reply = m_manager->get(request);
    eventLoop.exec();

    // Expect the reply to be 404
    if (reply->error() == QNetworkReply::ContentNotFoundError)
    {
        m_available = true;
        m_url       = m_url + QString::number(m_port);
    }
    else
    {
        warn("Failed to reach Spotify server: %d", reply->error());
        m_available = false;
    }

    reply->deleteLater();

    return m_available;
}

bool SpotifyQuasar::getCSRF()
{
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
        debug("csrf: %s", m_csrf.toStdString().c_str());
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
        debug("oauth: %s", m_oauth.toStdString().c_str());
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
