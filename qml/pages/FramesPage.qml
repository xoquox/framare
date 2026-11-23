// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height
        PullDownMenu {
            MenuItem {
                text: qsTr("Pair a frame")
                enabled: !framare.busy
                onClicked: pageStack.push(Qt.resolvedUrl("PairDialog.qml"))
            }
        }
        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingMedium
            PageHeader { title: qsTr("Frames and sender") }
            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: framare.busy
                visible: running
                size: BusyIndicatorSize.Medium
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Cancel")
                visible: framare.busy
                onClicked: framare.cancel()
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                text: framare.error
                visible: text.length > 0
                color: Theme.highlightColor
                wrapMode: Text.Wrap
            }
            TextField {
                id: sender
                width: parent.width
                text: framare.senderName
                label: qsTr("Your sender name")
                enabled: !framare.busy
                EnterKey.onClicked: focus = false
                onFocusChanged: if (!focus && text.length > 0) framare.setSenderName(text)
            }
            Repeater {
                model: framare.frames
                Column {
                    width: content.width
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2*x
                        text: modelData.name
                        textFormat: Text.PlainText
                        color: Theme.highlightColor
                        wrapMode: Text.Wrap
                    }
                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.isDefault ? qsTr("Default frame") : qsTr("Use as default")
                        enabled: !framare.busy && !modelData.isDefault
                        onClicked: framare.setDefaultFrame(modelData.name)
                    }
                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Forget locally")
                        enabled: !framare.busy
                        onClicked: pageStack.push(forgetDialog, { frameName: modelData.name })
                    }
                    Separator { width: parent.width; color: Theme.secondaryColor }
                }
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                text: qsTr("No frames paired yet. Choose Pair a frame from the pulley menu.")
                visible: framare.frames.length === 0
                wrapMode: Text.Wrap
            }
        }
        VerticalScrollDecorator {}
    }
    Component {
        id: forgetDialog
        Dialog {
            property string frameName
            Column {
                width: parent.width
                spacing: Theme.paddingLarge
                DialogHeader { acceptText: qsTr("Forget locally") }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2*x
                    text: qsTr("Remove the local pairing for %1? Photos on the frame are kept.").arg(frameName)
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
            }
            onAccepted: framare.forget(frameName)
        }
    }
}
