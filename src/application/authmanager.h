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

#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <memory>
#include <queue>
#include <stdint.h>

#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QString>
#include <QUrl>

#include "common.h"
#include "pingwaiter.h"
#include "portforwarder.h"
#include "serversmodel.h"

#define PINGWORKERS_NUM 2
#define PINGWORKER_MAX_TIMEOUT 2000

#ifndef uint64_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef WIN32
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <time.h>
#include <errno.h>

#endif		// #ifndef uint64_t

class AuthManager: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
    Q_PROPERTY(QString email READ email NOTIFY emailLoaded)
    Q_PROPERTY(QString subscription READ subscription NOTIFY subscriptionChanged)
    Q_PROPERTY(QString expiration READ expiration NOTIFY untilLoaded)

public:
    static AuthManager * instance();
    static bool exists();
    static void cleanup();

    ~AuthManager();

    bool loggedIn();

    void cancel();

    const QString & accountName();
    const QString & accountPassword();
    const QString & email();
    const QString & subscription();
    const QString & expiration();

    const QString & newIP();
    const QString & oldIP();

    const QList<int> currentEncryptionServers();		// return IDs of servers inside _servers available for this encryption
    const QList<int> currentEncryptionHubs();					// return IDs of habs inside _servers

    const std::vector<std::pair<bool, int> > & getLevel0();		// <is hub, hub id / srv id>
    const std::vector<int> & getLevel1(int hub);					// for given hub id all the server ids, including hub entry itself

    AServer *getServer(int id, bool forceServer = false);	 // on -1 returns nullptr
    int serverIxFromName(const QString & srv);		 // -1 if not found
    AServer *getHub(int id);

    int pingFromServerIx(int srv);

    void jump();

    Q_INVOKABLE void checkForUpdates();		// use own reply; can download in parallel with others; executed by main window at start regardless other actions
    void getOldIP();

    void pingComplete(size_t idWaiter);
    void pingError(size_t idWaiter);
    void pingTerminated(size_t idWaiter);

    int getServerToJump();		// except current Scr_Map::Instance()->CurrSrv()

    void forwardPorts();

    ServersModel *serversModel() const;

    // How many servers are in the current favorites list
    int favoritesCount();
    ServersModel *hubsModel() const;

    bool isServerListLoaded() const;

public slots:
    void login(const QString & name, const QString & password);
    void logout();

    void createAccount(const QString &email, const QString &name, const QString &password);

    void setNewIp(const QString & ip);

    void getServerList();
    void getHubs();

    Q_INVOKABLE void nextFavorite();
    Q_INVOKABLE void previousFavorite();

    Q_INVOKABLE bool hasNextFavorite();
    Q_INVOKABLE bool hasPreviousFavorite();

signals:
    void loginError(QString message);
    void logoutCompleted();

    void loggedInChanged();

    // Emitted when all server lists have been loaded
    void serverListsLoaded();
    void hubsLoaded();

    void untilLoaded(QString until);
    void amountLoaded(QString amount);
    void emailLoaded(QString email);
    void oldIpLoaded(QString oldIp);
    void newIpLoaded(QString newIp);
    void subscriptionChanged();

    void newVersionFound();
    void noUpdateFound();

private slots:
    void loginNetworkError(QNetworkReply::NetworkError error);
    void loginFinished();
    void autoLog();

    void createAccountError(QNetworkReply::NetworkError error);
    void createAccountFinished();

    void fetchServerListError(QNetworkReply::NetworkError error);
    void fetchServerListFinished();

    void fetchHubsError(QNetworkReply::NetworkError error);
    void fetchHubsFinished();

//    void processObfsServerNamesXml();
//    void processEccServerNamesXml();
//    void processAccountTypeXml();
//    void processExpirationXml();
//    void processDnsXml();

    void processUpdatesXml();
    void processOldIP();

private:
    AuthManager();

    QString processLoginResult();	// true = ok, empty msg on ok

//    bool processServerNamesForEncryptionType(int enc, QString & out_msg);
    void populateServerIdsFromNames(QStringList names, QList<int> &serverList);		// for _obfs_names lookup respective server ix in _servers
//    QStringList extractNames(QString & out_msg);
    void pingAllServers(bool hubs = false);

    void startPing(QProcess & pr, const QString & adr);		// pr must have already connected finished() signal
    int extractPing(QProcess & pr);		// exract ping value from pr's stdout; -1 on error / unavailable

    const QString & pingCommand();			// both for StartPing()
    QStringList formatArguments(const QString & adr);

    bool mLoggedIn;
    bool mCancellingLogin;
    static std::auto_ptr<AuthManager> mInstance;

    void clearServerLists();

    std::vector<std::pair<bool, int> > mLevel0;		// <is hub, hub id / srv id>
    std::map<int, std::vector<int> > mLevel1;		// <hub id, <srv ids, including srv id of hub entry> >
    std::vector<int> mFake;
    void prepareLevels();
    int hubidForServerNode(int srv);					// -1 if cannot find hub for this srv
    QMap<QString, int> mHubClearedId;	//QMap<QString, size_t> _HubClearedId;		// <hub cleared name (w/o ' Hub'), its hub id>
    std::vector<size_t> mHubToServer;	   // id of hub inside _servers
    SIMap mServerNameToId;

    QString mAccountLogin;
    QString mAccountPassword;

    QString mNewIP;
    QString mOldIP;
    QString mAccountName;
    QString mEmail;
    QString mAmount;
    QString mExpiration;
    QString mSubscription;

    QNetworkAccessManager mNAM;
    QPointer<QNetworkReply> mReply;
    QPointer<QNetworkReply> mIPReply;
    QPointer<QNetworkReply> mCreateAccountReply;
    QPointer<QNetworkReply> mServerListReply;
    QPointer<QNetworkReply> mHubsReply;
    int mIPAttemptCount;
    void clearReply();
    QPointer<QNetworkReply> mUpdateReply;

    std::vector<int> getPings(const std::vector<int> &toping);	// from _pings; do not wait for pings; return vec of the same size
    std::queue<int> mToPing;				// id inside _servers
    std::queue<int> mHubToPing;
    bool mPingsLoaded;

    std::vector<QProcess *> mWorkers;		// 3 vectors of the same size WORKERS_NUM
    std::vector<PingWaiter *> mWaiters;
    std::vector<QTimer *>  mTimers;
    void startWorker(size_t id);

    void getAccountType();		// TODO: -2 chain in a more flexible way e.g. queue
    void getExpirationDate();
    void getDns();
//    void getObfsServerNames();
//    void getEccServerNames();

    std::auto_ptr<PortForwarder> mPortForwarderThread;

    ServersModel *mServersModel;
    ServersModel *mHubsModel;
};

#endif // AUTHMANAGER_H
