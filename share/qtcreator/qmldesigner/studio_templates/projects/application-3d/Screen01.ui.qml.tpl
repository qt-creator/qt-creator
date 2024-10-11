/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick
import QtQuick.Controls
import QtQuick3D
import QtQuick3D.Effects
import QtQuick3D.Helpers
import %{ImportModuleName}

Rectangle {
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor

    View3D {
        id: view3D
        anchors.fill: parent

        environment: sceneEnvironment

        SceneEnvironment {
            id: sceneEnvironment
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: lightProbeTexture
        }

        Node {
            id: scene
            DirectionalLight {
                id: directionalLight
                y: 400
                z: 400
                eulerRotation.x: -40
                shadowMapQuality: Light.ShadowMapQualityVeryHigh
                shadowFactor: 100
                castsShadow: true
            }

            PerspectiveCamera {
                id: sceneCamera
                z: 390
                y: 300
                eulerRotation.x: -15
            }

            Model {
                id: cubeModel
                y: 200
                eulerRotation.y: 45
                eulerRotation.x: 30
                materials: defaultMaterial
                source: "#Cube"
                castsShadows: true
            }

            Model {
                id: groundPlane
                source: "#Rectangle"
                castsShadows: false
                receivesShadows: true
                eulerRotation.x: -90
                scale.y: 900
                scale.x: 900
                materials: groundPlaneMaterial
                readonly property bool _edit3dLocked: true
            }
        }
    }

    Item {
        id: __materialLibrary__
        PrincipledMaterial {
            id: defaultMaterial
            objectName: "Default Material"
            baseColorMap: defaultTexture
        }

        PrincipledMaterial {
            id: groundPlaneMaterial
            objectName: "Plane Material"
            baseColor: "gray"
        }

        Texture{
            id: defaultTexture
            source: "../Generated/images/template_texture.jpg"
            objectName: "Default Material"
        }

        Texture {
            id: lightProbeTexture
            textureData: ProceduralSkyTextureData {
            }
        }
    }

    Text {
        text: qsTr("Hello %{ProjectName}")
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 100
        font.family: Constants.font.family
    }
}
