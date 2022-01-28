import QtQuick %{QtQuickVersion}
import QtQuick.Controls %{QtQuickVersion}
import QtQuick3D %{QtQuick3DVersion}
import %{ImportModuleName} %{ImportModuleVersion}

Rectangle {
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor

    Text {
        text: qsTr("Hello %{ProjectName}")
        anchors.centerIn: parent
        font.family: Constants.font.family
    }

    View3D {
        id: view3D
        anchors.fill: parent

        environment: sceneEnvironment

        SceneEnvironment {
            id: sceneEnvironment
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        Node {
            id: scene
            DirectionalLight {
                id: directionalLight
            }

            PerspectiveCamera {
                id: sceneCamera
                z: 350
            }

            Model {
                id: cubeModel
                eulerRotation.y: 45
                eulerRotation.x: 30
                materials: cubeMaterial
                source: "#Cube"
                DefaultMaterial {
                    id: cubeMaterial
                    diffuseColor: "#4aee45"
                }
            }
        }
    }
}
