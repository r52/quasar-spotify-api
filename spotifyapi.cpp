#include "spotifyapi.h"

#include <extension_support.hpp>

#include <QDesktopServices>
#include <QtNetworkAuth>

#include <fmt/core.h>

#include <jsoncons/json.hpp>

const QUrl apiUrl("https://api.spotify.com/v1/me/player");

namespace
{
    void convertArgToQuery(jsoncons::json& args, QUrlQuery& query, std::string_view convert)
    {
        if (args.contains(convert))
        {
            auto strval = args.at(convert).as_string();
            auto key    = QString::fromStdString(std::string{convert});
            query.addQueryItem(key, QString::fromStdString(strval));

            args.erase(convert);
        }
    }

    bool checkArgsForKey(const jsoncons::json& args, std::string_view key, std::string_view cmd, quasar_data_handle output)
    {
        if (!args.contains(key))
        {
            warn("Argument '{}' required for the '{}' endpoint.", key, cmd);
            auto err = fmt::format("Argument '{}' required.", key);
            quasar_append_error(output, err.c_str());
            return false;
        }

        return true;
    }
}  // namespace

SpotifyAPI::SpotifyAPI(quasar_ext_handle exthandle, QString cid, QString csc) :
    handle(exthandle),
    clientid(cid),
    clientsecret(csc),
    authenticated(false),
    granting(false),
    expired(false)
{
    if (nullptr == handle)
    {
        throw std::invalid_argument("null extension handle");
    }

    char buf[512];

    if (quasar_get_storage_string(handle, "refreshtoken", buf, sizeof(buf)))
    {
        refreshtoken = QString::fromLocal8Bit(buf);
    }

    oauth2            = new QOAuth2AuthorizationCodeFlow(this);

    auto replyHandler = new QOAuthHttpServerReplyHandler(QHostAddress::LocalHost, 1337, this);
    replyHandler->setCallbackPath("callback");
    oauth2->setReplyHandler(replyHandler);
    oauth2->setClientIdentifier(clientid);
    oauth2->setClientIdentifierSharedKey(clientsecret);
    oauth2->setAuthorizationUrl(QUrl("https://accounts.spotify.com/authorize"));
    oauth2->setAccessTokenUrl(QUrl("https://accounts.spotify.com/api/token"));
    oauth2->setScope("user-read-currently-playing user-read-playback-state user-modify-playback-state user-read-recently-played");
    oauth2->setContentType(QAbstractOAuth::ContentType::Json);

    if (!refreshtoken.isEmpty())
    {
        oauth2->setRefreshToken(refreshtoken);
    }

    info("Callback url is {}", replyHandler->callback().toStdString());

    connect(oauth2, &QOAuth2AuthorizationCodeFlow::statusChanged, [this](QAbstractOAuth::Status status) {
        if (status == QAbstractOAuth::Status::Granted)
        {
            info("Authenticated.");
            authenticated = true;
            granting      = false;
        }
    });

    connect(oauth2, &QOAuth2AuthorizationCodeFlow::expirationAtChanged, [this](const QDateTime& expiration) {
        expired = (QDateTime::currentDateTime() > expiration);
    });

    connect(oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);

    connect(oauth2, &QOAuth2AuthorizationCodeFlow::refreshTokenChanged, [this](const QString& refreshToken) {
        refreshtoken = refreshToken;

        auto ba      = refreshtoken.toUtf8();
        quasar_set_storage_string(handle, "refreshtoken", ba.data());
    });

    oauth2->setModifyParametersFunction([this](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant>* parameters) {
        if (stage == QAbstractOAuth::Stage::RefreshingAccessToken)
        {
            parameters->insert("client_id", clientid);
            parameters->insert("client_secret", clientsecret);
        }
    });
}

