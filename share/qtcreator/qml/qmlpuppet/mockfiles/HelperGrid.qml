/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.0
import QtQuick3D 1.0
import GridGeometry 1.0

Node {
    id: grid

    property alias lines: gridGeometry.lines
    property alias step: gridGeometry.step

    rotation.x: 90

    // Note: Only one instance of HelperGrid is supported, as the geometry names are fixed
    Model {
        geometry: GridGeometry {
            id: gridGeometry
            name: "3D Edit View Helper Grid"
        }

        materials: [
            DefaultMaterial {
                id: mainGridMaterial
                emissiveColor: "#cccccc"
                lighting: DefaultMaterial.NoLighting
                cullingMode: Material.DisableCulling
            }
        ]
    }
    Model {
        geometry: GridGeometry {
            lines: gridGeometry.lines
            step: gridGeometry.step
            isCenterLine: true
            name: "3D Edit View Helper Grid Z Axis"
        }
        materials: [
            DefaultMaterial {
                id: vCenterLineMaterial
                emissiveColor: "#00a1d2"
                lighting: DefaultMaterial.NoLighting
                cullingMode: Material.DisableCulling
            }
        ]
    }
    Model {
        rotation.z: 90
        geometry: GridGeometry {
            lines: gridGeometry.lines
            step: gridGeometry.step
            isCenterLine: true
            name: "3D Edit View Helper Grid X Axis"
        }
        materials: [
            DefaultMaterial {
                id: hCenterLineMaterial
                emissiveColor: "#cb211a"
                lighting: DefaultMaterial.NoLighting
                cullingMode: Material.DisableCulling
            }
        ]
    }
}
