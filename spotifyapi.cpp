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

    m_manager = new QNetworkAccessManager(this);
    m_oauth2  = new QOAuth2AuthorizationCodeFlow(m_manager, this);

    auto replyHandler = new QOAuthHttpServerReplyHandler(1337, this);
    replyHandler->setCallbackPath("callback");
    m_oauth2->setReplyHandler(replyHandler);
    m_oauth2->setClientIdentifier(m_clientid);
    m_oauth2->setClientIdentifierSharedKey(m_clientsecret);
    m_oauth2->setAuthorizationUrl(QUrl("https://accounts.spotify.com/authorize"));
    m_oauth2->setAccessTokenUrl(QUrl("https://accounts.spotify.com/api/token"));
    m_oauth2->setScope("user-read-currently-playing user-read-playback-state user-modify-playback-state user-read-recently-played");
    m_oauth2->setContentType(QAbstractOAuth::ContentType::Json);

    if (!m_refreshtoken.isEmpty())
    {
        m_oauth2->setRefreshToken(m_refreshtoken);
    }

    connect(m_oauth2, &QOAuth2AuthorizationCodeFlow::statusChanged, [=](QAbstractOAuth::Status status) {
        if (status == QAbstractOAuth::Status::Granted)
        {
            qInfo() << "SpotifyAPI: Authenticated.";
            m_authenticated = true;
        }
        else
        {
            m_authenticated = false;
        }
    });

    connect(m_oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);

    connect(m_oauth2, &QOAuth2AuthorizationCodeFlow::refreshTokenChanged, [=](const QString& refreshToken) {
        m_refreshtoken = refreshToken;

        auto ba = m_refreshtoken.toUtf8();
        quasar_set_storage_string(m_handle, "refreshtoken", ba.data());
    });

    m_oauth2->setModifyParametersFunction([&](QAbstractOAuth::Stage stage, QVariantMap* parameters) {
        if (stage == QAbstractOAuth::Stage::RefreshingAccessToken)
        {
            parameters->insert("client_id", m_clientid);
            parameters->insert("client_secret", m_clientsecret);
        }
    });
}

