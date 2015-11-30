import QtQuick 2.4
import QtCanvas3D 1.0
import QtQuick.Window 2.2

import "glcode.js" as GLCode

Window {
    title: qsTr("%{ProjectName}")
    width: 1280
    height: 768
    visible: true

    Canvas3D {
        id: canvas3d
        anchors.fill: parent
        focus: true

        onInitializeGL: {
            GLCode.initializeGL(canvas3d);
        }

        onPaintGL: {
            GLCode.paintGL(canvas3d);
        }

        onResizeGL: {
            GLCode.resizeGL(canvas3d);
        }
    }
}
