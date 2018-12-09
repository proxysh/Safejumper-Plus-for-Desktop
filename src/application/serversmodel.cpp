/*                                                                         *
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

#include "serversmodel.h"

#include "flag.h"
#include "protocol.h"
#include "setting.h"

#include <QJsonObject>
#include <QQmlApplicationEngine>

#include <QDebug>

ServersModel::ServersModel(QObject *parent)
    :QAbstractListModel(parent)
{
    mServers.clear();
    setObjectName("ServersModel");

    qmlRegisterType<AServer>("vpn.server", 1, 0, "Server");
}

ServersModel::~ServersModel()
{

}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return mServers.count();
    return 0;
}

QVariant ServersModel::data(const QModelIndex &index, int role) const
{
    QVariant retval;

    if (index.row() >= 0 && index.row() < mServers.size()) {
        AServer *server = mServers.at(index.row());
        switch (role) {
        case nameRole:
            retval = server->name();
            break;

        case isoRole:
            retval = server->iso();
            break;

        case loadRole:
            retval = server->load();
            break;

//        case portsRole:
//            retval = server->ports();
//            break;

        case ipRole:
            retval = server->ip();
            break;

        case hostnameRole:
            retval = server->address();
            break;

        case pingRole:
            if (server->ping() > 0)
                retval = QString("%1 ms").arg(server->ping());
            else
                retval = "";
            break;

        }

    }

    return retval;
}

QHash<int, QByteArray> ServersModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[nameRole] = "name";
    roles[isoRole] = "iso";
    roles[hostnameRole] = "hostname";
    roles[ipRole] = "ip";
    roles[portsRole] = "ports";
    roles[favoriteRole] = "favorite";
    roles[loadRole] = "load";
    roles[pingRole] = "ping";

    return roles;
}

void ServersModel::updateServers(const QJsonArray &servers)
{
    mServers.clear();

    appendServers(servers);
}

void ServersModel::appendServers(const QJsonArray &servers)
{
    beginResetModel();

    // One list of protocols per encryption type
    QList<Protocol> portsForEncryption[ENCRYPTION_COUNT];

    Q_FOREACH(const QJsonValue &server, servers) {
        QJsonObject serverObject = server.toObject();
        AServer *newServer = new AServer(this);
        newServer->setIP(serverObject.value("ip").toString());
        newServer->setName(serverObject.value("name").toString());
        newServer->setAddress(serverObject.value("hostname").toString());
        newServer->setISO(serverObject.value("iso_code").toString());
        QJsonValue serverPorts = serverObject.value("ports");
        QJsonValue tcpPorts = serverPorts.toObject().value("tcp");
        QJsonValue udpPorts = serverPorts.toObject().value("udp");
        QStringList types;
        // IMPORTANT: Make sure the order here is the same as the encryption enumeration
        types << "plain" << "obfs2" << "obfs3" << "scramblesuit" << "ecc" << "xor";
        int encryption = ENCRYPTION_RSA;
        Q_FOREACH(const QString &type, types) {
            QVariantList tcpVariantList;
            QVariantList udpVariantList;
            QJsonArray tcpList = tcpPorts.toObject().value(type).toArray();
            QJsonArray udpList = udpPorts.toObject().value(type).toArray();
            if (tcpList.size() > 0) {
                Q_FOREACH(const QJsonValue &value, tcpList) {
                    Protocol newProtocol;
                    newProtocol.setTcp(true);
                    newProtocol.setPort(value.toInt());
                    tcpVariantList << value.toInt();
                    if (!portsForEncryption[encryption].contains(newProtocol)) {
                        portsForEncryption[encryption].append(newProtocol);
                    }
                }
                newServer->setPorts(encryption, true, tcpVariantList);
            }

            if (udpList.size() > 0) {
                Q_FOREACH(const QJsonValue &value, udpList) {
                    Protocol newProtocol;
                    newProtocol.setTcp(false);
                    newProtocol.setPort(value.toInt());
                    udpVariantList << value.toInt();
                    if (!portsForEncryption[encryption].contains(newProtocol)) {
                        portsForEncryption[encryption].append(newProtocol);
                    }
                }
                newServer->setPorts(encryption, false, udpVariantList);
            }
            encryption++;
        }
        newServer->setLoad(server.toObject().value("server_load").toString().toInt());
        // TODO: Load if this server is a favorite from settings
        newServer->setFavorite(Setting::instance()->favorites().contains(newServer->address()));
        newServer->setPing(-1);
        mServers.append(newServer);
    }

    // Now sort mServers by name
    std::sort(mServers.begin(), mServers.end(), [](AServer * a, AServer * b) {
        return a->name() < b->name();
    });

    // Reset ids since we just sorted the list
    int id = 0;
    Q_FOREACH(AServer *server, mServers) {
        server->setId(id);
        ++id;
    }

    for (int encryption = 0; encryption < ENCRYPTION_COUNT; ++encryption) {
        QStringList portNameList;
        QList<int>  portNumberList;
        Q_FOREACH(Protocol protocol, portsForEncryption[encryption]) {
            portNumberList.append(protocol.port());
            portNameList.append(protocol.displayName());
        }
        Setting::instance()->setEncryptionPorts(encryption, portNameList, portNumberList);
    }

    endResetModel();
}

AServer *ServersModel::server(int index)
{
    if (index >= 0 && index < mServers.size())
        return mServers.at(index);

    return nullptr;
}

int ServersModel::count()
{
    return mServers.count();
}

void ServersModel::setPing(int index, int ping)
{
    if (index >= 0 && index < mServers.size())
        mServers.at(index)->setPing(ping);
}

QList<int> ServersModel::serversForEncryption(int encryption)
{
    QList<int> ids;
    qDebug() << "serversForEncryption called with encryption " << encryption
             << " mServers.count() is " << mServers.count();
    for (int i = 0; i < mServers.count(); ++i) {
        if (mServers.at(i)->supportsEncryption(encryption))
            ids << i;
    }

    qDebug() << "found " << ids.count() << " servers for encryption " << encryption;

    return ids;
}

QList<int> ServersModel::favoriteServers()
{
    QList<int> favorites;

    for(int i = 0; i < mServers.count(); ++i) {
        if (mServers.at(i)->favorite())
            favorites << i;
    }

    return favorites;
}
