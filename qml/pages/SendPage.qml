// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

Page {
    id: page
    objectName: "sendPage"
    property string selectedFrame: framare.defaultFrame
    property bool hasReadyImages: {
        for (var i = 0; i < framare.jobs.length; ++i)
            if (framare.jobs[i].state === "Ready") return true
        return false
    }
    Connections {
        target: framare
        onFramesChanged: {
            var exists = false
            for (var i = 0; i < framare.frames.length; ++i)
                if (framare.frames[i].name === page.selectedFrame) exists = true
            if (!exists) page.selectedFrame = framare.defaultFrame
        }
    }
    function stateText(state) {
        switch (state) {
        case "Preparing": return qsTr("Preparing image…")
        case "Ready": return qsTr("Ready")
        case "Queued": return qsTr("Queued")
        case "Sending": return qsTr("Connecting and sending…")
        case "Sent": return qsTr("Accepted by the frame")
        case "Failed": return qsTr("Failed")
        case "Cancelled": return qsTr("Cancelled")
        }
        return ""
    }
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge
        PullDownMenu {
            MenuItem {
                text: qsTr("Frames and sender")
                onClicked: pageStack.push(Qt.resolvedUrl("FramesPage.qml"))
            }
            MenuItem {
                text: qsTr("Clear queue")
                enabled: !framare.busy
                onClicked: framare.clear()
            }
            MenuItem {
                text: qsTr("Add image")
                onClicked: pageStack.push(picker)
            }
        }
        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingMedium
            PageHeader { title: applicationName }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                text: framare.error
                visible: text.length > 0
                color: Theme.highlightColor
                wrapMode: Text.Wrap
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                text: qsTr("Pair a frame from the pulley menu to get started.")
                visible: framare.frames.length === 0
                wrapMode: Text.Wrap
            }
            ComboBox {
                id: frames
                width: parent.width
                label: qsTr("Send to")
                value: page.selectedFrame
                enabled: !framare.busy
                menu: ContextMenu {
                    Repeater {
                        model: framare.frames
                        MenuItem {
                            text: modelData.name
                            onClicked: page.selectedFrame = modelData.name
                        }
                    }
                }
            }
            TextArea {
                id: caption
                width: parent.width
                label: qsTr("Caption (optional)")
                placeholderText: label
                enabled: !framare.busy
            }
            TextSwitch {
                id: fit
                text: qsTr("Fit the whole image")
                description: checked ? qsTr("The complete image is shown.") : qsTr("Fill the screen with a centred crop.")
                enabled: !framare.busy
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: framare.busy ? qsTr("Cancel transfer") : qsTr("Send images")
                enabled: framare.busy || (page.hasReadyImages && page.selectedFrame.length > 0)
                onClicked: {
                    if (framare.busy) framare.cancel()
                    else framare.send(page.selectedFrame, caption.text, fit.checked)
                }
            }
            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: framare.busy
                visible: running
                size: BusyIndicatorSize.Medium
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                visible: framare.jobs.length === 0
                text: qsTr("Share JPEG or PNG images from the gallery, or choose Add image.")
                wrapMode: Text.Wrap
            }
            Repeater {
                model: framare.jobs
                Column {
                    width: content.width
                    spacing: Theme.paddingSmall
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: modelData.preview
                        width: parent.width - 2*Theme.horizontalPageMargin
                        height: source.toString().length > 0 ? Theme.itemSizeExtraLarge * 2 : 0
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        sourceSize.width: 600
                    }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2*x
                        text: modelData.name
                        textFormat: Text.PlainText
                        truncationMode: TruncationMode.Fade
                    }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2*x
                        text: page.stateText(modelData.state) + (modelData.frame ? " · " + modelData.frame : "")
                        color: Theme.highlightColor
                        wrapMode: Text.Wrap
                    }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2*x
                        text: modelData.message
                        visible: text.length > 0
                        font.pixelSize: Theme.fontSizeSmall
                        wrapMode: Text.Wrap
                    }
                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Retry this image")
                        visible: (modelData.state === "Failed" || modelData.state === "Cancelled") && modelData.preview.toString().length > 0
                        enabled: !framare.busy
                        onClicked: framare.retry(index, page.selectedFrame)
                    }
                    Separator { width: parent.width; color: Theme.secondaryColor }
                }
            }
        }
        VerticalScrollDecorator {}
    }
    Component {
        id: picker
        ImagePickerPage {
            onSelectedContentPropertiesChanged: {
                if (selectedContentProperties.filePath)
                    framare.addImages([selectedContentProperties.filePath])
            }
        }
    }
}
