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
import QtQuick.Layouts 1.11
import QtGraphicalEffects 1.0
Item {
    id: settingsPage

    signal closeClicked()
    signal logoutClicked();
    signal customerServiceClicked();

    property bool blur: false

    function showPopup(component)
    {
        settingsPage.blur = true;
        component.visible = true;
    }

    function hidePopup(component)
    {
        settingsPage.blur = false;
        component.visible = false;
    }

    FastBlur {
        id: blurItem
        source: guiRectangle
        radius: 32
        anchors.fill: parent
        z: 10
        visible: settingsPage.blur
    }

    SelectionPopup {
        id: encryptionPopup
        title: qsTr("Encryption Type");
        subtitle: qsTr("Set your default encryption type.");
        visible: false
        itemModel: encryptionModel
        selectedIndex: settings.encryption
        z: 20

        onItemSelected: {
            hidePopup(encryptionPopup);
            settings.encryption = index;
            // Encryption changed, so update which port is selected for this encryption
            portPopup.selectedIndex = settings.defaultPortIndex;
        }

        onCancel: {
            hidePopup(encryptionPopup);
        }
    }

    SelectionPopup {
        id: portPopup
        title: qsTr("Port No");
        subtitle: qsTr("Select the port you would like to use.");
        visible: false
        itemModel: settings.ports
        selectedIndex: settings.defaultPortIndex
        z: 20

        onItemSelected: {
            hidePopup(portPopup);
            // TODO: Also save the selection
            console.log("Item selected at index " + index);
            settings.defaultPortIndex = index;
        }

        onCancel: {
            hidePopup(portPopup);
        }
    }

    SelectionPopup {
        id: languagePopup
        title: qsTr("Language");
        subtitle: qsTr("Select language for user interface");
        visible: false
        itemModel: settings.languages
        selectedIndex: settings.language
        z: 20

        onItemSelected: {
            hidePopup(languagePopup);
            settings.language = index;
        }

        onCancel: {
            hidePopup(languagePopup);
        }
    }

    InputPopup {
        id: localPortPopup
        title: qsTr("Local OpenVPN Port");
        subtitle: qsTr("Define the port through which OpenVPN will communicate locally.");
        visible: false
        value: settings.localPort
        z: 20

        onInputSaved: {
            hidePopup(localPortPopup);
            settings.localPort = value;
        }

        onCancel: {
            hidePopup(localPortPopup);
        }
    }

    DNSPopup {
        id: dnsPopup
        title: qsTr("Default DNS");
        subtitle: qsTr("Set your primary and secondary DNS.");
        dns1: settings.dns1
        dns2: settings.dns2
        visible: false;
        z: 20

        onDnsIPsSaved: {
            hidePopup(dnsPopup);

            settings.dns1 = dns1;
            settings.dns2 = dns2;
        }

        onCancel: {
            hidePopup(dnsPopup);
        }
    }

    UpdatesPopup {
        id: updatesPopup
        visible: false
        z: 20
        onCancel: {
            hidePopup(updatesPopup);
        }
    }

    Rectangle {
        id: guiRectangle
        color: "#F4F5F7"
        anchors.fill: parent

        Item {
            id: headerArea
            width: parent.width
            height: 57
            anchors.top: parent.top

            Rectangle {
                id: toolBarTop
                anchors.fill: parent
                color: defaultColor

                Image {
                    id: menuImage
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    source: "../images/back-white.png"

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor
                        anchors.fill: parent
                        onClicked: { settingsPage.closeClicked() }
                    }
                }

                Row {
                    id: centerBox
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 8

                    Text {
                        id: titleText
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: 18
                        font.family: "Roboto"
                        font.bold: true
                        color: "white"
                        text: qsTr("Settings");
                    }
                }

                Image {
                    id: settingsImage
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 20
                    source: "../images/question.png"

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            settingsPage.customerServiceClicked();
                        }
                    }
                }
            }
        }

        Flickable {
            width: parent.width
            anchors.top: headerArea.bottom
//            anchors.topMargin: 5
//            anchors.bottomMargin: 5
            anchors.bottom: parent.bottom
            contentHeight: settingsColumn.childrenRect.height + 40
            clip: true

            Column {
                id: settingsColumn
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 20
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                width: parent.width - 40 // 20 px margin on both sides
                spacing: 10

                ShadowRect {
                    id: accountDetails
                    width: parent.width
                    height: 224
                    visible: authmanager.loggedIn ? true : false

                    color: "white"
                    radius: 5

                    Column {
                        anchors.fill: parent
                        spacing: 0

                        RowLayout {
                            // Account details header
                            width: parent.width
                            height: 55

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.bold: true
                                color: defaultColor
                                text: qsTr("Account Details");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Image {
                                source: "../images/person.png"
                                Layout.alignment: Qt.AlignRight
                                Layout.rightMargin: 20
                            }
                        }

                        Rectangle {
                            color: "#EBECF0"
                            height: 1
                            width: 295
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        RowLayout {
                            width: parent.width
                            height: 38

                            Text {
                                font.pixelSize: 12
                                font.family: "Roboto"
                                font.bold: true
                                color: "#6C798F"
                                text: qsTr("EMAIL");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.weight: Font.Medium
                                color: "#091E42"
                                text: authmanager.email
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                            }
                        }

                        Rectangle {
                            color: "#EBECF0"
                            height: 1
                            width: 295
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        RowLayout {
                            width: parent.width
                            height: 38

                            Text {
                                font.pixelSize: 12
                                font.family: "Roboto"
                                font.bold: true
                                color: "#6C798F"
                                text: qsTr("PLAN");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.weight: Font.Medium
                                color: "#091E42"
                                text: authmanager.subscription
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                            }
                        }

                        Rectangle {
                            color: "#EBECF0"
                            height: 1
                            width: 295
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        RowLayout {
                            width: parent.width
                            height: 38

                            Text {
                                font.pixelSize: 12
                                font.family: "Roboto"
                                font.bold: true
                                color: "#6C798F"
                                text: qsTr("EXPIRATION");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.weight: Font.Medium
                                color: "#091E42"
                                text: authmanager.expiration
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                            }
                        }

                        Item {
                            width: parent.width
                            height: 3
                        }

                        Rectangle {
                            // Upgrade account button
                            color: "#2CC532"
                            width: 335
                            height: 49
                            radius: 5

                            Rectangle {
                                id: removeTopCorners
                                width: parent.width
                                anchors.top: parent.top
                                height: parent.radius
                                color: parent.color
                                visible: parent.visible
                            }

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.bold: true
                                color: "white"
                                text: qsTr("MANAGE YOUR ACCOUNT");
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            MouseArea {
                                cursorShape: Qt.PointingHandCursor
                                anchors.fill: parent
                                onClicked: { mainwindow.launchUrl(shopUrl); }
                            }
                        }
                    }
                }

                ToggleSetting {
                    title: qsTr("Auto-launch on startup");
                    content: qsTr("App will automatically launch when you open your device.");
                    toggleChecked: settings.startup
                    onToggled: { settings.startup = toggleChecked; }
                }

                ToggleSetting {
                    title: qsTr("Auto-connect on launch");
                    content: qsTr("App will automatically connect to the last used server when launched.");
                    toggleChecked: settings.autoconnect
                    onToggled: { settings.autoconnect = toggleChecked; }
                }

                ToggleSetting {
                    title: qsTr("Auto-reconnect VPN");
                    content: qsTr("App will automatically attempt to reconnect to VPN when it disconnects.");
                    toggleChecked: settings.reconnect
                    onToggled: { settings.reconnect = toggleChecked; }
                }

                ToggleSetting {
                    title: qsTr("Kill Switch");
                    content: qsTr("It terminates the applications you specify if the VPN connection suddenly disconnects.");
                    toggleChecked: settings.killSwitch
                    onToggled: { settings.killSwitch = toggleChecked; }
                }

                ToggleSetting {
                    title: qsTr("Auto connect on public WiFi");
                    content: qsTr("App will auto-connect to VPN when your device connects to a non-secure WiFi.");
                }

                // Default encryption
                ShadowRect {
                    id: encryptionBox
                    width: parent.width
                    height: 103

                    color: "white"
                    radius: 5

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor
                        anchors.fill: parent
                        onClicked: { showPopup(encryptionPopup); }
                    }

                    Column {
                        anchors.fill: parent

                        RowLayout {
                            width: parent.width
                            height: 55
                            spacing: 16

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.bold: true
                                color: defaultColor
                                text: qsTr("Encryption Type");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Image {
                                source: "../images/pencil.png"
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                                width: 18
                                height: 18

                            }
                        }

                        Rectangle {
                            width: 295
                            height: 1
                            color: "#EBECF0"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        RowLayout {
                            width: parent.width
                            height: 41
                            spacing: 16
                            Text {
                                font.pixelSize: 16
                                font.family: "Nunito"
                                font.bold: true
                                color: "#091E42"
                                text: settings.encryptionName
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Text {
                                font.pixelSize: 12
                                font.family: "Nunito"
                                font.bold: true
                                color: "#6C798F"
                                text: settings.encryptionCount
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                            }
                        }
                    }
                }

                // Default port
                ShadowRect {
                    id: portBox
                    width: parent.width
                    height: 103

                    color: "white"
                    radius: 5

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor
                        anchors.fill: parent
                        onClicked:
                            {
                                showPopup(portPopup);
                            }
                    }

                    Column {
                        anchors.fill: parent

                        RowLayout {
                            width: parent.width
                            height: 55
                            spacing: 16

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.bold: true
                                color: defaultColor
                                text: qsTr("Default Port");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Image {
                                source: "../images/pencil.png"
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                                width: 18
                                height: 18
                            }
                        }

                        Rectangle {
                            width: 295
                            height: 1
                            color: "#EBECF0"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        RowLayout {
                            width: parent.width
                            height: 41
                            spacing: 16
                            Text {
                                font.pixelSize: 16
                                font.family: "Nunito"
                                font.bold: true
                                color: "#091E42"
                                text: settings.defaultPort
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Text {
                                font.pixelSize: 12
                                font.family: "Nunito"
                                font.bold: true
                                color: "#6C798F"
                                text: settings.portCount
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                            }
                        }
                    }
                }

                ToggleSetting {
                    title: qsTr("Fix IPv6 Leak");
                    content: qsTr("App will disable support for IPv6 on your device to prevent leaks.");
                    toggleChecked: settings.disableIPv6
                    onToggled: { settings.disableIPv6 = toggleChecked; }
                }

                ToggleSetting {
                    title: qsTr("Fix DNS Leak");
                    content: qsTr("App will change your default DNS on your device to prevent leaks.");
                    toggleChecked: settings.fixdns
                    onToggled: { settings.fixdns = toggleChecked; }
                }

                // Default dns servers
                ShadowRect {
                    id: dnsservers
                    width: parent.width
                    height: 144

                    color: "white"
                    radius: 5

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor
                        anchors.fill: parent;
                        onClicked: { showPopup(dnsPopup); }
                    }

                    Column {
                        anchors.fill: parent

                        RowLayout {
                            width: parent.width
                            height: 55
                            spacing: 16

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.bold: true
                                color: defaultColor
                                text: qsTr("Default DNS");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Image {
                                source: "../images/pencil.png"
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                                width: 18
                                height: 18
                            }
                        }

                        Rectangle {
                            width: 295
                            height: 1
                            color: "#EBECF0"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        RowLayout {
                            width: parent.width
                            height: 41
                            spacing: 16
                            Text {
                                font.pixelSize: 12
                                font.family: "Roboto"
                                font.weight: Font.Black
                                color: "#6C798F"
                                text: qsTr("PRIMARY");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.weight: Font.Medium
                                color: "#091E42"
                                text: settings.dns1
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                            }
                        }

                        Rectangle {
                            width: 295
                            height: 1
                            color: "#EBECF0"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        RowLayout {
                            width: parent.width
                            height: 41
                            spacing: 16
                            Text {
                                font.pixelSize: 12
                                font.family: "Roboto"
                                font.weight: Font.Black
                                color: "#6C798F"
                                text: qsTr("SECONDARY");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.weight: Font.Medium
                                color: "#091E42"
                                text: settings.dns2
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                            }
                        }
                    }
                }

                // disable ping
                ToggleSetting {
                    title: qsTr("Disable Ping");
                    content: qsTr("App will not display information about ping to save bandwidth.");
                    toggleChecked: !settings.pingEnabled
                    onToggled: { settings.pingEnabled = !toggleChecked; }
                }

                // Local openvpn port
                ShadowRect {
                    id: localPortBox
                    width: parent.width
                    height: 103

                    color: "white"
                    radius: 5

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor
                        anchors.fill: parent
                        onClicked:
                            {
                                showPopup(localPortPopup);
                            }
                    }

                    Column {
                        anchors.fill: parent

                        RowLayout {
                            width: parent.width
                            height: 55
                            spacing: 16

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.bold: true
                                color: defaultColor
                                text: qsTr("Local OpenVPN Port");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Image {
                                source: "../images/pencil.png"
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                                width: 18
                                height: 18
                            }
                        }

                        Rectangle {
                            width: 295
                            height: 1
                            color: "#EBECF0"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        RowLayout {
                            width: parent.width
                            height: 41
                            spacing: 16
                            Text {
                                font.pixelSize: 12
                                font.family: "Roboto"
                                font.weight: Font.Black
                                color: "#6C798F"
                                text: qsTr("PORT");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.weight: Font.Medium
                                color: "#091E42"
                                text: settings.localPort
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                            }
                        }
                    }
                }

                // debug logging
                ToggleSetting {
                    title: qsTr("Debug Logging");
                    content: qsTr("App will provide more information in the logs for extensive debugging.");
                    toggleChecked: settings.logging
                    onToggled: { settings.logging = toggleChecked; }
                }

                // Language
                ShadowRect {
                    id: languageBox
                    width: parent.width
                    height: 103

                    color: "white"
                    radius: 5

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor
                        anchors.fill: parent
                        onClicked:
                            {
                                showPopup(languagePopup);
                            }
                    }

                    Column {
                        anchors.fill: parent

                        RowLayout {
                            width: parent.width
                            height: 55
                            spacing: 16

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.bold: true
                                color: defaultColor
                                text: qsTr("Language");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Image {
                                source: "../images/pencil.png"
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                                width: 18
                                height: 18
                            }
                        }

                        Rectangle {
                            width: 295
                            height: 1
                            color: "#EBECF0"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        RowLayout {
                            width: parent.width
                            height: 41
                            spacing: 16
                            Text {
                                font.pixelSize: 12
                                font.family: "Roboto"
                                font.weight: Font.Black
                                color: "#6C798F"
                                text: qsTr("LANGUAGE");
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.leftMargin: 20
                            }

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.weight: Font.Medium
                                color: "#091E42"
                                text: settings.currentLanguage
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.rightMargin: 20
                            }
                        }
                    }
                }

                // Notifications
                ToggleSetting {
                    title: qsTr("Notifications");
                    content: qsTr("App will show OS notifications for most events around your VPN.");
                    toggleChecked: settings.notifications
                    onToggled: { settings.notifications = toggleChecked; }
                }

                // TODO: Enable in proxy.sh branch only
                ToggleSetting {
                    title: qsTr("Display all Servers / Just Hubs");
                    content: qsTr("You can choose to either see all available servers or only the main hubs only.");
                    toggleChecked: settings.showNodes
                    onToggled: { settings.showNodes = toggleChecked; }
                }

                // Now update check button/box
                ShadowRect {
                    id: updates
                    width: parent.width
                    height: 105

                    color: "#ecedef"
                    radius: 5

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor
                        anchors.fill: parent
                        onClicked:
                            {
                                showPopup(updatesPopup);
                                updatesPopup.checkForUpdates();
                            }
                    }

                    Column {
                        anchors.fill: parent

                        Row {
                            // Check for updates
                            height: 55
                            spacing: 16
                            anchors.left: parent.left
                            anchors.leftMargin: 20
                            anchors.right: parent.right

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.bold: true
                                color: defaultColor
                                text: qsTr("Updates");
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Text {
                            width: 295
                            font.pixelSize: 14
                            font.family: "Roboto"
                            color: "#6C798F"
                            text: qsTr("Check whether there is a software update available on the Internet.");
                            anchors.leftMargin: 20
                            anchors.left: parent.left
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                // Now logout button/box
                ShadowRect {
                    id: logout
                    width: parent.width
                    height: 119
                    visible: authmanager.loggedIn

                    color: defaultColor
                    radius: 5

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor
                        anchors.fill: parent
                        onClicked: { settingsPage.logoutClicked(); }
                    }

                    Column {
                        anchors.fill: parent

                        Row {
                            // Account details header
                            height: 55
                            spacing: 16
                            anchors.left: parent.left
                            anchors.leftMargin: 20
                            anchors.right: parent.right

//                            Image {
//                                source: "../images/logout.png"
//                                anchors.verticalCenter: parent.verticalCenter
//                                width: 18
//                                height: 18
//                            }

                            Text {
                                font.pixelSize: 16
                                font.family: "Roboto"
                                font.bold: true
                                color: "white"
                                text: qsTr("Logout");
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Text {
                            width: 295
                            font.pixelSize: 14
                            font.family: "Roboto"
                            color: "white"
                            text: qsTr("Logout from your account. Please note, logging out will terminate all open connections.");
                            anchors.leftMargin: 20
                            anchors.left: parent.left
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "#c1c1c1"
                    text: settings.version
                    font.pixelSize: 12
                    font.family: "Roboto"
                }
            }
        }
    }
} // End container
