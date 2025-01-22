// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "recipe.h"

#include "glueinterface.h"

#include <tasking/barrier.h>
#include <tasking/tasktree.h>

using namespace Tasking;
using namespace std::chrono;

template <typename Func>
static GroupItem transition(GlueInterface *iface, Func trigger)
{
    return BarrierTask([iface, trigger](Barrier &barrier) {
        QObject::connect(iface, trigger, &barrier, &Barrier::advance);
    }, DoneResult::Error);
}

ExecutableItem recipe(GlueInterface *iface)
{
    return Forever {
        finishAllAndSuccess,
        Group { // "working" state
            parallel,
            transition(iface, &GlueInterface::smashed), // transitions to the "broken" state
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
            transition(iface, &GlueInterface::repaired), // transitions to the "working" state
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
