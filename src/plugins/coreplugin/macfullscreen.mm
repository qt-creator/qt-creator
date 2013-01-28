/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "macfullscreen.h"

#include <AppKit/NSView.h>
#include <AppKit/NSWindow.h>
#include <Foundation/NSNotification.h>

#include <QSysInfo>
#include <qglobal.h>

enum {
    Qtc_NSWindowCollectionBehaviorFullScreenPrimary = (1 << 7)
};

static NSString *Qtc_NSWindowDidEnterFullScreenNotification = @"NSWindowDidEnterFullScreenNotification";
static NSString *Qtc_NSWindowDidExitFullScreenNotification = @"NSWindowDidExitFullScreenNotification";

@interface WindowObserver : NSObject {
    Core::Internal::MainWindow *window;
}

- (id)initWithMainWindow:(Core::Internal::MainWindow *)w;

- (void)notifyDidEnterFullScreen:(NSNotification *)notification;
- (void)notifyDidExitFullScreen:(NSNotification *)notification;

@end

@implementation WindowObserver

- (id)initWithMainWindow:(Core::Internal::MainWindow *)w
{
    if ((self = [self init])) {
        window = w;
    }
    return self;
}

- (void)notifyDidEnterFullScreen:(NSNotification *)notification
{
    Q_UNUSED(notification)
    window->setIsFullScreen(true);
}

- (void)notifyDidExitFullScreen:(NSNotification* )notification
{
    Q_UNUSED(notification)
    window->setIsFullScreen(false);
}

@end

static WindowObserver *observer = nil;

using namespace Core::Internal;

bool MacFullScreen::supportsFullScreen()
{
    return QSysInfo::MacintoshVersion >= QSysInfo::MV_LION;
}

void MacFullScreen::addFullScreen(MainWindow *window)
{
    if (supportsFullScreen()) {
        NSView *nsview = (NSView *) window->winId();
        NSWindow *nswindow = [nsview window];
        [nswindow setCollectionBehavior:Qtc_NSWindowCollectionBehaviorFullScreenPrimary];

        if (observer == nil)
            observer = [[WindowObserver alloc] initWithMainWindow:window];
        NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
        [nc addObserver:observer selector:@selector(notifyDidEnterFullScreen:)
            name:Qtc_NSWindowDidEnterFullScreenNotification object:nswindow];
        [nc addObserver:observer selector:@selector(notifyDidExitFullScreen:)
            name:Qtc_NSWindowDidExitFullScreenNotification object:nswindow];
    }
}

void MacFullScreen::toggleFullScreen(MainWindow *window)
{
    if (supportsFullScreen()) {
        NSView *nsview = (NSView *) window->winId();
        NSWindow *nswindow = [nsview window];
        [nswindow performSelector:@selector(toggleFullScreen:) withObject: nil];
    }
}
