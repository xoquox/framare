// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    canAccept: name.text.trim().length > 0 && code.text.length >= 7 && !framare.busy
    Column {
        width: parent.width
        spacing: Theme.paddingMedium
        DialogHeader { title: qsTr("Pair a frame"); acceptText: qsTr("Pair") }
        TextField {
            id: name
            width: parent.width
            label: qsTr("Frame name")
            placeholderText: qsTr("Living room")
            focus: true
            EnterKey.onClicked: code.focus = true
        }
        TextField {
            id: code
            width: parent.width
            label: qsTr("Friendship code")
            placeholderText: label
            inputMethodHints: Qt.ImhDigitsOnly
            EnterKey.onClicked: if (canAccept) accept()
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            text: qsTr("Enter the current code shown on your frame. Pairing needs a network connection.")
            wrapMode: Text.Wrap
        }
    }
    onAccepted: framare.pair(name.text, code.text)
}
