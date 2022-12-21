// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processhandle.h"
#import <AppKit/AppKit.h>

namespace Utils {

bool ProcessHandle::activate()
{
    NSRunningApplication *app = [NSRunningApplication
            runningApplicationWithProcessIdentifier:pid()];
    return app && [app activateWithOptions:static_cast<NSApplicationActivationOptions>(
            NSApplicationActivateIgnoringOtherApps)];
}

} // Utils
