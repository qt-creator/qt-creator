// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import GridGeometry 1.0

Node {
    id: grid

    property alias gridColor: mainGridMaterial.color
    property double density: 2500 / (gridGeometry.step + gridGeometry.step * (1.0 - subGridMaterial.generalAlpha))
    property bool orthoMode: false
    property double distance: 500

    readonly property int maxGridStep: 32 * _generalHelper.minGridStep

    readonly property int gridArea: {
        let newArea = _generalHelper.minGridStep * 512

        // Let's limit the grid size to something sensible
        while (newArea > 30000)
            newArea -= gridStep

        return newArea
    }

    readonly property double gridOpacity: 0.99

    // Step of the main lines of the grid, between those is always one subdiv line
    property int gridStep: 100

    // Minimum grid spacing in radians when viewed perpendicularly and lookAt is on origin.
    // If spacing would go smaller, gridStep is doubled and line count halved.
    // Note that spacing can stay smaller than this after maxGridStep has been reached.
    readonly property double minGridRad: 0.1

    eulerRotation.x: 90

    function calcRad(step)
    {
        return Math.atan(step / distance)
    }

    function calcStep()
    {
        if (distance === 0)
            return

        // Calculate new grid step
        let newStep = _generalHelper.minGridStep
        let gridRad = calcRad(newStep)
        while (gridRad < minGridRad && newStep < maxGridStep) {
            newStep *= 2
            if (newStep > maxGridStep)
                newStep = maxGridStep
            gridRad = calcRad(newStep)
        }
        gridStep = newStep
        subGridMaterial.generalAlpha = Math.min(1, 2 * (1 - (minGridRad / gridRad)))
    }

    onMaxGridStepChanged: calcStep()
    onDistanceChanged: calcStep()

    // Note: Only one instance of HelperGrid is supported, as the geometry names are fixed

    Model { // Main grid lines
        readonly property bool _edit3dLocked: true // Make this non-pickable
        geometry: GridGeometry {
            id: gridGeometry
            lines: grid.gridArea / grid.gridStep
            step: grid.gridStep
            name: "3D Edit View Helper Grid"
        }

        materials: [
            GridMaterial {
                id: mainGridMaterial
                color: "#cccccc"
                density: grid.density
                orthoMode: grid.orthoMode
            }
        ]
        opacity: grid.gridOpacity
    }

    Model { // Subdivision lines
        readonly property bool _edit3dLocked: true // Make this non-pickable
        geometry: GridGeometry {
            lines: gridGeometry.lines
            step: gridGeometry.step
            isSubdivision: true
            name: "3D Edit View Helper Grid subdivisions"
        }

        materials: [
            GridMaterial {
                id: subGridMaterial
                color: mainGridMaterial.color
                density: grid.density
                orthoMode: grid.orthoMode
            }
        ]
        opacity: grid.gridOpacity
    }

    Model { // Z Axis
        readonly property bool _edit3dLocked: true // Make this non-pickable
        geometry: GridGeometry {
            lines: gridGeometry.lines
            step: gridGeometry.step
            isCenterLine: true
            name: "3D Edit View Helper Grid Z Axis"
        }
        materials: [
            GridMaterial {
                id: vCenterLineMaterial
                color: "#00a1d2"
                density: grid.density
                orthoMode: grid.orthoMode
            }
        ]
        opacity: grid.gridOpacity
    }
    Model { // X Axis
        readonly property bool _edit3dLocked: true // Make this non-pickable
        eulerRotation.z: 90
        geometry: GridGeometry {
            lines: gridGeometry.lines
            step: gridGeometry.step
            isCenterLine: true
            name: "3D Edit View Helper Grid X Axis"
        }
        materials: [
            GridMaterial {
                id: hCenterLineMaterial
                color: "#cb211a"
                density: grid.density
                orthoMode: grid.orthoMode
            }
        ]
        opacity: grid.gridOpacity
    }
}
