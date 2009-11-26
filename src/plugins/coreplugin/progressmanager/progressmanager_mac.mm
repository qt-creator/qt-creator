/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "progressmanager_p.h"

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
#import <AppKit/NSDockTile.h>
#import <AppKit/NSApplication.h>
#import <AppKit/NSImageView.h>
#import <Foundation/NSString.h>

@interface ApplicationProgressView : NSView {
    int min;
    int max;
    int value;
}

+ (ApplicationProgressView *)sharedProgressView;

- (void)setRangeMin:(int)v1 max:(int)v2;
- (void)setValue:(int)v;
- (void)updateBadge;

@end

static ApplicationProgressView *sharedProgressView = nil;

@implementation ApplicationProgressView

+ (ApplicationProgressView *)sharedProgressView
{
    if (sharedProgressView == nil) {
        sharedProgressView = [[ApplicationProgressView alloc] init];
    }
    return sharedProgressView;
}

- (void)setRangeMin:(int)v1 max:(int)v2
{
    min = v1;
    max = v2;
    [self updateBadge];
}

- (void)setValue:(int)v
{
    value = v;
    [self updateBadge];
}

- (void)updateBadge
{
    [[NSApp dockTile] display];
}

- (void)drawRect:(NSRect)rect
{
    NSRect boundary = [self bounds];
    [[NSApp applicationIconImage] drawInRect:boundary
                                     fromRect:NSZeroRect
                                    operation:NSCompositeCopy
                                     fraction:1.0];
    NSRect progressBoundary = boundary;
    progressBoundary.size.height *= 0.13;
    progressBoundary.size.width *= 0.8;
    progressBoundary.origin.x = (NSWidth(boundary) - NSWidth(progressBoundary))/2.;
    progressBoundary.origin.y = NSHeight(boundary)*0.13;

    double range = max - min;
    double percent = 0.50;
    if (range != 0)
        percent = (value - min) / range;
    if (percent > 1)
        percent = 1;
    else if (percent < 0)
        percent = 0;

    NSRect currentProgress = progressBoundary;
    currentProgress.size.width *= percent;
    [[NSColor blackColor] setFill];
    [NSBezierPath fillRect:progressBoundary];
    [[NSColor lightGrayColor] setFill];
    [NSBezierPath fillRect:currentProgress];
    [[NSColor blackColor] setStroke];
    [NSBezierPath strokeRect:progressBoundary];
}

@end

void Core::Internal::ProgressManagerPrivate::setApplicationLabel(const QString &text)
{
    const char *utf8String = text.toUtf8().constData();
    NSString *cocoaString = [[NSString alloc] initWithUTF8String:utf8String];
    [[NSApp dockTile] setBadgeLabel:cocoaString];
    [cocoaString release];
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressRange(int min, int max)
{
    [[ApplicationProgressView sharedProgressView] setRangeMin:min max:max];
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressValue(int value)
{
    [[ApplicationProgressView sharedProgressView] setValue:value];
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressVisible(bool visible)
{
    if (visible) {
        [[NSApp dockTile] setContentView:[ApplicationProgressView sharedProgressView]];
    } else {
        [[NSApp dockTile] setContentView:nil];
    }
    [[NSApp dockTile] display];
}

#else

void Core::Internal::ProgressManagerPrivate::setApplicationLabel(const QString &text)
{
    Q_UNUSED(text)
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressRange(int min, int max)
{
    Q_UNUSED(min)
    Q_UNUSED(max)
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressValue(int value)
{
    Q_UNUSED(value)
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressVisible(bool visible)
{
    Q_UNUSED(visible)
}

#endif
