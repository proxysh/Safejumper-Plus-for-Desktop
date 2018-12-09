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

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QVariantList>

class Protocol
{
public:
    Protocol();
    virtual ~Protocol();

    void setPort(int port);
    int port();

    void setTcp(bool tcp);
    bool tcp();

    const QString displayName() const;

    bool operator==(const Protocol& other)const;
private:
    bool mTcp; // True for tcp, false for udp
    int mPort;
};

#endif // PROTOCOL_H
