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

Rectangle {
    id: button

    width: parent.width
    height: 72
    color: "transparent"

    signal clicked()
    property bool highlighted: false
    property string buttonIcon
    property string hoverButtonIcon
    property string buttonText

    Rectangle {
        id: blur
        anchors.fill: parent
        color: "white"
        opacity: mouseArea.containsMouse ? 1.0 : (highlighted ? .20 : 0)
    }

    Row {
        id: row
        anchors.fill: parent
        // Less margin when hovering since icons are slightly wider with shadows
        anchors.leftMargin: 48
        spacing: 8

        Rectangle { color: "transparent"; width: (38 - buttonIconID.width)/2; height: 50 }
        Image {
            id: buttonIconID
            fillMode: Image.Pad
            anchors.verticalCenter: parent.verticalCenter
            source: mouseArea.containsMouse ? hoverButtonIcon : buttonIcon
            horizontalAlignment: Image.horizontalCenter
            verticalAlignment: Image.verticalCenter
        }

        Rectangle { color: "transparent"; width: (38 - buttonIconID.width)/2; height: 50 }

        Text {
            id: element1
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 18
            font.family: "Roboto"
            font.bold: true
            color: mouseArea.containsMouse ? defaultColor : "white"
            text: buttonText
        }
    }

    MouseArea {
        cursorShape: Qt.PointingHandCursor
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: { button.clicked(); }
    }
} // End of rect
