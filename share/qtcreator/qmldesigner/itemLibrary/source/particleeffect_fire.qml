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
    id: fire
    ParticleEmitter3D {
        id: smokeEmitter
        emitRate: 20
        lifeSpan: 1500
        lifeSpanVariation: 750
        particle: smokeParticle
        particleScale: 1
        particleScaleVariation: 4
        particleEndScale: 25
        velocity: smokeDirection

        VectorDirection3D {
            id: smokeDirection
            directionVariation.x: 10
            directionVariation.y: 10
            directionVariation.z: 10
            direction.y: 75
        }

        SpriteParticle3D {
            id: smokeParticle
            color: "#ffffff"
            maxAmount: 400
            particleScale: 5
            fadeInDuration: 3500
            fadeOutDuration: 1250
            sortMode: Particle3D.SortNewest
            blendMode: SpriteParticle3D.SourceOver
            billboard: true
            sprite: smokeTexture
            spriteSequence: sequence

            Texture {
                id: smokeTexture
                source: "smoke_sprite.png"
            }

            SpriteSequence3D {
                id: sequence
                duration: 6000
                frameCount: 15
            }
        }
    }

    ParticleEmitter3D {
        id: sparkEmitter
        emitRate: 10
        lifeSpan: 800
        lifeSpanVariation: 600
        particle: sparkParticle
        particleScaleVariation: 1
        velocity: sparkDirection
        depthBias: -100

        VectorDirection3D {
            id: sparkDirection
            directionVariation.x: 25
            directionVariation.y: 10
            directionVariation.z: 25
            direction.y: 60
        }

        SpriteParticle3D {
            id: sparkParticle
            color: "#ffffff"
            maxAmount: 100
            particleScale: 1
            fadeOutEffect: Particle3D.FadeScale
            sortMode: Particle3D.SortNewest
            blendMode: SpriteParticle3D.Screen
            billboard: true
            sprite: sphereTexture
            colorTable: colorTable

            Texture {
                id: sphereTexture
                source: "sphere.png"
            }

            Texture {
                id: colorTable
                source: "colorTable.png"
            }
        }
    }

    ParticleEmitter3D {
        id: fireEmitter
        emitRate: 90
        lifeSpan: 750
        lifeSpanVariation: 100
        particle: fireParticle
        particleScale: 3
        particleScaleVariation: 2
        velocity: fireDirection
        depthBias: -100

        VectorDirection3D {
            id: fireDirection
            directionVariation.x: 10
            directionVariation.z: 10
            direction.y: 75
        }

        SpriteParticle3D {
            id: fireParticle
            maxAmount: 500
            color: "#ffffff"
            colorTable: colorTable2
            sprite: sphereTexture
            sortMode: Particle3D.SortNewest
            fadeInEffect: Particle3D.FadeScale
            fadeOutEffect: Particle3D.FadeOpacity
            blendMode: SpriteParticle3D.Screen
            billboard: true

            Texture {
                id: colorTable2
                source: "color_table2.png"
            }

        }
    }

    Gravity3D {
        id: sparkGravity
        magnitude: 100
        particles: sparkParticle
        enabled: true
    }
}
