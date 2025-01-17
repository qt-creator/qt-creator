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
    id: lightRain
    y: 2000
    ParticleEmitter3D {
        id: lightRainEmitter
        emitRate: 50
        lifeSpan: 500
        particle: lightRainParticle
        particleScale: 0.75
        particleScaleVariation: 0.25
        velocity: lightRainDirection
        shape: lightRainShape
        depthBias: -200

        VectorDirection3D {
            id: lightRainDirection
            direction.y: -(lightRain.y * 2)
        }

        SpriteParticle3D {
            id: lightRainParticle
            color: "#90e6f4ff"
            maxAmount: 100
            particleScale: 85
            fadeInDuration: 0
            fadeOutDuration: 10
            fadeOutEffect: Particle3D.FadeOpacity
            sortMode: Particle3D.SortDistance
            sprite: lightRainTexture
            billboard: true

            Texture {
                id: lightRainTexture
                source: "rain.png"
            }

            SpriteSequence3D {
                id: lightRainSequence
                duration: 15
                randomStart: true
                animationDirection: SpriteSequence3D.Normal
                frameCount: 3
                interpolate: true
            }
        }
    }

    ParticleShape3D {
        id: lightRainShape
        extents.x: 500
        extents.y: 0.01
        extents.z: 500
        type: ParticleShape3D.Cube
        fill: true
    }

    TrailEmitter3D {
        id: lightRainSplashEmitter
        emitRate: 0
        lifeSpan: 800
        particle: lightRainSplashParticle
        particleScale: 15
        particleScaleVariation: 15
        follow: lightRainParticle
        emitBursts: lightRainSplashBurst
        depthBias: -10

        SpriteParticle3D {
            id: lightRainSplashParticle
            color: "#8bc0e7fb"
            maxAmount: 250
            sprite: lightRainSplashTexture
            spriteSequence: lightRainSplashSequence
            fadeInDuration: 450
            fadeOutDuration: 800
            fadeInEffect: Particle3D.FadeScale
            fadeOutEffect: Particle3D.FadeOpacity
            sortMode: Particle3D.SortDistance
            billboard: true

            Texture {
                id: lightRainSplashTexture
                source: "splash7.png"
            }

            SpriteSequence3D {
                id: lightRainSplashSequence
                duration: 800
                frameCount: 6
            }
        }

        EmitBurst3D {
            id: lightRainSplashBurst
            time: lightRainEmitter.lifeSpan
            amount: 1
        }
    }
}
