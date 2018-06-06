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

#ifndef SETTING_H
#define SETTING_H

#include <memory>
#include <vector>
#include <QFile>
#include <QString>

#include "common.h"

// Controller: settings
// major controler in this Singleton while serialized storage in QSettings
// and current storage directly in checkboxes/controls around
// TODO: -2 move all settings controller code to here
class Setting: public QObject
{
    Q_OBJECT
public:
    ~Setting();
    static Setting *instance();
    static void cleanup();
    static bool exists();

    bool showNodes();
    void setShowNodes(bool v);

    bool disableIPv6();
    void setDisableIPv6(bool v);

    bool autoconnect();
    void setAutoconnect(bool v);

    bool startup();
    void setStartup(bool v);

    bool reconnect();
    void setReconnect(bool v);

    bool detectInsecureWifi();
    void setDetectInsecureWifi(bool v);

    bool blockOnDisconnect();
    void setBlockOnDisconnect(bool v);

    bool fixDns();
    void setFixDns(bool v);

    bool testing();
    void setTesting(bool value);

    bool logging();
    void setLogging(bool value);

    void setServer(int ixsrv);
    void loadServer();

    void setProtocol(int ix);  // -1 for not selected
    void loadProtocol();
    int currentProtocol();			// -1 if none selected

    QString serverAddress();			// "" if none
    int serverID();				// -1 if none selected
    QString port();				// "" if none

    QString localPort();
    void setLocalPort(QString port);

    QString tcpOrUdp();			// "tcp" : "udp"

    QString dns1();				// empty string if none / not valid
    void setDNS1(QString value);
    QString dns2();				//
    void setDNS2(QString value);

    void setDefaultDNS(const QString & dns1, const QString & dns2);
    QString defaultDNS1();
    QString defaultDNS2();

    const std::vector<QString> & currentEncryptionProtocols();
    const std::vector<int> & currentEncryptionPorts();
    const QString & protocolName(int ix);
    const QString & currentProtocolName();

    const QString forwardPortsString();
    UVec forwardPorts();		// load from the GUI; parse errors skipped - GUI should handle errors; empty vector if none
    void setForwardPorts(QString portsString);

    bool checkForUpdates();
    void updateMessageShown();

    void switchToNextPort();
    void switchToNextNode();

    int encryption();
    void setEncryption(int enc);

    static QString encryptionName(int enc);

signals:
    void showNodesChanged();
    void detectInsecureWifiChanged();
    void encryptionChanged();
    void protocolChanged();
    void serverChanged();

private:
    Setting();
    static std::auto_ptr<Setting> mInstance;
    static std::vector<QString> mProtocols[ENCRYPTION_COUNT];
    static std::vector<int> mPorts[ENCRYPTION_COUNT];

    QString mDefaultDNS[2];
    bool mTesting;
    static void PopulateColls(std::vector<QString> & v_strs, std::vector<int> & v_ports, size_t sz, const char ** protocols, const int * ports);

    QString ProtocolSettingsName();
    QString ProtocolSettingsStrName();
    QString EncryptionIx();
    QString LocationSettingsName();
    QString LocationSettingsStrName();
    int determineNextPort();
#ifdef Q_OS_LINUX
    void delete_startup(QFile &f);
#endif
    QSettings mSettings;
};

#endif // SETTING_H
