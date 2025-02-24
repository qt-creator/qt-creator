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
    id: rainMist
    ParticleEmitter3D {
        id: rainMistEmitter
        depthBias: -20
        lifeSpan: 1200
        particleScale: 5
        particle: rainMistParticle
        particleEndScale: 20
        lifeSpanVariation: 300
        velocity: rainMistDirection
        particleScaleVariation: 5
        emitRate: 30

        SpriteParticle3D {
            id: rainMistParticle
            color: "#c5e3eaf2"
            maxAmount: 100
            particleScale: 12
            sprite: rainMistTexture
            spriteSequence: rainMistSequence
            fadeInDuration: 200
            fadeOutDuration: 350
            fadeInEffect: Particle3D.FadeOpacity
            blendMode: SpriteParticle3D.SourceOver
            sortMode: Particle3D.SortNewest
            billboard: true

            Texture {
                id: rainMistTexture
                source: "smoke2.png"
            }

            SpriteSequence3D {
                id: rainMistSequence
                duration: 2000
                frameCount: 15
            }

            VectorDirection3D {
                id: rainMistDirection
                direction.x: 500
                direction.y: 0
                directionVariation.x: 100
                directionVariation.y: 2
                directionVariation.z: 100
            }

            Wander3D {
                id: rainMistWander
                uniqueAmountVariation: 1
                uniquePaceVariation: 1
                fadeInDuration: 500
                uniqueAmount.y: 10
                uniquePace.y: 0.3
                fadeOutDuration: 200
                particles: rainMistParticle
                system: rainMist
            }
        }
    }
}
