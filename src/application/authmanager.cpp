/***************************************************************************
 *   Copyright (C) 2017 by Jeremy Whiting <jeremypwhiting@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation version 2 of the License.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "authmanager.h"

#include <cassert>
#include <algorithm>
#include <set>
#include <stdexcept>

#include <QDomDocument>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

#include "common.h"
#include "log.h"
#include "flag.h"
#include "pingwaiter.h"
#include "setting.h"
#include "vpnservicemanager.h"

static QStringList subscriptions;

#define kHubIndex "hub"
#define kServerIndex "server"

std::auto_ptr<AuthManager> AuthManager::mInstance;
AuthManager * AuthManager::instance()
{
    if (!mInstance.get())
        mInstance.reset(new AuthManager());
    return mInstance.get();
}

bool AuthManager::exists()
{
    return (mInstance.get() != nullptr);
}

void AuthManager::cleanup()
{
    if (mInstance.get() != nullptr)
        delete mInstance.release();
}

AuthManager::AuthManager()
    :mLoggedIn(false),
     mCancellingLogin(false),
     mIPAttemptCount(0),
     mServersModel(new ServersModel(this)),
     mHubsModel(new ServersModel(this))
{
    subscriptions << AuthManager::tr("Free");
    subscriptions << AuthManager::tr("Standard");
    subscriptions << AuthManager::tr("Premium");

    connect(VPNServiceManager::instance(), &VPNServiceManager::gotNewIp,
            this, &AuthManager::setNewIp);
}

AuthManager::~AuthManager()
{
    for (size_t k = 0; k < mWorkers.size(); ++k) {
        if (mTimers.at(k) != nullptr) {
            mTimers.at(k)->stop();
            delete mTimers.at(k);
        }
        /*              if (_workers.at(k) != NULL && _waiters.at(k) != NULL)
                        {
                                MainWindow * m = MainWindow::Instance();
                        }*/
        if (mWorkers.at(k) != nullptr) {
            if (mWorkers.at(k)->state() != QProcess::NotRunning)
                mWorkers.at(k)->terminate();
            mWorkers.at(k)->deleteLater();
        }
        if (mWaiters.at(k) != nullptr)
            delete mWaiters.at(k);
    }
    // TODO: -0 terminate Network Manager
//      _nam
}

bool AuthManager::loggedIn()
{
    // TODO: -0 not implemented
    return mLoggedIn;
}

