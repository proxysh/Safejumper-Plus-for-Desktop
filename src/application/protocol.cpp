/***************************************************************************
 *   Copyright (C) 2018 by Jeremy Whiting <jeremypwhiting@gmail.com>       *
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

#include "protocol.h"

Protocol::Protocol()
{
}

Protocol::~Protocol()
{
}

void Protocol::setPort(int port)
{
    mPort = port;
}

int Protocol::port()
{
    return mPort;
}

void Protocol::setTcp(bool tcp)
{
    mTcp = tcp;
}

bool Protocol::tcp()
{
    return mTcp;
}

const QString Protocol::displayName() const
{
    if (mTcp)
        return QString("TCP %1").arg(mPort);
    return QString("UDP %1").arg(mPort);
}

bool Protocol::operator==(const Protocol &other) const
{
    return mTcp == other.mTcp && mPort == other.mPort;
}

