// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "progressmanager_p.h"

void Core::Internal::ProgressManagerPrivate::initInternal()
{
}

void Core::Internal::ProgressManagerPrivate::cleanup()
{
}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
#import <AppKit/NSDockTile.h>
#import <AppKit/NSApplication.h>
#import <AppKit/NSImageView.h>
#import <AppKit/NSCIImageRep.h>
#import <AppKit/NSBezierPath.h>
#import <AppKit/NSColor.h>
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
    Q_UNUSED(rect)
    NSRect boundary = [self bounds];
    [[NSApp applicationIconImage] drawInRect:boundary
                                    fromRect:NSZeroRect
                                   operation:NSCompositingOperationCopy
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

void Core::Internal::ProgressManagerPrivate::doSetApplicationLabel(const QString &text)
{
    NSString *cocoaString = [[NSString alloc] initWithUTF8String:text.toUtf8().constData()];
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
