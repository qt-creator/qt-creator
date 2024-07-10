// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "recipe.h"

#include "glueinterface.h"

#include <tasking/barrier.h>
#include <tasking/tasktree.h>

using namespace Tasking;
using namespace std::chrono;

Group recipe(GlueInterface *iface)
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
                        iface->setRed(true);
                    },
                    [iface] { iface->setRed(false); }),
                TimeoutTask( // "redGoingGreen" state
                    [iface](milliseconds &timeout) {
                        timeout = 1s;
                        iface->setRed(true);
                        iface->setYellow(true);
                    },
                    [iface] { iface->setRed(false); iface->setYellow(false); }),
                TimeoutTask( // "green" state
                    [iface](milliseconds &timeout) {
                        timeout = 3s;
                        iface->setGreen(true);
                    },
                    [iface] { iface->setGreen(false); }),
                TimeoutTask( // "greenGoingRed" state
                    [iface](milliseconds &timeout) {
                        timeout = 1s;
                        iface->setYellow(true);
                    },
                    [iface] { iface->setYellow(false); }),
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
                        iface->setYellow(true);
                    },
                    [iface] { iface->setYellow(false); }),
                TimeoutTask([](milliseconds &timeout) { timeout = 1s; }) // "unblinking" state
            }
        }
    };
}
