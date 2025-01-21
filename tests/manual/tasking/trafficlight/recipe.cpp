// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "recipe.h"

#include "glueinterface.h"

#include <tasking/barrier.h>
#include <tasking/tasktree.h>

using namespace Tasking;
using namespace std::chrono;

ExecutableItem recipe(GlueInterface *iface)
{
    return Forever {
        finishAllAndSuccess,
        Group { // "working" state
            parallel,
            BarrierTask( // transitions to the "broken" state
                [iface](Barrier &barrier) {
                    QObject::connect(iface, &GlueInterface::smashed, &barrier, &Barrier::advance);
                },
                DoneResult::Error),
            Forever {
                TimeoutTask( // "red" state
                    [iface](milliseconds &timeout) {
                        timeout = 3s;
                        iface->setLights(Light::Red);
                    }),
                TimeoutTask( // "redGoingGreen" state
                    [iface](milliseconds &timeout) {
                        timeout = 1s;
                        iface->setLights(Light::Red | Light::Yellow);
                    }),
                TimeoutTask( // "green" state
                    [iface](milliseconds &timeout) {
                        timeout = 3s;
                        iface->setLights(Light::Green);
                    }),
                TimeoutTask( // "greenGoingRed" state
                    [iface](milliseconds &timeout) {
                        timeout = 1s;
                        iface->setLights(Light::Yellow);
                    }),
            }
        },
        Group { // "broken" state
            parallel,
            BarrierTask( // transitions to the "working" state
                [iface](Barrier &barrier) {
                    QObject::connect(iface, &GlueInterface::repaired, &barrier, &Barrier::advance);
                },
                DoneResult::Error),
            Forever {
                TimeoutTask( // "blinking" state
                    [iface](milliseconds &timeout) {
                        timeout = 1s;
                        iface->setLights(Light::Yellow);
                    },
                    [iface] { iface->setLights(Light::Off); }),
                TimeoutTask([](milliseconds &timeout) { timeout = 1s; }) // "unblinking" state
            }
        }
    };
}
