import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import "controls"

Item {
    id: saveRoot
    width: 1280
    height: 720

    Button {
        id: save
        width: 256 - 64
        text: qsTr("Save")
        anchors.right: border1.right
        anchors.rightMargin: 0
        anchors.top: border1.bottom
        anchors.topMargin: 8
        onClicked: {
            root.saveShaderXml(shaderName.text);
            root.setUiMode("edit");
        }
    }

    Button {
        id: cancel
        width: 256 - 64
        text: qsTr("Cancel")
        anchors.top: save.top
        anchors.topMargin: 0
        anchors.right: save.left
        anchors.rightMargin: 8
        onClicked: {
            root.setUiMode("edit");
        }
    }

    Border {
        id: border1
        width: 512
        height: 512
        radius: 2
        anchors.verticalCenterOffset: 2
        anchors.horizontalCenterOffset: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter

        Border {
            id: border2
            width: 384
            height: 64
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 64

            TextEdit {
                id: shaderName
                objectName: "shaderName"
                verticalAlignment: TextEdit.AlignVCenter
                horizontalAlignment: TextEdit.AlignHCenter
                anchors.fill: parent
                anchors.margins: parent.margin
                onTextChanged: {
                    save.enabled = text;
                }
            }
        }
    }
    onVisibleChanged: {
        if (visible) {
            shaderName.forceActiveFocus()
        }
    }
}




