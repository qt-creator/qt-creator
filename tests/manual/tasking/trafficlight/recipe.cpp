// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "recipe.h"

#include "glueinterface.h"

#include <tasking/barrier.h>
#include <tasking/tasktree.h>

using namespace Tasking;
using namespace std::chrono;

static Group lightState(GlueInterface *iface, Lights lights, const milliseconds &timeout)
{
    return {
        Sync([iface, lights] { iface->setLights(lights); }),
        timeoutTask(timeout, DoneResult::Success)
    };
}

ExecutableItem recipe(GlueInterface *iface)
{
    return Forever {
        finishAllAndSuccess,
        Group { // "working" state
            parallel,
            signalAwaiter(iface, &GlueInterface::smashed) && DoneResult::Error, // transitions to the "broken" state
            Forever {
                lightState(iface, Light::Red, 3s), // "red" state
                lightState(iface, Light::Red | Light::Yellow, 1s), // "redGoingGreen" state
                lightState(iface, Light::Green, 3s), // "green" state
                lightState(iface, Light::Yellow, 1s), // "greenGoingRed" state
            }
        },
        Group { // "broken" state
            parallel,
            signalAwaiter(iface, &GlueInterface::repaired) && DoneResult::Error, // transitions to the "working" state
            Forever {
                lightState(iface, Light::Yellow, 1s), // "blinking" state
                lightState(iface, Light::Off, 1s), // "unblinking" state
            }
        }
    };
}
