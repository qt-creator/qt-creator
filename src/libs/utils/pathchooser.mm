// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH
// Qt-GPL-exception-1.0

#include "pathchooser.h"

#import <AppKit/AppKit.h>

QString findMacOSAppByBundleId(const QString &path) {
  @autoreleasepool {
    NSString *nsPath =
        [NSString stringWithUTF8String:path.toUtf8().constData()];

    NSURL *appURL = [[NSWorkspace sharedWorkspace]
        URLForApplicationWithBundleIdentifier:nsPath];

    if (!appURL)
      return QString();

    return QString::fromUtf8([[appURL path] UTF8String]);
  }
}
