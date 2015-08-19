import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Controls.Styles 1.3
import Qt.labs.settings 1.0

Rectangle {
    id: editRoot
    width: 1280
    height: 720
    color: "#00000000"
    function setChannelIcon(channel, path) {
        var channelItem;
        switch(channel) {
        case 0:
            channelItem = channel0;
            break;
        case 1:
            channelItem = channel1;
            break;
        case 2:
            channelItem = channel2;
            break;
        case 3:
            channelItem = channel3;
            break;
        }
        if (channelItem) {
            channelItem.source = path;
        }
    }

    ChannelsColumn {

    }

    Column {
        id: channelColumn
        width: childrenRect.width
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
        spacing: 8

        CustomBorder {
            id: channel0control
            height: 128 + 24
            width: 128 + 24
            Image {
                id: channel0
                objectName: "channel0"
                anchors.centerIn: parent
                height: 128
                width: 128
                onSourceChanged: {
                    console.log(channel0.source);
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: root.channelSelect(0);
            }
        }

        CustomBorder {
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

        CustomBorder {
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

        CustomBorder {
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

    CodeEditor {
        id: codeFrame
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.bottom: buttonArea.top
        anchors.bottomMargin: 8
        anchors.left: channelColumn.right
        anchors.leftMargin: 8
        anchors.right: infoColumn.left
        anchors.rightMargin: 8
    }


    CustomBorder {
        id: infoColumn
        width: 196
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.bottom: buttonArea.top
        anchors.bottomMargin: 16

        Grid {
            anchors.fill: parent
            anchors.margins: parent.margin * 2
            columns: 2
            spacing: 12
            CustomText { text: "FPS"; } CustomText { objectName: "fps"; text: "0" }
            CustomText { text: "RES"; } CustomText { objectName: "res"; text: "0" }
            CustomText { text: "EPS"; } CustomText { objectName: "eps"; text: "0" }
            CustomText { text: "EPF";  } Switch {
                objectName: "epf";
                onCheckedChanged: {
                    root.toggleEyePerFrame();
                }
            }
            CustomText { text: "Font";  } Row {
                IconControl {
                    text: "\uF056"
                    onClicked: shaderTextEdit.font.pointSize -= 1
                }
                IconControl {
                    text: "\uF055"
                    onClicked: shaderTextEdit.font.pointSize += 1
                }
            }
        }
    }

    Rectangle {
        id: buttonArea
        color: "#00000000"
        height: 64
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        anchors.left: codeFrame.left
        anchors.right: codeFrame.right
        Row {
            height: parent.height
            anchors.right: parent.right
            spacing: 8
            layoutDirection: Qt.RightToLeft
            CustomButton {
                id: load
                height: parent.height
                text: qsTr("Load")
                onClicked: {
                    editor.visible = false;
                    loader.visible = true;
                }
            }
            CustomButton {
                id: save
                height: parent.height
                text: qsTr("Save")
                onClicked: {
                    editor.visible = false;
                    saver.visible = true;
                }
            }
        }
        Row {
            height: parent.height
            anchors.left: parent.left
            spacing: 8

            CustomButton {
                id: run
                height: parent.height
                text: qsTr("Run")
                onClicked: shaderSourceChanged(shaderTextEdit.text);
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            shaderTextEdit.forceActiveFocus()
        }
    }
}




