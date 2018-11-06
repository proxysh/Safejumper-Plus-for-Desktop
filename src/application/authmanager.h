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
#include <winsock2.h>
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
    const QString & VPNName();
    const QString & VPNPassword();
    const QString & email();
    const QString & subscription();
    const QString & expiration();

    const QString & newIP();
    const QString & oldIP();

    const QList<int> & currentEncryptionServers();		// return IDs of servers inside _servers available for this encryption
    const QList<int> & currentEncryptionHubs();					// return IDs of habs inside _servers

    const std::vector<std::pair<bool, int> > & getLevel0();		// <is hub, hub id / srv id>
    const std::vector<int> & getLevel1(size_t hub);					// for given hub id all the server ids, including hub entry itself

    size_t serverIdFromHubId(size_t ixHub);
    AServer *getServer(int id);	 // on -1 returns nullptr
    int hubIxFromServerName(const QString & srv);	  // -1 if not found
    int hubIdFromServerId(int ixsrv);			   // -1 if ixsrv not a hub
    int serverIxFromName(const QString & srv);		 // -1 if not found
    AServer *getHub(int idhub);

    int pingFromServerIx(int srv);

    void jump();

    uint64_t getRandom64();

    void checkUpdates();		// use own reply; can download in parallel with others; executed by main window at start regardless other actions
    void getOldIP();

    void pingComplete(size_t idWaiter);
    void pingError(size_t idWaiter);
    void pingTerminated(size_t idWaiter);

    int getServerToJump();		// except current Scr_Map::Instance()->CurrSrv()

    void forwardPorts();

    ServersModel *serversModel() const;

public slots:
    void login(const QString & name, const QString & password);
    void logout();

    void createAccount(const QString &email, const QString &name, const QString &password);

    void setNewIp(const QString & ip);

signals:
    void loginError(QString message);
    void logoutCompleted();

    void loggedInChanged();

    // Emitted when all server lists have been loaded
    void serverListsLoaded();

    void untilLoaded(QString until);
    void amountLoaded(QString amount);
    void emailLoaded(QString email);
    void oldIpLoaded(QString oldIp);
    void newIpLoaded(QString newIp);
    void subscriptionChanged();

    void confirmation(const QString &message);

private slots:
    void loginNetworkError(QNetworkReply::NetworkError error);
    void loginFinished();

    void createAccountError(QNetworkReply::NetworkError error);
    void createAccountFinished();
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
    void pingAllServers();

    void startPing(QProcess & pr, const QString & adr);		// pr must have already connected finished() signal
    int extractPing(QProcess & pr);		// exract ping value from pr's stdout; -1 on error / unavailable

    const QString & pingCommand();			// both for StartPing()
    QStringList formatArguments(const QString & adr);

    bool mLoggedIn;
    bool mCancellingLogin;
    static std::auto_ptr<AuthManager> mInstance;

    void clearServerLists();
    bool mSeeded;
    void seed();

    QList<int> mServerIds[ENCRYPTION_COUNT];		// IDs inside _servers available for each encryption
    QList<AServer*> mHubs;
    QList<int> mHubIds[ENCRYPTION_COUNT];		// IDs of hubs inside _servers available for each encryption		// _hub_ids[0] the same as _HubToServer
    std::vector<std::pair<bool, int> > mLevel0;		// <is hub, hub id / srv id>
    std::map<int, std::vector<int> > mLevel1;		// <hub id, <srv ids, including srv id of hub entry> >
    std::vector<int> mFake;
    void prepareLevels();
    int hubidForServerNode(int srv);					// -1 if cannot find hub for this srv
    std::map<std::string, size_t> mHubClearedId;	//std::map<QString, size_t> _HubClearedId;		// <hub cleared name (w/o ' Hub'), its hub id>
    std::vector<size_t> mHubToServer;	   // id of hub inside _servers
    IIMap mServerIdToHubId;
    SIMap mServerNameToId;

    QString mAccountLogin;
    QString mAccountPassword;
    QString mVPNLogin;
    QString mVPNPassword;

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
    int mIPAttemptCount;
    void clearReply();
    QPointer<QNetworkReply> mUpdateReply;

    QList<size_t> mPings;		// ping for each server; -1 on err
    std::vector<int> getPings(const std::vector<size_t> & toping);	// from _pings; do not wait for pings; return vec of the same size
    std::queue<size_t> mToPing;				// id inside _servers
    bool mPingsLoaded;

    std::vector<QProcess *> mWorkers;		// 3 vectors of the same size WORKERS_NUM
    std::vector<PingWaiter *> mWaiters;
    std::vector<size_t> mInProgress;			// id inside _servers
    std::vector<QTimer *>  mTimers;
    void startWorker(size_t id);

    void getAccountType();		// TODO: -2 chain in a more flexible way e.g. queue
    void getExpirationDate();
    void getDns();
//    void getObfsServerNames();
//    void getEccServerNames();

    std::auto_ptr<PortForwarder> mPortForwarderThread;

    ServersModel *mServersModel;
};

#endif // AUTHMANAGER_H
