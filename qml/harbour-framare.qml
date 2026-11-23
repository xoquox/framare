// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import "pages"

ApplicationWindow {
    id: app
    property var sendPage
    initialPage: Component { SendPage { id: sending; Component.onCompleted: app.sendPage = sending } }
    Timer {
        id: shareNavigation
        interval: 100
        onTriggered: {
            if (!app.sendPage || pageStack.busy) { restart(); return }
            if (pageStack.currentPage !== app.sendPage) pageStack.pop(app.sendPage)
        }
    }
    cover: Component {
        CoverBackground {
            Column {
                anchors.centerIn: parent
                width: parent.width
                Label { text: applicationName; anchors.horizontalCenter: parent.horizontalCenter }
                Label {
                    text: framare.busy ? qsTr("Working…") : qsTr("Send your photos")
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeSmall
                }
            }
        }
    }
    ShareProvider {
        method: "images"
        capabilities: ["image/jpeg", "image/png"]
        registerName: false
        onTriggered: {
            var paths = []
            for (var i = 0; i < resources.length; ++i) {
                if (resources[i].type === ShareResource.FilePathType)
                    paths.push(resources[i].filePath)
                else
                    framare.unsupportedResource()
            }
            framare.addImages(paths)
            shareNavigation.restart()
            app.activate()
        }
    }
}