void AuthManager::login(const QString & name, const QString & password)
{
    mAccountLogin = name;
    mAccountPassword = password;

    mLoggedIn = false;
    mCancellingLogin = false;
    Log::logt("Starting login with name '" + QUrl::toPercentEncoding(name, "", "") + "'");
    Log::logt("Password encoded is " + QUrl::toPercentEncoding(password, "", ""));

    QUrlQuery postData;
    postData.addQueryItem("username", QUrl::toPercentEncoding(name, "", ""));
    postData.addQueryItem("password", QUrl::toPercentEncoding(password, "", ""));

    QNetworkRequest request(QUrl::fromUserInput(kLoginUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    mReply = mNAM.post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    connect(mReply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(loginNetworkError(QNetworkReply::NetworkError)));
    connect(mReply, SIGNAL(finished()),
            this, SLOT(loginFinished()));

    if (Setting::instance()->rememberMe()) {
        Setting::instance()->setLogin(name);
        Setting::instance()->setPassword(password);
    }
}

void AuthManager::cancel()
{
    mCancellingLogin = true;
    if(mReply != nullptr) {
        mReply->abort();
    }
}

void AuthManager::logout()
{
    cancel();
    mLoggedIn = false;
    mAccountLogin.clear();
    mAccountPassword.clear();            // TODO: -2 secure clear

    emit loggedInChanged();
}

void AuthManager::createAccount(const QString &email, const QString &name, const QString &password)
{
    Q_UNUSED(name)
    // Create account with given e-mail, name, and password of type free with
    // expiration 1 year in the future. Send message to gui when complete
    QUrlQuery postData;
    postData.addQueryItem("email", QUrl::toPercentEncoding(email));
    postData.addQueryItem("password", QUrl::toPercentEncoding(password));
    postData.addQueryItem("type", "0");

    QNetworkRequest request(QUrl::fromUserInput(kCreateAccountUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // TODO: Implement as api is designed on server side as required
//    mCreateAccountReply = mNAM.post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
//    connect(mCreateAccountReply, SIGNAL(error(QNetworkReply::NetworkError)),
//            this, SLOT(createAccountError(QNetworkReply::NetworkError)));
//    connect(mCreateAccountReply, &QNetworkReply::finished,
//            this, &AuthManager::createAccountFinished);
}

const QString &AuthManager::accountName()
{
    return mAccountLogin;    // TODO: -1 check: still valid, synchro with the main wnd
}

const QString &AuthManager::accountPassword()
{
    return mAccountPassword;
}

const QString &AuthManager::email()
{
    return mEmail;
}

const QString &AuthManager::subscription()
{
    return mSubscription;
}

const QString &AuthManager::expiration()
{
    return mExpiration;
}

const QString &AuthManager::newIP()
{
    return mNewIP;
}

const QString &AuthManager::oldIP()
{
    return mOldIP;
}

AServer *AuthManager::getServer(int id, bool forceServer)
{
    AServer *s = nullptr;
    //assert(id > -1);
    if (Setting::instance()->showNodes() || forceServer) {
        if (id > -1 && id < mServersModel->count()) {
            s = mServersModel->server(id);
        } else {
            Log::logt("getServer called with invalid id " + QString::number(id));
        }
    } else {
        if (id > - 1 && id < mHubsModel->count()) {
            s = mHubsModel->server(id);
        } else {
            Log::logt("getServer called with invalid hub id " + QString::number(id));
        }
    }
    return s;
}

AServer *AuthManager::getHub(int id)
{
    AServer *s = nullptr;
    //assert(id > -1);
    if (id > -1 && id < mHubsModel->count()) {
        s = mHubsModel->server(id);
    } else {
        Log::logt("getHub called with invalid id " + QString::number(id));
    }
    return s;
}

void AuthManager::setNewIp(const QString & ip)
{
    static const QString self = "127.0.0.1";
    if (ip != self) {
        mNewIP = ip;
        emit newIpLoaded(ip);
    }
}

void AuthManager::getServerList()
{
    // If we are already loading a server list, stop the request
    if (!mServerListReply.isNull() && !mServerListReply->isFinished())
        mServerListReply->abort();

    if (!mLoggedIn) {
        Log::logt("Fetching default server list");

        QNetworkRequest request(QUrl::fromUserInput(kServersUrl));

        mServerListReply = mNAM.get(request);
    } else {
        Log::logt("Fetching server list for logged in user");

        QUrlQuery postData;
        postData.addQueryItem("username", QUrl::toPercentEncoding(mAccountLogin, "", ""));
        postData.addQueryItem("password", QUrl::toPercentEncoding(mAccountPassword, "", ""));

        QNetworkRequest request(QUrl::fromUserInput(kUserServersUrl));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        mServerListReply = mNAM.post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    }
    connect(mServerListReply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(fetchServerListError(QNetworkReply::NetworkError)));
    connect(mServerListReply, SIGNAL(finished()),
            this, SLOT(fetchServerListFinished()));
}

void AuthManager::getHubs()
{
    if (!mLoggedIn) {
        Log::logt("Fetching default hubs list");

        QNetworkRequest request(QUrl::fromUserInput(kHubsUrl));

        mHubsReply = mNAM.get(request);
    } else {
        Log::logt("Fetching hubs for logged in user");

        QUrlQuery postData;
        postData.addQueryItem("username", QUrl::toPercentEncoding(mAccountLogin, "", ""));
        postData.addQueryItem("password", QUrl::toPercentEncoding(mAccountPassword, "", ""));

        QNetworkRequest request(QUrl::fromUserInput(kUserHubsUrl));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        mHubsReply = mNAM.post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    }
    connect(mHubsReply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(fetchHubsError(QNetworkReply::NetworkError)));
    connect(mHubsReply, SIGNAL(finished()),
            this, SLOT(fetchHubsFinished()));
}

void AuthManager::nextFavorite()
{
    // Set server to next favorite
    // Find next favorite
    int startIndex = Setting::instance()->favorite();
    int index = startIndex;
    Log::logt(QString("Next favorite called, current favorite has index %1").arg(index));
    bool found = false;
    if (Setting::instance()->showNodes()) {
        while (!found) {
            index++;

            // Don't go past the end of the list
            if (index >= mServersModel->count()) {
                return;
            }

            if (mServersModel->server(index)->favorite()) {
                found = true;
                Setting::instance()->setFavorite(index);
            }
        }
    } else {
        while (!found) {
            index++;

            // Start over if we go past the end of the list
            if (index >= mHubsModel->count())
                index = 0;

            // If we loop back on the startindex, give up
            if (index == startIndex)
                return;

            if (mHubsModel->server(index)->favorite()) {
                found = true;
                Setting::instance()->setFavorite(index);
            }
        }

    }
}

void AuthManager::previousFavorite()
{
    // Set server to previous favorite
    int startIndex = Setting::instance()->favorite();
    int index = startIndex;
    bool found = false;
    while (!found) {
        index--;

        // Don't go past the beginning of the list
        if (index < 0) {
            return;
        }

        if (Setting::instance()->showNodes()) {
            if (mServersModel->server(index)->favorite()) {
                found = true;
                Setting::instance()->setFavorite(index);
            }
        } else {
            if (mHubsModel->server(index)->favorite()) {
                found = true;
                Setting::instance()->setFavorite(index);
            }
        }
    }
}

bool AuthManager::hasNextFavorite()
{
    int currentFavorite = Setting::instance()->favorite();

    QList<int> favorites = Setting::instance()->showNodes() ? mServersModel->favoriteServers()
                                                            : mHubsModel->favoriteServers();

    int index = favorites.indexOf(currentFavorite);

    return favorites.count() - 1 > index;
}

bool AuthManager::hasPreviousFavorite()
{
    int currentFavorite = Setting::instance()->favorite();

    QList<int> favorites = Setting::instance()->showNodes() ? mServersModel->favoriteServers()
                                                            : mHubsModel->favoriteServers();

    int index = favorites.indexOf(currentFavorite);

    return index > 0;
}

void AuthManager::loginNetworkError(QNetworkReply::NetworkError error)
{
    Log::logt(QString("Login error: %1").arg(error));

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    emit loginError(QString("Network error logging in: %1").arg(reply->errorString()));
}

const QList<int> AuthManager::currentEncryptionServers()
{
    int enc = Setting::instance()->encryption();
    return mServersModel->serversForEncryption(enc);
}

const QList<int> AuthManager::currentEncryptionHubs()
{
    int enc = Setting::instance()->encryption();
    return mHubsModel->serversForEncryption(enc);
}

int AuthManager::hubidForServerNode(int srv)
{
    int hub = -1;
    if (srv > -1 && srv < mServersModel->count()) {
        AServer *rs = mServersModel->server(srv);
        QString cleared = flag::clearName(rs->name());       // QString cleared = flag::ClearName(rs.name);
        qDebug() << "looking for hub for server with cleared name " << cleared;
        if (mHubClearedId.contains(cleared)) {
            hub = mHubClearedId.value(cleared);
            qDebug() << "found hub at hub index " << hub
                     << " which has name " << mServersModel->server(hub)->name();
        } else {
            qDebug() << "No hub for cleared name " << cleared << " found ";
        }
    } else {
        Log::logt("Hub ID For Server Node " + QString::number(srv) + " requested but out of bounds");
    }
    return hub;
}

const std::vector<std::pair<bool, int> > & AuthManager::getLevel0()
{
    prepareLevels();
    return mLevel0;
}

void AuthManager::prepareLevels()
{
    Log::logt("prepareLevels called");
    // TODO: -1 special hub for boost
    if (mServersModel->count() > 0 && mLevel0.empty()) {
        const QList<int> hubs = currentEncryptionHubs();
        qDebug() << "current encryption " << Setting::instance()->encryption()
                 << " has " << hubs.size() << " hubs";
        std::set<int> hub_srvids;
        for (int k =0; k < hubs.size(); ++k) {
            int srv = hubs.at(k);  //ServerIdFromHubId(k);
            hub_srvids.insert(srv);
            std::vector<int> v;
            v.push_back(srv);
            mLevel1.insert(std::make_pair(srv, v));
        }

        const QList<int> & servers = currentEncryptionServers();
        for (int k = 0; k < servers.size(); ++k) {
            int srv = servers.at(k);
            std::set<int>::iterator it = hub_srvids.find(srv);
            if (it != hub_srvids.end()) {
                mLevel0.push_back(std::make_pair(true, *it));
            } else {
                int chub = hubidForServerNode(srv);
                if (chub > -1) {
                    std::map<int, std::vector<int> >::iterator it2 = mLevel1.find(chub);
                    if (it2 != mLevel1.end()) {
                        std::vector<int> & rv = (*it2).second;
                        rv.push_back(k);
                    }
                } else {
                    mLevel0.push_back(std::make_pair(false, k));
                }
            }
        }
    }
}

const std::vector<int> & AuthManager::getLevel1(int hub)
{
    prepareLevels();
    std::map<int, std::vector<int> >::iterator it = mLevel1.find(hub);
    if (it == mLevel1.end())
        return mFake;
    else
        return (*it).second;
}

int AuthManager::serverIxFromName(const QString & srv)
{
    int ix = -1;
    if (mServerNameToId.empty() && mServersModel->count() > 0) {
        for (int k = 0, sz = mServersModel->count(); k < sz; ++k)
            mServerNameToId.insert(SIMap::value_type(mServersModel->server(k)->name().toStdString(), k));
    }
    SIMap::iterator it = mServerNameToId.find(srv.toStdString());
    if (it != mServerNameToId.end())
        ix = (*it).second;
    return ix;
}

int AuthManager::pingFromServerIx(int srv)
{
    int pn = -1;
    if (srv > -1 && srv < mServersModel->count())
        pn = mServersModel->server(srv)->ping();
    return pn;
}

void AuthManager::clearServerLists()
{
    mLevel0.clear();
    mLevel1.clear();
    mHubToServer.clear();
    mServerNameToId.clear();
}

void AuthManager::clearReply()
{
    if (mReply != nullptr) {
        mReply->abort();
        mReply->deleteLater();
        mReply = nullptr;
    }
}

//void AuthManager::getEccServerNames()
//{
//    clearReply();
//    // https://api.proxy.sh/safejumper/get_ecc/name
//    mReply = AuthManager::instance()->mNAM.get(BuildRequest(
//                     QUrl("https://api.proxy.sh/safejumper/get_ecc/name")));
//    connect(mReply, &QNetworkReply::finished,
//            this, &AuthManager::processEccServerNamesXml);
//}

//void AuthManager::processEccServerNamesXml()
//{
//    QString message;
//    bool err = processServerNamesForEncryptionType(ENCRYPTION_ECC, message);

//    if (err)
//        Log::logt("Error getting ecc names: " + message);

//    // do not get obfs addresses: proceed
//    getAccountType();

//    if (!err) {
//        // clone ECC nodes into ECC+XOR
//        // TODO: -2 is there a specific for ECC+XOR  API page?
//        mServerIds[ENCRYPTION_ECCXOR].clear();
//        mServerIds[ENCRYPTION_ECCXOR] = mServerIds[ENCRYPTION_ECC];
//        int enc = Setting::instance()->encryption();
//        if (enc == ENCRYPTION_ECC || enc == ENCRYPTION_ECCXOR)
//            Setting::instance()->loadServer();
//    }

//    emit serverListsLoaded();
//}

//void AuthManager::getObfsServerNames()
//{
//    clearReply();
//    // https://api.proxy.sh/safejumper/get_obfs/name
//    mReply = AuthManager::instance()->mNAM.get(BuildRequest(
//                     QUrl("https://api.proxy.sh/safejumper/get_obfs/name")));
//    connect(mReply, &QNetworkReply::finished,
//            this, &AuthManager::processObfsServerNamesXml);
//}

//void AuthManager::processObfsServerNamesXml()
//{
//    QString message;
//    bool err = processServerNamesForEncryptionType(ENCRYPTION_TOR_OBFS2, message);

//    if (!err) {
//        // clone ECC nodes into ECC+XOR
//        // TODO: -2 is there a specific for ECC+XOR  API page?
//        mServerIds[ENCRYPTION_TOR_OBFS3].clear();
//        mServerIds[ENCRYPTION_TOR_OBFS3] = mServerIds[ENCRYPTION_TOR_OBFS2];
//        mServerIds[ENCRYPTION_TOR_SCRAMBLESUIT].clear();
//        mServerIds[ENCRYPTION_TOR_SCRAMBLESUIT] = mServerIds[ENCRYPTION_TOR_OBFS2];
//        int enc = Setting::instance()->encryption();
//        if (enc == ENCRYPTION_TOR_OBFS2
//                || enc == ENCRYPTION_TOR_OBFS3
//                || enc == ENCRYPTION_TOR_SCRAMBLESUIT
//           )
//            Setting::instance()->loadServer();
//    }

//    // do not get obfs addresses: proceed
//    getEccServerNames();

//    return;


//#if 0
//    bool err = false;
//    out_msg.clear();
//    if (_reply->error() != QNetworkReply::NoError) {
//        Log::logt(_reply->errorString());
//    } else {
//        QByteArray ba = _reply->readAll();

////{QFile f("/tmp/obfsname.xml");
////f.open(QIODevice::WriteOnly);
////f.write(ba);
////f.flush();
////f.close();}

//        /*
//        <?xml version="1.0"?>
//        <root>
//                <0>U.S. California 3</0>
//                <1>U.S. Georgia 1</1>

//                <130>Boost - Singapore - SoftLayer</130>
//        </root>
//        */
//        QDomDocument doc;
//        std::vector<QString> v;
//        QString w(ba);
//        if (!doc.setContent(w, &out_msg)) {
//            int p0 = w.indexOf("<root>");
//            int p1 = w.indexOf('<', p0 + 1);
//            int end = w.indexOf("</root>", p1 + 1);
//            if (end > -1) {
//                for (int t = p1; t > -1 && t < end; ) {
//                    int p2 = w.indexOf('>', t + 1);
//                    int p3 = w.indexOf("</", p2 + 1);
//                    QString internal = w.mid(p2 +1, p3 - p2 - 1);
//                    if (!internal.isEmpty())
//                        v.push_back(internal);
//                    t = w.indexOf('<', p3 + 1);
//                }
//            } else {
//                err = true;
//                out_msg = "Error parsing Obfs name XML\n" + out_msg;
//            }
//        } else {
//            QDomNodeList roots = doc.elementsByTagName("root");
//            if (roots.size() > 0) {
//                QDomNode root = roots.item(0);
//                QDomNodeList chs = root.childNodes();
//                if (!chs.isEmpty()) {
//                    for (int k = 0, sz = chs.size(); k < sz; ++k) {
//                        QDomNode n = chs.at(k);
//                        QString s = n.toElement().text();
//                        v.push_back(s);
//                    }
//                } else {
//                    err = true;
//                    out_msg = "empty obfs name list";
//                }
//            } else {
//                err = true;
//                out_msg = "Missing root node";
//            }
//        }

//        if (!err && !v.empty()) {
//            _obfs_names.swap(v);
//            MatchObfsServers();
//        }
//    }

//    // do not get obfs addresses: proceed
//    //  StartDwnl_AccType();
//    StartDwnl_EccName();

//    // force update of locations: if needed, previously empy
//    if (Setting::IsExists() && Scr_Map::IsExists()) {
//        Scr_Map::Instance()->RePopulateLocations();
//        // TODO: -0 LoadServer()
//    }
//#endif
//}

//void AuthManager::StartDwnl_EccxorName()
//{
//      ClearReply();
//      // https://api.proxy.sh/safejumper/get_ecc/name
//      _reply.reset(AuthManager::Instance()->_nam.get(BuildRequest(
//              QUrl("https://api.proxy.sh/safejumper/get_ecc/name"))));
//      SjMainWindow * sj = SjMainWindow::Instance();
//      sj->connect(_reply.get(), SIGNAL(finished()), sj, SLOT(Finished_EccxorName()));
//}

//void AuthManager::StartDwnl_ObfsAddr()
//{
//      ClearReply();
//      // https://api.proxy.sh/safejumper/get_obfs/name
//      _reply.reset(AuthManager::Instance()->_nam.get(BuildRequest(
//              QUrl("https://api.proxy.sh/safejumper/get_obfs/name"))));
//      SjMainWindow * sj = SjMainWindow::Instance();
//      sj->connect(_reply.get(), SIGNAL(finished()), sj, SLOT(AccTypeFinishedZZ()));
//}

//void AuthManager::getAccountType()
//{
//    clearReply();
//    // https://api.proxy.sh/safejumper/account_type/VPNusername/VPNpassword
//    mReply = AuthManager::instance()->mNAM.get(BuildRequest(
//                     QUrl("https://api.proxy.sh/safejumper/account_type/"
//                          + QUrl::toPercentEncoding(AuthManager::instance()->VPNName(), "", "")
//                          + "/" + QUrl::toPercentEncoding(AuthManager::instance()->VPNPassword(), "", ""))));
//    connect(mReply, &QNetworkReply::finished,
//            this, &AuthManager::processAccountTypeXml);
//}

//void AuthManager::getExpirationDate()
//{
//    clearReply();
//    // https://api.proxy.sh/safejumper/expire_date/VPNusername/VPNpassword
//    mReply = mNAM.get(BuildRequest(
//                              QUrl("https://api.proxy.sh/safejumper/expire_date/"
//                                   + QUrl::toPercentEncoding(AuthManager::instance()->VPNName(), "", "")
//                                   + "/" + QUrl::toPercentEncoding(AuthManager::instance()->VPNPassword(), "", ""))));
//    connect(mReply, &QNetworkReply::finished,
//            this, &AuthManager::processExpirationXml);
//}

void AuthManager::checkForUpdates()
{
    QString us(UPDATE_URL);
    if (!us.isEmpty()) {
        Log::logt(QString("Checking for updates from %1").arg(UPDATE_URL));
        mUpdateReply = mNAM.get(BuildRequest(QUrl(us)));
        connect(mUpdateReply, SIGNAL(finished()),
                this, SLOT(processUpdatesXml()));
    } else {
        emit noUpdateFound();
    }
}

void AuthManager::getOldIP()
{
    ++mIPAttemptCount;
    Log::logt("StartDwnl_OldIp() attempt " + QString::number(mIPAttemptCount));
    static const QString us = "https://proxy.sh/ip.php";
    mIPReply = AuthManager::instance()->mNAM.get(BuildRequest(QUrl(us)));
    connect(mIPReply, SIGNAL(finished()),
            this, SLOT(processOldIP()));
}

//void AuthManager::getDns()
//{
//    clearReply();
//    // https://api.proxy.sh/safejumper/get_dns
//    //<?xml version="1.0"?>
//    //<root>
//    //  <dns>146.185.134.104</dns>
//    //  <dns>192.241.172.159</dns>
//    //</root>
//    mReply = AuthManager::instance()->mNAM.get(BuildRequest(
//                     QUrl("https://api.proxy.sh/safejumper/get_dns")));
//    connect(mReply, &QNetworkReply::finished,
//            this, &AuthManager::processDnsXml);
//}

//void AuthManager::processAccountTypeXml()
//{
//    QString message;
//    if (mReply->error() != QNetworkReply::NoError) {
//        Log::logt(mReply->errorString());
//        return;
//    }
//    QByteArray ba = mReply->readAll();
//    if (ba.isEmpty()) {
//        Log::logt("Cannot get account info. Server response is empty.");
//        return;
//    }
//    // parse XML response

//    // <?xml version="1.0"?>
//    //<root><package>10</package><email>aaa@gmail.com</email></root>
//    QDomDocument doc;
//    if (!doc.setContent(QString(ba), &message)) {
//        Log::logt("Error parsing XML account info\n" + message);
//        return;
//    }
//    QDomNodeList nlpackage = doc.elementsByTagName("package");
//    if (nlpackage.size() <= 0) {
//        Log::logt("Missing package amount");
//        return;
//    }
//    QDomNode n = nlpackage.item(0);
//    QString amount = "$" + n.toElement().text();

//    QDomNodeList nl = doc.elementsByTagName("email");
//    if (nl.size() <= 0) {
//        Log::logt("Missing email in account XML");
//        return;
//    }
//    n = nl.item(0);
//    mEmail = n.toElement().text();
//    Log::logt("Got account e-mail " + mEmail + " and amount " + amount);
//    emit amountLoaded(amount);
//    emit emailLoaded(mEmail);
//    /*
//    QFile f("/tmp/acc.xml");
//    f.open(QIODevice::WriteOnly);
//    f.write(ba);
//    f.flush();
//    f.close();
//    */

//    getExpirationDate();
//}

//void AuthManager::processExpirationXml()
//{
//    QString message;
//    if (mReply->error() != QNetworkReply::NoError) {
//        Log::logt(mReply->errorString());
//        return;
//    }
//    QByteArray ba = mReply->readAll();
//    if (ba.isEmpty()) {
//        Log::logt("Cannot get expiration info. Server response is empty.");
//        return;
//    }
//    // parse XML response
//    /*
//    {QFile f("/tmp/expire.xml");
//    f.open(QIODevice::WriteOnly);
//    f.write(ba);
//    f.flush();
//    f.close();}
//    */
//    // <?xml version="1.0"?>
//    // <root><expire_date>2015-05-28</expire_date></root>
//    QDomDocument doc;
//    QString until = "--";
//    if (!doc.setContent(QString(ba), &message)) {
//        Log::logt("Error parsing XML expiration info\n" + message);
//        return;
//    }
//    QDomNodeList nl = doc.elementsByTagName("expire_date");
//    if (nl.size() <= 0) {
//        Log::logt("Missing expiration info");
//        return;
//    }
//    QDomNode n = nl.item(0);
//    until = n.toElement().text();

//    mExpiration = until;

//    emit untilLoaded(until);

//    getDns();
//}

//void AuthManager::processDnsXml()
//{
//    if (mReply->error() != QNetworkReply::NoError) {
//        Log::logt(mReply->errorString());
//        return;
//    }
//    QByteArray ba = mReply->readAll();
//    if (ba.isEmpty()) {
//        Log::logt("Cannot get DNS info. Server response is empty.");
//        return;
//    }
//    // parse XML response

//    // <?xml version="1.0"?>
//    // <root>
//    //  <dns>146.185.134.104</dns>
//    //  <dns>192.241.172.159</dns>
//    // </root>
//    QDomDocument doc;
//    QString msg;
//    if (!doc.setContent(QString(ba), &msg)) {
//        Log::logt("Error parsing XML DNS info\n" + msg);
//        return;
//    }
//    QDomNodeList nl = doc.elementsByTagName("dns");
//    if (nl.size() <= 0) {
//        Log::logt("Missing DNS nodes");
//        return;
//    }
//    QString dns[2];
//    for (int k = 0; k < 2 && k < nl.size(); ++k) {
//        QDomNode n = nl.item(k);
//        dns[k] = n.toElement().text();
//    }
//    if (!dns[0].isEmpty() || !dns[1].isEmpty())
//        Setting::instance()->setDefaultDNS(dns[0], dns[1]);

//    clearReply();               // TODO: -2 further processing here
//}

void AuthManager::processUpdatesXml()
{
    if (mUpdateReply->error() != QNetworkReply::NoError) {
        Log::logt(mUpdateReply->errorString());
        emit noUpdateFound();
        return;
    }
    QByteArray ba = mUpdateReply->readAll();

    /*
    QByteArray ba =
    "<?xml version=\"1.1\" encoding=\"UTF-8\"?>"
    "<version>"
      "<stable>3.0</stable>"
      "<build>24</build>"
      "<files>"
            "<file url=\"/safejumper.exe\"/>"
      "</files>"
      "<date>2015-08-05</date>"
    "</version>";
    */
    if (ba.isEmpty()) {
        Log::logt("Cannot get Updates info. Server response is empty.");
        emit noUpdateFound();
        return;
    }
    // parse XML response

    // <?xml version="1.1" encoding="UTF-8"?>
    // <version>
    //  <stable>3.0</stable>
    //  <build>23</build>
    //  <files>
    //    <file url="/safejumper.exe"/>
    //  </files>
    //  <date>2015-08-05</date>
    // </version>
    QDomDocument doc;
    QString msg;
    if (!doc.setContent(QString(ba), &msg)) {
        Log::logt("Error parsing XML Updates info\n" + msg);
        emit noUpdateFound();
        return;
    }
    QDomNodeList nl = doc.elementsByTagName("build");
    if (nl.size() <= 0) {
        Log::logt("Missing 'build' node");
        emit noUpdateFound();
        return;
    }
    QDomNode n = nl.item(0);
    QString ss = n.toElement().text();
    if (!ss.isEmpty()) {
        bool ok;
        int upd = ss.toInt(&ok);
        Log::logt(QString("Got updated xml, server version is %1, local version is %2").arg(upd).arg(APP_BUILD_NUM));
        if (ok && APP_BUILD_NUM < upd) {
            emit newVersionFound();
            Setting::instance()->updateMessageShown();
        }
    }
}

//bool AuthManager::processServerNamesForEncryptionType(int enc, QString & out_msg)
//{
//    out_msg.clear();
//    QStringList names = extractNames(out_msg);

//    if (out_msg.isEmpty() && !names.isEmpty()) {
//        populateServerIdsFromNames(names, mServerIds[enc]);
//    }

//    return !out_msg.isEmpty();
//}

//QStringList AuthManager::extractNames(QString & out_msg)
//{
//    QStringList names;
//    if (mReply->error() != QNetworkReply::NoError) {
//        Log::logt("Network error:" + mReply->errorString());
//        out_msg = "Network error: " + mReply->errorString();
//    } else {
//        QByteArray ba = mReply->readAll();

////{QFile f("/tmp/obfsname.xml");
////f.open(QIODevice::WriteOnly);
////f.write(ba);
////f.flush();
////f.close();}

//        /*
//        <?xml version="1.0"?>
//        <root>
//                <0>U.S. California 3</0>
//                <1>U.S. Georgia 1</1>

//                <130>Boost - Singapore - SoftLayer</130>
//        </root>
//        */

//        QString w(ba);

//        int p0 = w.indexOf("<root>");
//        int p1 = w.indexOf('<', p0 + 1);
//        int end = w.indexOf("</root>", p1 + 1);
//        if (end > -1) {
//            for (int t = p1; t > -1 && t < end; ) {
//                int p2 = w.indexOf('>', t + 1);
//                int p3 = w.indexOf("</", p2 + 1);
//                QString internal = w.mid(p2 +1, p3 - p2 - 1);
//                if (!internal.isEmpty())
//                    names.append(internal);
//                t = w.indexOf('<', p3 + 1);
//            }
//        } else {
//            out_msg = "Error parsing Obfs name XML\n" + out_msg;
//        }
//    }
//    return names;
//}


void AuthManager::populateServerIdsFromNames(QStringList names, QList<int> &serverList)
{
    QList<int> ids;
    for (int j = 0; j < mServersModel->count(); ++j) {
        if (names.contains(mServersModel->server(j)->name())) {
            ids.append(j);
        }
    }
    qSort(ids.begin(), ids.end());
    serverList = ids;
}

QString AuthManager::processLoginResult()
{
    QString message;
    clearServerLists();
    if (mReply->error() != QNetworkReply::NoError) {
        mLoggedIn = false;
        return mReply->errorString();
    } else {
        QByteArray ba = mReply->readAll();
        if (ba.isEmpty()) {
            mLoggedIn = false;
            return "Cannot log in with this name and password pair. Server response is empty.";
        }

        Log::logt("json response to login is " + QString(ba));

        // Parse json response
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(ba, &error);

        if (jsonDoc.isNull())
            return error.errorString();

        QJsonObject documentObject = jsonDoc.object();

        int package = documentObject.value("package").toString().toInt();
        QDate expiration = QDate::fromString(documentObject.value("expire_date").toString(), "yyyy-MM-dd");

        if (documentObject.contains("email")) {
            mEmail = documentObject.value("email").toString();
            Log::logt(QString("User e-mail address is %1").arg(mEmail));
            emit emailLoaded(mEmail);
        }

        Log::logt(QString("User package is %1").arg(package));
        Log::logt(QString("User expiration is %1").arg(expiration.toString()));

        if (expiration.daysTo(QDate::currentDate()) > 0)
            return tr("The account has expired");

        mSubscription = tr("$%1 Package").arg(package);
        emit subscriptionChanged();

        mLoggedIn = true;
        emit loggedInChanged();

        mExpiration = expiration.toString();
        emit untilLoaded(mExpiration);

        // Now load user's server list and hubs list
        getServerList();
    }

    return message;
}

void AuthManager::pingAllServers(bool hubs)
{
    Log::logt("pingAllServers called");
    if (hubs)
        for (int k = 0; k < mHubsModel->count(); ++k)
            mHubToPing.push(k);
    else
        for (int k = 0; k < mServersModel->count(); ++k)
            mToPing.push(k);

    if (mWorkers.empty()) {
        for (size_t k = 0; k < PINGWORKERS_NUM; ++k) {
            mWorkers.push_back(nullptr);
            mWaiters.push_back(new PingWaiter(k, this));
            mTimers.push_back(new QTimer());
        }
    }

    mPingsLoaded = false;
    for (size_t k = 0; k < mWorkers.size() && (!mToPing.empty() || !mHubToPing.empty()); ++k)
        startWorker(k);
}

void AuthManager::startWorker(size_t id)
{
    Log::logt("startWorker called with id " + QString::number(id));
    if (!mToPing.empty()) {
        int srv = mToPing.front();
        mToPing.pop();
        if (mServersModel->server(srv) == nullptr) {
            return ;
        }
        Log::logt("startWorker will ping server number " + QString::number(srv));

        if (mWorkers.at(id) != nullptr) {
            Log::logt("Workers at " + QString::number(id) + " not null, so disconnecting and terminating");
            disconnect(mWorkers.at(id), static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                       mWaiters.at(id), &PingWaiter::PingFinished);
            disconnect(mWorkers.at(id), &QProcess::errorOccurred,
                       mWaiters.at(id), &PingWaiter::PingError);
            if (mWorkers.at(id)->state() != QProcess::NotRunning) {
                mWorkers.at(id)->terminate();
                mWorkers.at(id)->deleteLater();
            }
        }
        mWorkers[id] = new QProcess(this);
        mWorkers[id]->setProperty(kHubIndex, -1);
        mWorkers[id]->setProperty(kServerIndex, srv);
        connect(mWorkers.at(id), static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                mWaiters.at(id), &PingWaiter::PingFinished);
        connect(mWorkers.at(id), &QProcess::errorOccurred,
                mWaiters.at(id), &PingWaiter::PingError);
        startPing(*mWorkers.at(id), mServersModel->server(srv)->address());

        connect(mTimers.at(id), &QTimer::timeout,
                mWaiters.at(id), &PingWaiter::Timer_Terminate);
        mTimers.at(id)->setSingleShot(true);
        mTimers.at(id)->start(PINGWORKER_MAX_TIMEOUT);
    } else if (!mHubToPing.empty()) {
        int hub = -1;
        do {
            int hubToCheck = mHubToPing.front();
            mHubToPing.pop();
            // TODO : why mHubToPing contains hubs which are not present in mHubsModel!?!?!
            if (mHubsModel->server(hub) != nullptr) {
                hub = hubToCheck;
            }
        } while (hub != -1 || !mHubToPing.empty());

        if (hub == -1) {
            mPingsLoaded = true;
            return;
        }

        Log::logt("startWorker will ping hub number " + QString::number(hub));

        if (mWorkers.at(id) != nullptr) {
            Log::logt("Workers at " + QString::number(id) + " not null, so disconnecting and terminating");
            disconnect(mWorkers.at(id), static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                       mWaiters.at(id), &PingWaiter::PingFinished);
            disconnect(mWorkers.at(id), &QProcess::errorOccurred,
                       mWaiters.at(id), &PingWaiter::PingError);
            if (mWorkers.at(id)->state() != QProcess::NotRunning) {
                mWorkers.at(id)->terminate();
                mWorkers.at(id)->deleteLater();
            }
        }
        mWorkers[id] = new QProcess(this);
        mWorkers[id]->setProperty(kHubIndex, hub);
        mWorkers[id]->setProperty(kServerIndex, -1);
        connect(mWorkers.at(id), static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                mWaiters.at(id), &PingWaiter::PingFinished);
        connect(mWorkers.at(id), &QProcess::errorOccurred,
                mWaiters.at(id), &PingWaiter::PingError);
        startPing(*mWorkers.at(id), mHubsModel->server(hub)->address());

        connect(mTimers.at(id), &QTimer::timeout,
                mWaiters.at(id), &PingWaiter::Timer_Terminate);
        mTimers.at(id)->setSingleShot(true);
        mTimers.at(id)->start(PINGWORKER_MAX_TIMEOUT);
    } else {
        mPingsLoaded = true;
    }
}

void AuthManager::pingComplete(size_t idWaiter)
{
    Log::logt("pingComplete called with id " + QString::number(idWaiter));
    mTimers.at(idWaiter)->stop();
    int p = extractPing(*mWorkers.at(idWaiter));
    int hubIndex = mWorkers[idWaiter]->property(kHubIndex).toInt();
    int serverIndex = mWorkers[idWaiter]->property(kServerIndex).toInt();
//      Log::logt(_servers.at(_inprogress.at(idWaiter)).address + " Got ping " + QString::number(p));

    if (hubIndex >= 0) {
        mHubsModel->setPing(hubIndex, p);
    } else if (serverIndex >= 0) {
        mServersModel->setPing(serverIndex, p);
    } else {
        Log::logt("No hub or server index set for ping somehow, ignoring result");
    }
    startWorker(idWaiter);
}

void AuthManager::pingError(size_t idWaiter)
{
    Log::logt("pingError called with id " + QString::number(idWaiter));
    mTimers.at(idWaiter)->stop();
    int p = extractPing(*mWorkers.at(idWaiter));
    int hubIndex = mWorkers[idWaiter]->property(kHubIndex).toInt();
    int serverIndex = mWorkers[idWaiter]->property(kServerIndex).toInt();

    if (hubIndex >= 0) {
        mHubsModel->setPing(hubIndex, p);
    } else if (serverIndex >= 0) {
        mServersModel->setPing(serverIndex, p);
    } else {
        Log::logt("No hub or server index set for ping somehow, ignoring result");
    }
    startWorker(idWaiter);
}

void AuthManager::pingTerminated(size_t idWaiter)
{
    Log::logt("pingTerminated called with id " + QString::number(idWaiter));
    mWorkers.at(idWaiter)->terminate();
    int p = extractPing(*mWorkers.at(idWaiter));
//      Log::logt(_servers.at(_inprogress.at(idWaiter)).address + " ping process terminated, extracted ping: " + QString::number(p));
    int hubIndex = mWorkers[idWaiter]->property(kHubIndex).toInt();
    int serverIndex = mWorkers[idWaiter]->property(kServerIndex).toInt();
//      Log::logt(_servers.at(_inprogress.at(idWaiter)).address + " Got ping " + QString::number(p));

    if (hubIndex >= 0) {
        mHubsModel->setPing(hubIndex, p);
    } else if (serverIndex >= 0) {
        mServersModel->setPing(serverIndex, p);
    } else {
        Log::logt("No hub or server index set for ping somehow, ignoring result");
    }
    startWorker(idWaiter);
}

std::vector<int> AuthManager::getPings(const std::vector<int> & toping)
{
    std::vector<int> v;
    v.assign(toping.size(), -1);
    for (size_t k = 0; k < toping.size(); ++k) {
        if (Setting::instance()->showNodes()) {
            if (toping.at(k) >= mServersModel->count())
                Log::logt("GetPings(): Server id greater than size of pings coll");
            else
                v.at(k) = mServersModel->server(toping.at(k))->ping();
        } else {
            if (toping.at(k) >= mHubsModel->count())
                Log::logt("GetPings(): Server id greater than size of pings coll");
            else
                v.at(k) = mHubsModel->server(toping.at(k))->ping();
        }
    }
    return v;
}

typedef std::pair<int, size_t> IUPair;
typedef std::vector<IUPair> IUVec;
static bool PCmp(const IUPair & a, const IUPair & b)
{
    return a.first < b.first;
}

int AuthManager::getServerToJump()
{
    Log::logt("getServerToJump called");
    if (mServersModel->count() == 0) {
        Log::logt("Server list not loaded, so using -1");
        return -1;
    }
    int srv = -1;
    int prev = Setting::instance()->serverID();
    Log::logt("Previous server is " + QString::number(prev));
    std::vector<int> toping;     // ix inside mServers
    int enc = Setting::instance()->encryption();
    if (Setting::instance()->showNodes()) {
        Log::logt("showNodes is set, so getting pings of all servers");
        // jump to server
        QList<int> servers = mServersModel->serversForEncryption(enc);
        for (int k = 0; k < servers.size(); ++k) {
            if (servers.at(k) != prev)
                toping.push_back(servers.at(k));
        }
    } else {
        // jump to hub
        Log::logt("showNodes is not set, so getting pings of hubs");
        Log::logt("prevhub is " + QString::number(prev));
        Log::logt("Looping through " + QString::number(mHubsModel->count()) + " hubs");
        for (int k = 0; k < mHubsModel->count(); ++k) {
            if (prev < 0)
                toping.push_back(k);
            else {
                if (prev != k)
                    toping.push_back(k);
            }
        }
    }

    Log::logt("getServerToJump pings list is " + QString::number(toping.size()));

    std::vector<int> pings = getPings(toping);      // from cache; do not wait for pings; return vec of the same size

    IUVec ping_ix;
    for (size_t k = 0; k < toping.size(); ++k) {
        if (pings.at(k) > -1)
            ping_ix.push_back(IUPair(pings.at(k), toping.at(k)));
    }

    if (!ping_ix.empty()) {
        std::sort(ping_ix.begin(), ping_ix.end(), PCmp);
        unsigned int num = Setting::instance()->showNodes() ? 20 : 6;      // pick this many from the top
        if (num >= ping_ix.size())
            num = ping_ix.size();
        unsigned int offset = rand() % num;
        srv = ping_ix.at(offset).second;
    }

    // pings can be unavailable
    if (srv < 0) {
        // just pick random
        if (!toping.empty()) {
            srv = toping.at(rand() % toping.size());
        } else {
            if (Setting::instance()->showNodes()) {
                if (mServersModel->count() > 0)
                    srv = rand() % mServersModel->count();
            } else {
                if (mHubsModel->count() > 0) {
                    srv = rand() % mHubsModel->count();
                } else {
                    srv = 0; // We should always have one hub
                }
            }
        }
    }
//Log::logt("SrvToJump() returns " + QString::number(srv));
    return srv;
}

void AuthManager::jump()
{
    // TODO: -2 update lists
    int srv = getServerToJump();              // except current srv/hub
    if (srv > -1) {
// TODO: -0             SetNewIp("");
        Setting::instance()->setServer(srv);
        VPNServiceManager::instance()->sendConnectToVPNRequest();               // contains stop
    }
}

void AuthManager::processOldIP()
{
    Log::logt("ProcessOldIpHttp() attempt " + QString::number(mIPAttemptCount));
    QString ip;
    bool err = true;
    if (mIPReply->error() != QNetworkReply::NoError) {
        Log::logt(mIPReply->errorString());
    } else {
        QByteArray ba = mIPReply->readAll();
        if (ba.isEmpty()) {
            Log::logt("Cannot get old IP address. Server response is empty.");
        } else {
            QString s(ba);
            int p[3];
            int t = 0;
            bool ok = true;
            for (size_t k = 0; k < 3; ++k) {
                p[k] = s.indexOf('.', t);
                if (p[k] < 0) {
                    ok = false;
                    break;
                }
                t = p[k] + 1;
            }
            if (ok) {
                if (s.length() >= QString("2.2.2.2").length()
                        && s.length() <= QString("123.123.123.123").length()) {
                    ip = s;
                    err = false;
                }
            }
        }
    }

    if (err) {
        Log::logt("ProcessOldIpHttp() attempt " + QString::number(mIPAttemptCount) + " fails");
        if (mIPAttemptCount < 4)
            getOldIP();
        else
            Log::logt("ProcessOldIpHttp() conceide at attempt " + QString::number(mIPAttemptCount));
    } else {
        Log::logt("Determined old IP:  " + ip);
        mOldIP = ip;
        // try to push value (if Scr_Connect was constructed yet)
        emit oldIpLoaded(mOldIP);
    }
}

// TODO: Call this after reaching connected state
void AuthManager::forwardPorts()
{
    UVec ports = Setting::instance()->forwardPorts();
    if (!ports.empty()) {
        mPortForwarderThread.reset(new PortForwarder(ports, mNAM, mAccountLogin, mAccountPassword));
        mPortForwarderThread->StartFirst();
        for (size_t k = 0, sz = ports.size(); k < sz; ++k) {

            ;
        }
    }
}

ServersModel *AuthManager::serversModel() const
{
    return mServersModel;
}

int AuthManager::favoritesCount()
{
    return Setting::instance()->showNodes() ? mServersModel->favoriteServers().count()
                                            : mHubsModel->favoriteServers().count();
}

ServersModel *AuthManager::hubsModel() const
{
    return mHubsModel;
}

bool AuthManager::isServerListLoaded() const
{
    return mServersModel->count() > 0 && mHubsModel->count() > 0;
}


void AuthManager::loginFinished()
{
    QString message = processLoginResult();
    Log::logt("loginFinished called message is " + message);
    if (message.isEmpty()) {
        VPNServiceManager::instance()->sendCredentials();
    } else {
        // Login failed, so get default server list instead
        //getDefaultServerList();
        emit loginError(message);
    }
}

void AuthManager::createAccountError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
}

void AuthManager::createAccountFinished()
{
}

void AuthManager::fetchServerListError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
    Log::logt("Error fetching server list, retrying in 5 seconds");
    QTimer::singleShot(5000, this, &AuthManager::getServerList);
}

void AuthManager::fetchServerListFinished()
{
    if (mServerListReply->error() != QNetworkReply::NoError) {
        Log::logt("Error fetching default server list");
    } else {
        QByteArray ba = mServerListReply->readAll();
        if (ba.isEmpty()) {
            Log::logt("server list is empty, trying again in 5 seconds");
            QTimer::singleShot(5000, this, &AuthManager::getServerList);
            return;
        }

        Log::logt("json response to server list:");
        Log::logt(QString::fromLatin1(ba));

        // Parse json response
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(ba, &error);

        if (jsonDoc.isNull()) {
            Log::logt("json document from server list is invalid, trying again in 5 seconds: error: " + error.errorString());
            QTimer::singleShot(5000, this, &AuthManager::getServerList);
            return;
        }

        QJsonArray serversArray = jsonDoc.array();

        mServersModel->updateServers(serversArray);

        qDebug() << "Populating hubcleared list";
        mHubClearedId.clear();

        for (int i = 0; i < mServersModel->count(); ++i) {
            AServer *hub = mServersModel->server(i);
            if (hub->name().contains("Hub")) {
                QString clearName = flag::clearName(hub->name());
                qDebug() << "Adding hub cleared name for hub " << hub->name() << " with id " << hub->id() << " and cleared name " << clearName;
                mHubClearedId.insert(clearName, hub->id());
            }
        }

        if (mServersModel->count() > 0 && mHubsModel->count() > 0)
            emit serverListsLoaded();
    }

    getHubs();

    if (!Setting::instance()->testing() && Setting::instance()->pingEnabled())
        pingAllServers(false); // No need to ping servers when in testing mode
}

void AuthManager::fetchHubsError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
    Log::logt("Error fetching hub list, retrying in 5 seconds");
    QTimer::singleShot(5000, this, &AuthManager::getHubs);
}

void AuthManager::fetchHubsFinished()
{
    if (mHubsReply->error() != QNetworkReply::NoError) {
        Log::logt("Error fetching hubs list");
    } else {
        QByteArray ba = mHubsReply->readAll();
        if (ba.isEmpty()) {
            Log::logt("hubs list is empty, trying again in 5 seconds");
            QTimer::singleShot(5000, this, &AuthManager::getHubs);
            return;
        }

        Log::logt("json response to hubs list is " + QString(ba));

        // Parse json response
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(ba, &error);

        if (jsonDoc.isNull()) {
            Log::logt("json document from hubs list is invalid, trying again in 5 seconds: error: " + error.errorString());
            QTimer::singleShot(5000, this, &AuthManager::getHubs);
            return;
        }

        QJsonArray serversArray = jsonDoc.array();

        mHubsModel->updateServers(serversArray);
//        Log::logt("Appending hub data to server list");
//        mServersModel->appendServers(serversArray);

        emit hubsLoaded();

        if (mServersModel->count() > 0 && mHubsModel->count() > 0)
            emit serverListsLoaded();
    }

    if (!Setting::instance()->testing() && Setting::instance()->pingEnabled())
        pingAllServers(true); // No need to ping servers when in testing mode
}

void AuthManager::startPing(QProcess & pr, const QString & adr)
{
    pr.start(pingCommand(), formatArguments(adr));
}

int AuthManager::extractPing(QProcess & pr)
{
    int ping = -1;
    QByteArray ba = pr.readAllStandardOutput();
    QString s(ba);
    QStringList out = s.split("\n", QString::SkipEmptyParts);
    if (!out.isEmpty()) {
        const QString & sp = out.at(out.size() - 1).trimmed();	// last line
#ifndef Q_OS_WIN
        if (sp.indexOf("min/avg/max") > -1) {
            int e = sp.indexOf('=');
            int slash = sp.indexOf('/', e +1);
            int sl1 = sp.indexOf('/', slash +1);
            if (sl1 > -1) {
                QString sv = sp.mid(slash + 1, sl1 - slash - 1);
                bool ok;
                double d = sv.toDouble(&ok);
                if (ok)
                    ping = (int)d;
            }
        }
#else
        int a;
        if ((a = sp.indexOf("Average =")) > -1) {
            int e = sp.indexOf('=', a);
            if (e > -1) {
                QString val = sp.mid(e + 1, sp.length() - (e + 1 + 2));
                bool ok;
                int p = val.toInt(&ok);
                if (ok)
                    ping = p;
            }

        }
#endif
    }
    return ping;
}

QStringList AuthManager::formatArguments(const QString & adr)
{
    QStringList args;
    args
#ifndef Q_OS_WIN
            << "-c" << "1"		// one packet - Mac, Linux
#ifdef Q_OS_LINUX
            << "-w" << "1"		// 1s deadline - Linux
#endif
#ifdef Q_OS_DARWIN
            << "-t" << "1"		// 1s timeout - Mac
#endif
#else
            << "-n" << "1"		// one packet - Windows
            << "-w"	<< "1200"	// timeout in ms
#endif
            << adr
            ;
    return args;
}

const QString & AuthManager::pingCommand()
{
#ifdef  Q_OS_DARWIN
    static const QString cmd = "/sbin/ping";
#else
#ifdef  Q_OS_LINUX
    static const QString cmd = "/bin/ping";
#else
    static const QString cmd = "ping";
#endif
#endif
    return cmd;
}
