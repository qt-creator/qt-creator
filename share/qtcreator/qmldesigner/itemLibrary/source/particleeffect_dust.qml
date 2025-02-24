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
    id: dust
    y: 100
    ParticleEmitter3D {
        id: dustEmitter
        emitRate: 20
        particle: dustParticle
        particleScaleVariation: 0.25
        particleScale: 0.75
        lifeSpan: 10000
        lifeSpanVariation: 100
        velocity: dustDirection
        shape: dustShape
        SpriteParticle3D {
            id: dustParticle
            color: "#6ed0d0d0"
            sprite: dustTexture
            billboard: true
            maxAmount: 500
            fadeInDuration: 1500
            fadeOutDuration: 1500
            VectorDirection3D {
                id: dustDirection
                direction.y: 2
                direction.z: 0
                directionVariation.x: 2
                directionVariation.y: 2
                directionVariation.z: 2
            }

            Texture {
                id: dustTexture
                source: "sphere.png"
            }
        }
    }

    ParticleShape3D {
        id: dustShape
        extents.x: 500
        extents.y: 200
        extents.z: 500
    }

    Wander3D {
        id: dustWander
        system: dust
        particles: dustParticle
        uniquePaceVariation: 0.5
        uniqueAmountVariation: 0.5
        uniquePace.x: 0.05
        uniquePace.z: 0.05
        uniquePace.y: 0.05
        uniqueAmount.x: 10
        uniqueAmount.z: 10
        uniqueAmount.y: 10
    }
}
