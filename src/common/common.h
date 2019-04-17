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

#ifndef COMMON_H
#define COMMON_H

#include <QHash>
#include <QString>
#include <QSettings>
#include <QNetworkRequest>

#include <map>
#include <set>
#include <string>
#include <vector>

#define kHELPER_LABEL "sh.proxy.SafejumperHelper"

const QString kAppName = "Safejumper";
const QString kOrgName = "sh.proxy";
const QString kLowerAppName = "safejumper";

#define APP_BUILD_NUM 102
#define APP_BUILD "102"
#define APP_VERSION "5.0"

#define APPLICATION_VERSION QObject::tr("Version %1 (build %2)").arg(APP_VERSION).arg(APP_BUILD)

#ifdef Q_OS_DARWIN
#define UPDATE_URL "https://proxy.sh/safejumper_mac.xml"
#define OS_STRING "Mac"
#endif

#ifdef Q_OS_WIN
#define UPDATE_URL "https://proxy.sh/safejumper_windows.xml"
#define OS_STRING "Windows"
#endif

#ifdef Q_OS_LINUX
#define UPDATE_URL "https://proxy.sh/safejumper_linux.xml"
#define OS_STRING "Linux"
#endif

const QString kServersUrl = "https://api.proxy.sh/v2/public/servers";
const QString kUserServersUrl = "https://api.proxy.sh/v2/public/access";
const QString kHubsUrl = "https://api.proxy.sh/v2/public/hubs";
const QString kUserHubsUrl = "https://api.proxy.sh/v2/public/access/hubs";
const QString kLoginUrl = "https://api.proxy.sh/v2/public/account_info";

const QString kCreateAccountUrl = "https://proxy.sh/v1/user/add";
const QString kOvpnUrl = "https://api.proxy.sh/v2/public/config";

const QString kNotConnectedFill = "#7a869a";
const QString kConnectingFill = "#ffab00";
const QString kConnectedFill = "#2cc532";

bool IsValidIp(const QString & ip);
bool IsValidPort(const QString & s);

// IMPORTANT: Never add languages in the middle, always add them to the end
// if sorting is needed in gui, sort in the gui or code, not here since
// user's selected language is stored in settings by number
enum language {
    languageEnglish,
    languageSimplifiedChinese
};

#define FIRST_LANGUAGE languageEnglish
#define LAST_LANGUAGE languageSimplifiedChinese

// This list is reloaded in setting::setLanguage so the translated strings
// will be loaded when language is initialized/changed
static QStringList kLanguageNames = {
    QObject::tr("English"),
    QObject::tr("Simplified Chinese")
};

const QStringList kLanguageTranslations = {
#ifdef Q_OS_WIN
    "gui_en.qm",
    "gui_zh.qm"
#else
#ifdef Q_OS_DARWIN
    "/Applications/Safejumper.app/Contents/Resources/gui_en.qm",
    "/Applications/Safejumper.app/Contents/Resources/gui_zh.qm"
#else
    "/opt/safejumper/gui_en.qm",
    "/opt/safejumper/gui_zh.qm"
#endif
#endif
};

enum vpnState {
    vpnStateDisconnected = 0,
    vpnStateConnecting,
    vpnStateConnected,
    vpnStateTotal,
    vpnStateDisconnecting
};

QString vpnStateWord(vpnState state);

// Only use commands for start and stop. all settings such as server choice,
// protocol choice, credentials use QSettings.
enum commands {
    cmdGetStatus,
    cmdStart,
    cmdStop,
    cmdKillRunningOpenvpn, // Kill openvpn processes that are running
    cmdSetCredentials, // Set vpn username and password. performed once after connecting from gui to service
    cmdNetdown, // Turn off network devices because of kill switch
    notifyStatusChange,
    notifyStatusWord,
    notifyTimeout, // Openvpn timed out, so switch ports or nodes and try again
    notifyError, // Error message from service
    notifyGotIP, // Got new ip address from openvpn management socket
};

#ifdef Q_OS_WIN
static const QString kSocketName = "SafejumperVPN";
#else
static const QString kSocketName = "/var/tmp/SafejumperVPN";
#endif

enum EncryptionType {
    ENCRYPTION_RSA = 0,
    ENCRYPTION_TOR_OBFS2,
    ENCRYPTION_TOR_OBFS3,
    ENCRYPTION_TOR_SCRAMBLESUIT,
    ENCRYPTION_ECC,
    ENCRYPTION_ECCXOR,
//    ENCRYPTION_TLSCRYPT,
//    ENCRYPTION_TLSCRYPT_XOR,
    ENCRYPTION_COUNT
};

#define DEFAULT_ENCRYPTION ENCRYPTION_TLSCRYPT

const QList<QString> encryptionNames = {
    "RSA 4096-bit",
    "RSA + TOR (obfs2)",
    "RSA + TOR (obfs3)",
    "RSA + TOR (scramblesuit)",
    "ECC (secp384r1)",
    "ECC + XOR",
};

enum OpenVPNStateWord {
    ovnStateConnecting,
    ovnStateTCPConnecting,
    ovnStateWait,
    ovnStateExiting,
    ovnStateReconnecting,
    ovnStateAuth,
    ovnStateGetConfig,
    ovnStateAssignIP,
    ovnStateResolve,
    ovnStateUnknown, // Unknown state word from openvpn
};

#define PORT_FORWARD_MAX 5

bool IsValidIp(const QString & ip);
bool IsValidPort(const QString & s);

bool OpenUrl(const char * url);
bool OpenUrl_Support();
bool OpenUrl_Panel();
bool OpenUrl_Earn();
bool OpenUrl_Bug();

//void SaveCb(const char * name, bool val);

// typedef QHash<QString, size_t>  HMSI;

typedef std::map<std::string, int> SIMap;
typedef std::map<int, int> IIMap;

typedef std::vector<unsigned int> UVec;
typedef std::set<unsigned int> USet;

// escape \ and "
QString EscapePsw(const QString & raw);

QNetworkRequest BuildRequest(const QUrl & u);

#endif // COMMON_H
