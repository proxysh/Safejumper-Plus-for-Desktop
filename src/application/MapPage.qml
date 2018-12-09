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
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import vpn.server 1.0

Item {
    id: mapPage
//    anchors.fill: parent

    signal menuClicked()
    signal settingsClicked()
    signal allServersClicked()
    signal showLogin()

    property Server currentServer: allserversmodel.server(settings.server)
    property Server favoriteServer: allserversmodel.server(settings.favorite)

    function refresh()
    {
        currentServer = allserversmodel.server(settings.server)
        favoriteServer = allserversmodel.server(settings.favorite)
        var iso = favoriteServer.iso
        currentServerCard.currentServer = currentServer;
        if (!background2.source)
            background2.source = background.source;
        background2.x = background.x
        background2.y = background.y
        background2.opacity = 1

        background.opacity = 0
        background.source = settings.mapData() // "../maps/" + iso + vpnservicemanager.stateMapSuffix
        background.x = -settings.mapXOffset()
        background.y = -settings.mapYOffset()
        leftButton.opacity = authmanager.hasPreviousFavorite() ? 1.0 : 0.0
        rightButton.opacity = authmanager.hasNextFavorite() ? 1.0 : 0.0
        mapFadeOutAnimation.start();
        mapFadeInAnimation.start();
    }

    function stateChanged()
    {
        currentServer = allserversmodel.server(settings.server)
        favoriteServer = allserversmodel.server(settings.favorite)
        var iso = favoriteServer.iso
        background.source = settings.mapData() // "../maps/" + iso + vpnservicemanager.stateMapSuffix
        background.x = -settings.mapXOffset()
        background.y = -settings.mapYOffset()
    }

    function updateFavorites()
    {
        leftButton.opacity = authmanager.hasPreviousFavorite() ? 1.0 : 0.0
        rightButton.opacity = authmanager.hasNextFavorite() ? 1.0 : 0.0
    }

    Connections {
        target: authmanager
        onServerListsLoaded: {
            refresh();
        }
    }

    Connections {
        target: settings
        onServerChanged: {
            refresh();
        }
        onFavoriteChanged: {
            refresh();
        }
        onFavoritesChanged: {
            updateFavorites();
        }
    }

    Connections {
        target: vpnservicemanager
        onVpnStateChanged: {
            stateChanged();
        }
    }

    HeaderBar {
        onSettingsClicked: { mapPage.settingsClicked(); }
        onMenuClicked: { mapPage.menuClicked(); }
    }

    Rectangle {
        anchors.fill: parent
        z: -2
        color: "#B3d4ff"
    }

    Image {
        id: background2
        z: -1
        x: -settings.mapXOffset()
        y: -settings.mapYOffset()
        width: 2200
        height: 2001
        fillMode: IMage.PreserveAspectCrop
        sourceSize.width: 2200
        sourceSize.height: 2001
        verticalAlignment: Qt.AlignTop
        horizontalAlignment: Qt.AlignLeft
        clip: true

        PropertyAnimation {
            id: mapFadeOutAnimation;
            target: background2
            property: "opacity"
            from: 1
            to: 0
            duration: 500

            onStopped: {
                background2.source = background.source
            }
        }
    }

    Image {
        id: background
        z: -1
        x: -settings.mapXOffset()
        y: -settings.mapYOffset()
        width: 2200
        height: 2001
        fillMode: Image.PreserveAspectCrop
        source: settings.mapData() // "../maps/" + allserversmodel.server(settings.favorite).iso + vpnservicemanager.stateMapSuffix
        sourceSize.width: 2200
        sourceSize.height: 2001
        verticalAlignment: Qt.AlignTop
        horizontalAlignment: Qt.AlignLeft
        clip: true

        PropertyAnimation {
            id: mapFadeInAnimation;
            target: background;
            property: "opacity";
            from: 0
            to: 1
            duration: 500
        }
    }

    Image {
        source: "../images/mapshadow.png"
        anchors.bottom: parent.bottom
        anchors.left: parent.left
    }

    Column {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        spacing: 12

        width: 335
        height: childrenRect.height

        RowLayout {
            width: parent.width
            visible: settings.showFavorites

            Image {
                id: leftButton
                Layout.alignment: Qt.AlignVCenter | Qt.alignLeft
                source: "../images/chevron-left.png"
                opacity: authmanager.hasPreviousFavorite() ? 1.0 : 0.0

                MouseArea {
                    enabled: leftButton.opacity == 1.0
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    anchors.fill: parent
                    onClicked: { authmanager.previousFavorite(); }
                }
            }

            ShadowRect {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                id: serverRectangle
                color: "white"
                radius: 5
                width: 260
                height: 48

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 35
                    anchors.rightMargin: 35
                    spacing: 15

                    Image {
                        id: flagImage
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        fillMode: Image.Stretch
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        source: "../roundflags/" + favoriteServer.iso + ".png"
                        z: 2

                        Image {
                            anchors.centerIn: parent
                            width: 30
                            height: 30
                            fillMode: Image.Stretch
                            source: "../images/small-grey-ring.png"
                            z: 4
                        }
                    }


                    Text {
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        font.family: "Roboto"
                        font.weight: Font.Medium
                        font.pixelSize: 16
                        color: "#091E42"
                        text: favoriteServer.name
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        settings.server = settings.favorite
                    }
                }
            }

            Image {
                id: rightButton
                Layout.alignment: Qt.AlignVCenter | Qt.alignRight
                source: "../images/chevron-right.png"
                opacity: authmanager.hasNextFavorite() ? 1.0 : 0.0

                MouseArea {
                    enabled: rightButton.opacity == 1.0
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    anchors.fill: parent
                    onClicked: { authmanager.nextFavorite(); }
                }
            }
        }

        ServerCard {
            id: currentServerCard
            showButton: true
            selectable: false
            showState: true
            isCurrentServer: true
        }

        ShadowRect {
            id: allServersButton
            width: parent.width
            height: 56
            color: "white"
            radius: 5

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("SHOW ALL SERVERS");
                color: defaultColor
                font.family: "Roboto"
                font.bold: true
                font.pixelSize: 16
            }

            MouseArea {
                cursorShape: Qt.PointingHandCursor
                anchors.fill: parent
                onClicked: {
                    mapPage.allServersClicked();
                }
            }
        }


    }
}
