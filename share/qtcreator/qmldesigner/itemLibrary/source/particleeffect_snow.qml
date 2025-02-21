/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Design Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick3D
import QtQuick3D.Particles3D
ParticleSystem3D {
    id: snow
    x: 50
    y: 300
    ParticleEmitter3D {
        id: snowEmitter
        emitRate: 500
        lifeSpan: 4000
        particle: snowParticle
        particleScale: 2
        particleScaleVariation: 1
        velocity: snowDirection
        shape: snowShape

        VectorDirection3D {
            id: snowDirection
            direction.y: -100
            direction.z: 0
        }

        SpriteParticle3D {
            id: snowParticle
            color: "#dcdcdc"
            maxAmount: 5000
            particleScale: 1
            sprite: snowTexture
            billboard: true

            Texture {
                id: snowTexture
                source: "snowflake.png"
            }
        }
    }
    ParticleShape3D {
        id: snowShape
        fill: true
        extents.x: 400
        extents.y: 1
        extents.z: 400
        type: ParticleShape3D.Cube
    }

    Wander3D {
        id: wander
        globalPace.x: 0.01
        globalAmount.x: -500
        uniqueAmount.x: 50
        uniqueAmount.y: 20
        uniqueAmount.z: 50
        uniqueAmountVariation: 0.1
        uniquePaceVariation: 0.2
        uniquePace.x: 0.03
        uniquePace.z: 0.03
        uniquePace.y: 0.01
        particles: snowParticle
    }
}
