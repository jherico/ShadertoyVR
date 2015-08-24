import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import Qt.labs.settings 1.0
import ShadertoyVR 1.0 as ShadertoyVR
import "controls"

ShadertoyVR.ChannelsColumn {
    id: root
    implicitWidth: channelColumn.width
    implicitHeight: channelColumn.height
    Column {
        id: channelColumn
        width: childrenRect.width
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
        spacing: 8

        Border {
            id: channel0control
            height: 128 + 24
            width: 128 + 24
            Image {
                id: channel0
                objectName: "channel0"
                anchors.centerIn: parent
                height: 128
                width: 128
            }
            MouseArea {
                anchors.fill: parent
                onClicked: root.channelSelect(0);
            }
        }

        Border {
            id: channel1control
            height: 128 + 24
            width: 128 + 24
            Image {
                id: channel1
                objectName: "channel1"
                anchors.centerIn: parent
                height: 128
                width: 128
            }
            MouseArea {
                anchors.fill: parent
                onClicked: root.channelSelect(1);
            }
        }

        Border {
            id: channel2control
            height: 128 + 24
            width: 128 + 24
            Image {
                id: channel2
                objectName: "channel2"
                anchors.centerIn: parent
                height: 128
                width: 128
            }
            MouseArea {
                anchors.fill: parent
                onClicked: root.channelSelect(2);
            }
        }

        Border {
            id: channel3control
            height: 128 + 24
            width: 128 + 24
            Image {
                id: channel3
                objectName: "channel3"
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.top: parent.top
                anchors.topMargin: 12
                height: 128
                width: 128
            }
            MouseArea {
                anchors.fill: parent
                onClicked: root.channelSelect(3);
            }
        }
    }
}