void SpotifyAPI::grant()
{
    if (m_clientid.isEmpty())
    {
        qWarning() << "SpotifyAPI: Client ID not set for authentication.";
        return;
    }

    if (!m_refreshtoken.isEmpty() && !m_clientsecret.isEmpty())
    {
        // Refresh token instead of granting if already granted
        qInfo() << "SpotifyAPI: Refreshing authorization tokens.";
        m_oauth2->refreshAccessToken();

        QTimer timer;
        timer.setSingleShot(true);
        QEventLoop loop;
        connect(m_oauth2, &QOAuth2AuthorizationCodeFlow::statusChanged, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(1000);
        loop.exec();

        // If authenticated by refreshtoken
        if (timer.isActive() && m_authenticated)
            return;
    }

    qInfo() << "SpotifyAPI: Obtaining Authorization grant.";
    m_oauth2->grant();
}

void SpotifyAPI::setClientIds(QString clientid, QString clientsecret)
{
    if (m_clientid != clientid)
    {
        m_clientid = clientid;
        m_oauth2->setClientIdentifier(m_clientid);
    }

    if (m_clientsecret != clientsecret)
    {
        m_clientsecret = clientsecret;
        m_oauth2->setClientIdentifierSharedKey(m_clientsecret);
    }
}

bool SpotifyAPI::execute(SpotifyAPI::Command cmd, quasar_data_handle output, QString args)
{
    auto& dt = m_queue[cmd];

    {
        std::unique_lock<std::mutex> lk(dt.mtx);
        if (dt.data_ready)
        {
            if (dt.data.isEmpty())
            {
                quasar_set_data_null(output);
            }
            else
            {
                quasar_set_data_json(output, dt.data.data());
            }

            for (auto i : dt.errs)
            {
                quasar_append_error(output, i.toUtf8().data());
            }

            dt.data.clear();
            dt.errs.clear();

            dt.data_ready = false;
            return true;
        }

        if (dt.processing)
        {
            // still processing
            return true;
        }

        // set processiing
        dt.processing = true;
    }

    const auto cmdinfo = m_infomap[cmd];

    QUrlQuery query;
    QUrlQuery argq(args);

    // Validate args
    convertArgToQuery(argq, query, "device_id");

    switch (cmd)
    {
        case VOLUME:
        {
            if (!checkArgsForKey(argq, "volume_percent", cmdinfo.src, output))
                return false;

            convertArgToQuery(argq, query, "volume_percent");
            break;
        }

        case REPEAT:
        {
            if (!checkArgsForKey(argq, "state", cmdinfo.src, output))
                return false;

            convertArgToQuery(argq, query, "state");
            break;
        }

        case SEEK:
        {
            if (!checkArgsForKey(argq, "position_ms", cmdinfo.src, output))
                return false;

            convertArgToQuery(argq, query, "position_ms");
            break;
        }

        case SHUFFLE:
        {
            if (!checkArgsForKey(argq, "state", cmdinfo.src, output))
                return false;

            convertArgToQuery(argq, query, "state");
            break;
        }

        default:
            break;
    }

    // Process args into data if any
    auto parameters = convertArgsToParameters(argq);

    // Create query url
    QUrl cmdurl = apiUrl.url() + cmdinfo.api + "?" + query.toString(QUrl::FullyEncoded);

    switch (cmdinfo.ptcl)
    {
        case GET:
        {
            QNetworkReply* reply = m_oauth2->get(cmdurl, parameters);
            connect(reply, &QNetworkReply::finished, [=]() {
                reply->deleteLater();

                auto& dt = m_queue[cmd];

                {
                    std::unique_lock<std::mutex> lk(dt.mtx);

                    dt.data_ready = true;
                    dt.processing = false;

                    if (reply->error() != QNetworkReply::NoError)
                    {
                        qWarning() << "SpotifyAPI:" << reply->errorString();
                        dt.errs.append(reply->errorString());
                        quasar_signal_data_ready(m_handle, cmdinfo.src.toUtf8().data());
                        return;
                    }

                    auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                    if (code != 204)
                    {
                        const auto json = reply->readAll();
                        dt.data         = json;
                    }

                    quasar_signal_data_ready(m_handle, cmdinfo.src.toUtf8().data());
                }
            });

            return true;
        }

        case PUT:
        case POST:
        {
            QNetworkReply* reply = (cmdinfo.ptcl == PUT ? m_oauth2->put(cmdurl, parameters) : m_oauth2->post(cmdurl, parameters));
            connect(reply, &QNetworkReply::finished, [=]() {
                reply->deleteLater();

                auto& dt = m_queue[cmd];

                {
                    std::unique_lock<std::mutex> lk(dt.mtx);

                    dt.data_ready = true;
                    dt.processing = false;

                    if (reply->error() != QNetworkReply::NoError)
                    {
                        qWarning() << "SpotifyAPI:" << reply->errorString();
                        dt.errs.append(reply->errorString());
                        quasar_signal_data_ready(m_handle, cmdinfo.src.toUtf8().data());
                        return;
                    }

                    auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                    if (code != 204)
                    {
                        dt.errs.append(QString::number(code));
                    }

                    quasar_signal_data_ready(m_handle, cmdinfo.src.toUtf8().data());
                }
            });

            return true;
        }
    }

    return false;
}

bool SpotifyAPI::checkArgsForKey(const QUrlQuery& args, const QString& key, const QString& cmd, quasar_data_handle output)
{
    if (!args.hasQueryItem(key))
    {
        qWarning() << "SpotifyAPI: Argument '" << key << "' required for the '" << cmd << "' endpoint.";
        QString m{"Argument '" + key + "' required."};
        quasar_append_error(output, m.toUtf8().data());
        return false;
    }

    return true;
}

void SpotifyAPI::convertArgToQuery(QUrlQuery& args, QUrlQuery& query, const QString& convert)
{
    if (args.hasQueryItem(convert))
    {
        auto v = args.queryItemValue(convert);
        query.addQueryItem(convert, v);
        args.removeQueryItem(convert);
    }
}

QVariantMap SpotifyAPI::convertArgsToParameters(const QUrlQuery& args)
{
    QVariantMap parameters;

    auto alist = args.queryItems();
    for (auto e : alist)
    {
        parameters.insert(e.first, e.second);
    }

    return parameters;
}
