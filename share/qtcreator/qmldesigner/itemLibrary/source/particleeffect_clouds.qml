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
    id: cloudSystem
    ParticleEmitter3D {
        id: baseCloudEmitter
        emitRate: 0
        lifeSpan: 200000
        particle: cloudParticle
        particleScale: 35
        particleScaleVariation: 10
        emitBursts: cloudBaseBurst
        velocity: cloudDirection
        shape: cloudShape
        depthBias: -20
        SpriteParticle3D {
            id: cloudParticle
            color: "#bcffffff"
            particleScale: 12
            fadeInEffect: Particle3D.FadeScale
            fadeInDuration: 0
            fadeOutDuration: 0
            blendMode: SpriteParticle3D.SourceOver
            sprite: cloudTexture
            spriteSequence: cloudSequence
            billboard: true
            maxAmount: 50
            sortMode: Particle3D.SortNewest
            Texture {
                id: cloudTexture
                source: "smoke_sprite2.png"
            }
            SpriteSequence3D {
                id: cloudSequence
                animationDirection: SpriteSequence3D.Alternate
                durationVariation: 3000
                interpolate: true
                randomStart: true
                frameCount: 15
                duration: 50000
            }
        }

        ParticleShape3D {
            id: cloudShape
            type: ParticleShape3D.Sphere
            fill: false
            extents.z: 250
            extents.y: 100
            extents.x: 250
        }

        EmitBurst3D {
            id: cloudBaseBurst
            amount: 10
        }
    }

    ParticleEmitter3D {
        id: smallCloudEmitter
        lifeSpan: 2000000
        emitRate: 0
        particle: cloudSmallParticle
        particleScale: 18
        particleScaleVariation: 7
        velocity: cloudDirection
        shape: cloudOuterShape
        emitBursts: cloudSmallBurst
        depthBias: -25
        SpriteParticle3D {
            id: cloudSmallParticle
            color: "#65ffffff"
            maxAmount: 75
            particleScale: 12
            fadeOutDuration: 0
            fadeInDuration: 0
            fadeInEffect: Particle3D.FadeScale
            blendMode: SpriteParticle3D.SourceOver
            sortMode: Particle3D.SortNewest
            spriteSequence: cloudSequence
            sprite: cloudTexture
            billboard: true
        }

        ParticleShape3D {
            id: cloudOuterShape
            extents.x: 350
            extents.y: 150
            extents.z: 350
            fill: true
            type: ParticleShape3D.Sphere
        }

        EmitBurst3D {
            id: cloudSmallBurst
            amount: 15
        }
    }
    VectorDirection3D {
        id: cloudDirection
        direction.y: 0
        direction.z: -20
    }
    Wander3D {
        id: cloudWander
        uniqueAmountVariation: 0.3
        uniqueAmount.x: 15
        uniqueAmount.y: 15
        uniqueAmount.z: 15
        uniquePace.x: 0.01
        uniquePace.y: 0.01
        uniquePace.z: 0.01
        particles: [cloudParticle, cloudSmallParticle, smallCloudEmitter]
        system: cloudSystem
    }
}
