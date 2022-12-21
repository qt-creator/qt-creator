// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "touchbar_appdelegate_mac_p.h"
#include "touchbar_mac_p.h"

#include <stack>

#import <AppKit/NSTouchBar.h>
#import <AppKit/NSWindow.h>

Q_GLOBAL_STATIC(Utils::Internal::ApplicationDelegate, staticApplicationDelegate);

namespace Utils {
namespace Internal {

ApplicationDelegate *ApplicationDelegate::instance()
{
    return staticApplicationDelegate;
}

ApplicationDelegate::ApplicationDelegate()
{
    applicationDelegate = [[ApplicationDelegateImpl alloc] init];
    [applicationDelegate installAsDelegateForApplication:[NSApplication sharedApplication]];
}

ApplicationDelegate::~ApplicationDelegate()
{
    [applicationDelegate release];
}

void ApplicationDelegate::setApplicationTouchBar(TouchBarPrivate *touchBar)
{
    [applicationDelegate setApplicationTouchBar: touchBar];
}

void ApplicationDelegate::pushTouchBar(TouchBarPrivate *touchBar)
{
    [applicationDelegate pushTouchBar: touchBar];
}

void ApplicationDelegate::popTouchBar()
{
    [applicationDelegate popTouchBar];
}

} // Internal
} // Utils

using namespace Utils::Internal;

@implementation ApplicationDelegateImpl {
    std::stack<TouchBarPrivate *> touchBarStack;
}
@synthesize qtDelegate;

- (id)init
{
    self = [super init];
    return self;
}

- (void)setApplicationTouchBar:(TouchBarPrivate *)bar
{
    while (!touchBarStack.empty())
        [self popTouchBar];
    [self pushTouchBar:bar];
}

- (void)pushTouchBar:(TouchBarPrivate *)bar
{
    if (!touchBarStack.empty())
        touchBarStack.top()->m_invalidate = [](){}; // do nothing
    touchBarStack.push(bar);
    if (bar)
        bar->m_invalidate = []() { staticApplicationDelegate->applicationDelegate.touchBar = nil; };
    self.touchBar = nil; // force re-build
}

- (void)popTouchBar
{
    if (!touchBarStack.empty()) {
        touchBarStack.top()->m_invalidate = [](){}; // do nothing
        touchBarStack.pop();
        if (!touchBarStack.empty()) {
            touchBarStack.top()->m_invalidate = [](){
                staticApplicationDelegate->applicationDelegate.touchBar = nil;
            };
        }
        self.touchBar = nil; // force re-build
    }
}

- (NSTouchBar *)makeTouchBar
{
    if (!touchBarStack.empty())
        return [touchBarStack.top()->m_delegate makeTouchBar];
    return nil;
}

- (void)installAsDelegateForApplication:(NSApplication *)application
{
    self.qtDelegate = application.delegate;
    application.delegate = self;
}

- (BOOL)respondsToSelector:(SEL)aSelector
{
    // We want to forward to the Qt delegate. Respond to selectors it
    // responds to in addition to selectors this instance resonds to.
    return [self.qtDelegate respondsToSelector:aSelector] || [super respondsToSelector:aSelector];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
    if ([super respondsToSelector:aSelector])
        return [super methodSignatureForSelector:aSelector];
    return [self.qtDelegate methodSignatureForSelector:aSelector];
}

- (void)forwardInvocation:(NSInvocation *)anInvocation
{
    // Forward to the existing delegate. This function is only called for selectors
    // this instance does not responds to, which means that the Qt delegate
    // must respond to it (due to the respondsToSelector implementation above).
    [anInvocation invokeWithTarget:self.qtDelegate];
}

// Work around QTBUG-61707
- (void)applicationWillFinishLaunching:(NSNotification*)notify
{
    Q_UNUSED(notify)
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];
    NSWindow.allowsAutomaticWindowTabbing = NO;
}
@end
