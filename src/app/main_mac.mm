// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qglobal.h>

#include <Foundation/NSNumberFormatter.h>
#include <Foundation/NSUserDefaults.h>

// Disable "ApplePressAndHold". That feature allows entering special characters by holding
// a key, e.g. holding the 'l' key opens a popup that allows entering 'ł'.
// Since that disables key repeat, we don't want it, though.
static void initializeCreatorDefaults()
{
    [NSUserDefaults.standardUserDefaults registerDefaults:@{@"ApplePressAndHoldEnabled": @NO}];
}
Q_CONSTRUCTOR_FUNCTION(initializeCreatorDefaults);
