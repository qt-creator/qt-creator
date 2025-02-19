import QtQuick
import QtQuick3D.Helpers
import QtQuick3D
import QtQuick3D.Xr

XrView {
    id: xrView

@if %{Passthrough}
    property bool preferPassthrough: true
    passthroughEnabled: passthroughSupported && preferPassthrough
@endif

    environment: SceneEnvironment {
        clearColor: "skyblue"
@if %{Passthrough}
        backgroundMode: xrView.passthroughEnabled ? SceneEnvironment.Transparent : SceneEnvironment.Color
@else
        backgroundMode: SceneEnvironment.Color
@endif
    }

    xrOrigin: theOrigin
    XrOrigin {
        id: theOrigin

        XrController {
            controller: XrController.ControllerLeft
            poseSpace: XrController.AimPose
            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                materials: PrincipledMaterial {
                    lighting: DefaultMaterial.NoLighting
                    baseColor: "red"
                }
            }
        }

        XrController {
            controller: XrController.ControllerRight
            poseSpace: XrController.AimPose
            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                materials: PrincipledMaterial {
                    lighting: DefaultMaterial.NoLighting
                    baseColor: "green"
                }
            }
        }
    }

    DirectionalLight {
        eulerRotation.x: -30
        eulerRotation.y: -70
    }

    // The scene:
    Model {
        y: 100
        z: -50
        source: "#Cube"
        scale: Qt.vector3d(0.2, 0.2, 0.2)
        materials: PrincipledMaterial {
            baseColor: "green"
        }
        eulerRotation.x: 30
        PropertyAnimation on eulerRotation {
            from: "30, 0, 0"
            to: "30, 360, 0"
            loops: -1
            duration: 20000
        }
    }
@if %{Anchors}
    // Anchors:
    Repeater3D {
        id: spatialAnchors
        model: XrSpatialAnchorListModel {
        }
        delegate: Node {
            id: anchorNode
            required property XrSpatialAnchor anchor
            required property int index
            position: anchor.position
            rotation: anchor.rotation

            Model {
                // Visualize anchor orientation
                materials: PrincipledMaterial { baseColor: "white" }
                source: "#Cone"
                scale: Qt.vector3d(0.2, 0.2, 0.2)
                eulerRotation.x: 90
            }
            visible: anchor.has2DBounds || anchor.has3DBounds
        }
    }
@endif
}
