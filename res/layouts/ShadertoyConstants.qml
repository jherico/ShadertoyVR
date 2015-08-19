import QtQuick 2.4

Item {
    SystemPalette { id: sysPalette; colorGroup: SystemPalette.Active }

    Item {
        id: colors
     }

    QtObject {
        id: fonts
        readonly property string fontFamily: "Arial"  // Available on both Windows and OSX
        readonly property real pixelSize: 22  // Logical pixel size; works on Windows and OSX at varying physical DPIs
        readonly property real headerPixelSize: 32
    }

    QtObject {
        id: layout
        property int spacing: 8
        property int rowHeight: 40
        property int windowTitleHeight: 48
    }

    QtObject {
        id: styles
        readonly property int borderWidth: 5
        readonly property int borderRadius: borderWidth * 2
    }

    QtObject {
        id: effects
        readonly property int fadeInDuration: 300
    }
}
