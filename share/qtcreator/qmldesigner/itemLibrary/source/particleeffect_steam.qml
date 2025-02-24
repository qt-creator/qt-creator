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
    id: steam
    ParticleEmitter3D {
        id: steamEmitter
        emitRate: 10
        lifeSpan: 1500
        lifeSpanVariation: 300
        particle: steamParticle
        particleScale: 7.5
        particleEndScale: 12.5
        particleScaleVariation: 2.5
        velocity: steamDirection
        depthBias: -100

        SpriteParticle3D {
            id: steamParticle
            color: "#c5e3eaf2"
            maxAmount: 50
            particleScale: 12
            fadeInDuration: 200
            fadeOutDuration: 350
            sprite: steamTexture
            spriteSequence: steamSequence
            fadeInEffect: Particle3D.FadeOpacity
            blendMode: SpriteParticle3D.SourceOver
            sortMode: Particle3D.SortNewest
            billboard: true

            Texture {
                id: steamTexture
                source: "smoke2.png"
            }

            SpriteSequence3D {
                id: steamSequence
                duration: 2000
                frameCount: 15
            }

            VectorDirection3D {
                id: steamDirection
                direction.y: 150
                directionVariation.x: 50
                directionVariation.y: 10
                directionVariation.z: 50
            }

            Wander3D {
                id: steamWander
                uniquePace.y: 0.03
                uniqueAmount.y: 20
                uniquePaceVariation: 1
                uniqueAmountVariation: 1
                fadeInDuration: 300
                fadeOutDuration: 500
                particles: steamParticle
                system: steam
            }
        }
    }
}
