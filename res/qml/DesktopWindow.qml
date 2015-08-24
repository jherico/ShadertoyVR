import QtQuick 2.4
import QtQuick.Window 2.2
import QtQuick.Controls 1.3
import ShadertoyVR 1.0
import "controls"

MainWindow {
    id: root
    readonly property int spacing: 8
    implicitWidth: col.width + spacing * 2
    implicitHeight: col.height + spacing * 2
    ExclusiveGroup { id: displayPluginGroup }
    Column {
        id: col
        x: root.spacing
        y: root.spacing
        spacing: 5
        Repeater {
            model: root.displayPlugins
            RadioButton {
                text: root.displayPlugins[index]
                exclusiveGroup: displayPluginGroup
                onClicked: {
                    root.activatePlugin(index);
                }
            }
        }
        Button {
            text: "B"
        }
        Button {
            text: "C"
        }
    }
}
