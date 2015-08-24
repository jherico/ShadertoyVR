import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Controls.Styles 1.3

Button {
    text: "Text"
    width: 128
    height: 64
    style: ButtonStyle {
        background:  Border {
            anchors.fill: parent
        }
        label: Text {
           renderType: Text.NativeRendering
           verticalAlignment: Text.AlignVCenter
           horizontalAlignment: Text.AlignHCenter
           text: control.text
           color: control.enabled ? "white" : "dimgray"
        }
    }
}



