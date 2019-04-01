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

import QtQuick 2.11
import QtQuick.Controls 2.4
import QtGraphicalEffects 1.0

Item {
    id: allServersPage

    signal backClicked()
    signal settingsClicked()

    signal selectEncryption(int index)
    signal selectPort(int index)

    function setEncryption(index)
    {
        // Set the encryption type for the currently selected server
        settings.setServerEncryption(allserversmodel.server(allServersList.currentIndex).address, index);
        allServersList.currentItem.refresh();
    }

    function setPort(index)
    {
        // Set the port for the currently selected server
        var encryption = settings.serverEncryption(allServersList.currentIndex)
        settings.setServerProtocol(allserversmodel.server(allServersList.currentIndex).address, encryption, index);
        allServersList.currentItem.refresh();
    }

    Rectangle {
        id: guiRectangle
        color: "#F4F5F7"
        anchors.fill: parent

        HeaderBar {
            id: headerArea
            backButton: true
            onSettingsClicked: { allServersPage.settingsClicked(); }
            onBackClicked: { allServersPage.backClicked(); }
        }

        ListView {
            id: allServersList
            boundsMovement: Flickable.StopAtBounds
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            flickDeceleration: 1000
            maximumFlickVelocity: 2500
            anchors {
                bottom: parent.bottom
                top: headerArea.bottom
                topMargin: 25
                left: parent.left
                leftMargin: 20
                rightMargin: 20
                right: parent.right
                horizontalCenter: parent.horizontalCenter
            }
            cacheBuffer: 10000
            model: allserversmodel
            spacing: 10
            currentIndex: settings.server
            focus: true
            delegate:
                ServerCard {
                    selectable: true
                    currentServer: allserversmodel.server(index)
                    isCurrentServer: index == settings.server
                    showOptions: index == allServersList.currentIndex
                    showButton: index == allServersList.currentIndex

                    onSelected: {
                        allServersList.currentIndex = index;
                    }

                    onSelectEncryption: {
                        console.log("onSelectEncryption called in allserverspage");
                        allServersPage.selectEncryption(index);
                    }

                    onSelectPort: {
                        console.log("onSelectPort called in allserverspage");
                        allServersPage.selectPort(index);
                    }
            }
        }
    }
}