void SpotifyAPI::grant()
{
    if (clientid.isEmpty())
    {
        warn("Client ID not set for authentication.");
        return;
    }

    if (!refreshtoken.isEmpty() && !clientsecret.isEmpty())
    {
        // Refresh token instead of granting if already granted
        info("Refreshing authorization tokens.");
        oauth2->refreshAccessToken();

        QTimer timer;
        timer.setSingleShot(true);
        QEventLoop loop;
        connect(oauth2, &QOAuth2AuthorizationCodeFlow::statusChanged, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(1000);
        loop.exec();

        // If authenticated by refreshtoken
        if (timer.isActive() && authenticated && !expired)
            return;
    }

    // If a grant already initiated, don't perform another one for a while
    if (granting)
        return;

    granting = true;
    // 1 minute grant timeout
    QTimer::singleShot(60000, [this]() {
        granting = false;
    });

    info("Obtaining Authorization grant.");
    oauth2->grant();
}

void SpotifyAPI::SetClientIds(QString cid, QString csc)
{
    if (clientid != cid)
    {
        clientid = cid;
        oauth2->setClientIdentifier(clientid);
    }

    if (clientsecret != csc)
    {
        clientsecret = csc;
        oauth2->setClientIdentifierSharedKey(clientsecret);
    }
}

bool SpotifyAPI::Execute(SpotifyAPI::Command cmd, quasar_data_handle output, char* args)
{
    // check expiry
    auto curr = QDateTime::currentDateTime();
    if (curr > oauth2->expirationAt())
    {
        // renew if token expired
        expired = true;
        grant();
    }

    if (!authenticated || expired)
    {
        qWarning() << "Unauthenticated or expired access token";
        return false;
    }

    auto& dt = m_queue[cmd];

    if (dt.data_ready)
    {
        // set data
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

        // clear queue and flags
        dt.data.clear();
        dt.errs.clear();

        dt.data_ready = false;
        dt.processing = false;
        return true;
    }

    if (dt.processing)
    {
        // still processing
        return true;
    }

    // otherwise, set processing
    dt.processing = true;

    // Process command
    const auto&    cmdinfo = m_infomap[cmd];

    jsoncons::json oargs   = args ? jsoncons::json::parse(args) : jsoncons::json();

    QUrlQuery      query;

    // Validate args
    convertArgToQuery(oargs, query, "device_id");

    switch (cmd)
    {
        case VOLUME:
            {
                if (!checkArgsForKey(oargs, "volume_percent", cmdinfo.src, output))
                    return false;

                convertArgToQuery(oargs, query, "volume_percent");
                break;
            }

        case RECENTLY_PLAYED:
            {
                convertArgToQuery(oargs, query, "limit");
                convertArgToQuery(oargs, query, "after");
                convertArgToQuery(oargs, query, "before");
                break;
            }

        case REPEAT:
            {
                if (!checkArgsForKey(oargs, "state", cmdinfo.src, output))
                    return false;

                convertArgToQuery(oargs, query, "state");
                break;
            }

        case SEEK:
            {
                if (!checkArgsForKey(oargs, "position_ms", cmdinfo.src, output))
                    return false;

                convertArgToQuery(oargs, query, "position_ms");
                break;
            }

        case SHUFFLE:
            {
                if (!checkArgsForKey(oargs, "state", cmdinfo.src, output))
                    return false;

                convertArgToQuery(oargs, query, "state");
                break;
            }

        default:
            break;
    }

    // Process args into data if any
    QVariantMap parameters;

    for (const auto& member : oargs.object_range())
    {
        parameters.insert(QString::fromStdString(member.key()), QVariant::fromValue(QString::fromStdString(member.value().as_string())));
    }

    // Create query url
    QUrl cmdurl = apiUrl.url() + cmdinfo.api + "?" + query.toString(QUrl::FullyEncoded);

    switch (cmdinfo.ptcl)
    {
        case GET:
            {
                QNetworkReply* reply = oauth2->get(cmdurl, parameters);
                connect(reply, &QNetworkReply::finished, [=, &cmdinfo, this]() {
                    reply->deleteLater();

                    auto& dt      = m_queue[cmd];

                    dt.data_ready = true;
                    dt.processing = false;

                    if (reply->error() != QNetworkReply::NoError)
                    {
                        qWarning() << "SpotifyAPI:" << reply->error() << reply->errorString();
                        dt.errs.append(reply->errorString());
                        quasar_signal_data_ready(handle, cmdinfo.src.c_str());
                        return;
                    }

                    auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                    if (code != 204)
                    {
                        const auto json = reply->readAll();
                        dt.data         = json;
                    }

                    quasar_signal_data_ready(handle, cmdinfo.src.c_str());
                });

                return true;
            }

        case PUT:
        case POST:
            {
                QNetworkReply* reply = (cmdinfo.ptcl == PUT ? oauth2->put(cmdurl, parameters) : oauth2->post(cmdurl, parameters));
                connect(reply, &QNetworkReply::finished, [=, &cmdinfo, this]() {
                    reply->deleteLater();

                    auto& dt      = m_queue[cmd];

                    dt.data_ready = true;
                    dt.processing = false;

                    if (reply->error() != QNetworkReply::NoError)
                    {
                        qWarning() << "SpotifyAPI:" << reply->error() << reply->errorString();
                        dt.errs.append(reply->errorString());
                        quasar_signal_data_ready(handle, cmdinfo.src.c_str());
                        return;
                    }

                    auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                    if (code != 204)
                    {
                        dt.errs.append(QString::number(code));
                    }

                    quasar_signal_data_ready(handle, cmdinfo.src.c_str());
                });

                return true;
            }
    }

    return false;
}
