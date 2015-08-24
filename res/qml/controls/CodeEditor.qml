import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import Qt.labs.settings 1.0
import ShadertoyVR 1.0 as ShadertoyVR

ShadertoyVR.CodeEditor {
//Item {
    id: root
    property int errorMargin: 0
    property alias text: textEdit.text
    property alias lineHeight: textEdit.font.pixelSize
    implicitWidth: 800
    implicitHeight: 600
    
    Item {
        anchors.fill: parent

        Border {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: errorFrame.top
            anchors.bottomMargin: errorFrame.height == 0 ? 0 : 6

            Item {
                anchors.fill: parent
                anchors.margins: 8
                Rectangle {
                    id: lineColumn
                    clip: true
                    width: 48
                    color: "#222"
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    Column {
                        width: parent.width
                        Repeater {
                            model: Math.max(textEdit.lineCount + 2, (lineColumn.height/lineColumn.rowHeight) )
                            delegate: Text {
                                id: text
                                color: "lightsteelblue"
                                font: textEdit.font
                                width: lineColumn.width
                                height: root.lineHeight
                                horizontalAlignment: Text.AlignRight
                                verticalAlignment: Text.AlignVCenter
                                renderType: Text.NativeRendering
                                text: index + 1
                            }
                        }
                    }
                }

                Rectangle {
                    id: lineColumnSep
                    height: parent.height
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: lineColumn.right
                    width: 1
                    color: "#ddd"
                }

                TextArea {
                    id: textEdit
                    objectName: "textEdit"
                    focus: true
                    style: TextAreaStyle {
                        backgroundColor: "#222"
                        textColor: "white"
                    }
                    font.family: "Lucida Console"
                    font.pointSize: 14
                    text: qsTr("Text Edit")
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: lineColumnSep.right
                    anchors.right: parent.right
                    wrapMode: TextEdit.NoWrap
                    frameVisible: false
                    Settings {
                        property alias fontSize: textEdit.font.pointSize
                    }
                }

            }
        }

        Border {
            id: errorFrame
            objectName: "errorFrame"
            height: root.lineHeight * 6
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            Item {
                anchors.fill: parent
                anchors.margins: 8

                TextArea {
                    id: compileErrors
                    readOnly: true
                    objectName: "compileErrors"
                    style: TextAreaStyle {
                        backgroundColor: "#00000000"
                        textColor: "red"
                    }
                    font.family: "Lucida Console"
                    font.pixelSize: 14
                    text: qsTr("Text Edit")
                    anchors.margins: parent.margin
                    anchors.fill: parent
                    wrapMode: TextEdit.NoWrap
                    frameVisible: false
                }
            }
        }
    }
}
