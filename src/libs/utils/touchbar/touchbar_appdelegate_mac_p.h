// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "touchbar.h"

#import <AppKit/NSApplication.h>

@interface ApplicationDelegateImpl : NSResponder <NSApplicationDelegate>
@property (strong, atomic) NSObject *qtDelegate;

- (id)init;
- (void)installAsDelegateForApplication:(NSApplication *)application;
- (void)setApplicationTouchBar:(Utils::Internal::TouchBarPrivate *)bar;
- (void)pushTouchBar:(Utils::Internal::TouchBarPrivate *)bar;
- (void)popTouchBar;
- (void)applicationWillFinishLaunching:(NSNotification*)notify;

@end

namespace Utils {
namespace Internal {

class ApplicationDelegate
{
public:
    static ApplicationDelegate *instance();

    ApplicationDelegate();
    ~ApplicationDelegate();
    void setApplicationTouchBar(TouchBarPrivate *touchBar);
    void pushTouchBar(TouchBarPrivate *touchBar);
    void popTouchBar();

    ApplicationDelegateImpl *applicationDelegate;
};

} // Internal
} // Utils
