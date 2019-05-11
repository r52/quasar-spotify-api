#include "spotifyapi.h"

#include <extension_support.h>

#include <QDesktopServices>
#include <QtNetworkAuth>

const QUrl apiUrl("https://api.spotify.com/v1/me/player");

SpotifyAPI::SpotifyAPI(quasar_ext_handle handle, QString clientid, QString clientsecret) :
    m_handle(handle), m_clientid(clientid), m_clientsecret(clientsecret), m_authenticated(false)
{
    if (nullptr == m_handle)
    {
        throw std::invalid_argument("null extension handle");
    }

    char buf[512];

    if (quasar_get_storage_string(handle, "refreshtoken", buf, sizeof(buf)))
    {
        m_refreshtoken = buf;
    }

    auto replyHandler = new QOAuthHttpServerReplyHandler(1337, this);
    m_oauth2.setReplyHandler(replyHandler);
    m_oauth2.setClientIdentifier(m_clientid);
    m_oauth2.setClientIdentifierSharedKey(m_clientsecret);
    m_oauth2.setAuthorizationUrl(QUrl("https://accounts.spotify.com/authorize"));
    m_oauth2.setAccessTokenUrl(QUrl("https://accounts.spotify.com/api/token"));
    m_oauth2.setScope("user-read-currently-playing user-read-playback-state user-modify-playback-state user-read-recently-played");

    if (!m_refreshtoken.isEmpty())
    {
        m_oauth2.setRefreshToken(m_refreshtoken);
    }

    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::statusChanged, [=](QAbstractOAuth::Status status) {
        if (status == QAbstractOAuth::Status::Granted)
        {
            m_authenticated = true;
        }
        else
        {
            m_authenticated = false;
        }
    });

    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);

    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::refreshTokenChanged, [=](const QString& refreshToken) {
        m_refreshtoken = refreshToken;

        auto ba = m_refreshtoken.toUtf8();
        quasar_set_storage_string(m_handle, "refreshtoken", ba.data());
    });
}

void SpotifyAPI::grant()
{
    m_oauth2.grant();
}

void SpotifyAPI::setClientIds(QString clientid, QString clientsecret)
{
    if (m_clientid != clientid)
    {
        m_clientid = clientid;
        m_oauth2.setClientIdentifier(m_clientid);
    }

    if (m_clientsecret != clientsecret)
    {
        m_clientsecret = clientsecret;
        m_oauth2.setClientIdentifierSharedKey(m_clientsecret);
    }
}

bool SpotifyAPI::execute(SpotifyAPI::Command cmd, quasar_data_handle output, QString args)
{
    // TODO: ARG VALIDATION, OUTPUT IF DUMPED
    const auto cmdinfo = m_infomap[cmd];

    const QUrl cmdurl = apiUrl.url() + cmdinfo.api;

    switch (cmdinfo.ptcl)
    {
        case GET:
        {
            QNetworkReply* reply = m_oauth2.get(cmdurl);
            connect(reply, &QNetworkReply::finished, [=]() {
                reply->deleteLater();
                if (reply->error() != QNetworkReply::NoError)
                {
                    qWarning() << "SpotifyAPI:" << reply->errorString();
                    return;
                }

                // TODO: READ, DUMP, SIGNAL
                const auto json = reply->readAll();

                const auto document = QJsonDocument::fromJson(json);
                Q_ASSERT(document.isObject());
                const auto rootObject = document.object();
            });

            return true;
        }

        case PUT:
        {
            QNetworkReply* reply = m_oauth2.put(cmdurl);
            connect(reply, &QNetworkReply::finished, [=]() {
                reply->deleteLater();
                if (reply->error() != QNetworkReply::NoError)
                {
                    qWarning() << "SpotifyAPI:" << reply->errorString();
                    return;
                }
            });
            return true;
        }
    }

    return false;
}
