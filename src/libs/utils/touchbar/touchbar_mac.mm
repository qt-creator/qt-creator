/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "touchbar.h"
#include "touchbar_mac_p.h"
#include "touchbar_appdelegate_mac_p.h"

#include <utils/utilsicons.h>

#import <AppKit/NSButton.h>
#import <AppKit/NSCustomTouchBarItem.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSPopoverTouchBarItem.h>

namespace Utils {

TouchBar::TouchBar(const QByteArray &id, const QIcon &icon, const QString &title)
    : d(new Internal::TouchBarPrivate)
{
    d->m_id = id;
    d->m_action.setIcon(icon);
    d->m_action.setText(title);
    d->m_invalidate = []() {}; // safe-guard, do nothing by default
    d->m_delegate = [[TouchBarDelegate alloc] initWithParent:d];
}

TouchBar::~TouchBar()
{
    // clean up connections to the QActions
    clear();
    delete d;
}

TouchBar::TouchBar(const QByteArray &id, const QIcon &icon)
    : TouchBar(id, icon, {})
{
}

TouchBar::TouchBar(const QByteArray &id, const QString &title)
    : TouchBar(id, {}, title)
{
}

TouchBar::TouchBar(const QByteArray &id)
    : TouchBar(id, {}, {})
{
}

QByteArray TouchBar::id() const
{
    return d->m_id;
}

QAction *TouchBar::touchBarAction() const
{
    return &d->m_action;
}

void TouchBar::insertAction(QAction *before, const QByteArray &id, QAction *action)
{
    const QMetaObject::Connection connection = QObject::connect(action,
                                                                &QAction::changed,
                                                                [this]() { d->m_invalidate(); });
    const auto item = std::find_if(d->m_items.begin(), d->m_items.end(),
                                   [before](const Internal::TouchBarItem &item) {
                                       return item.action == before;
                                   });
    d->m_items.insert(item, {id, connection, action, nullptr});
    d->m_invalidate();
}

void TouchBar::insertTouchBar(QAction *before, TouchBar *touchBar)
{
    const QMetaObject::Connection connection = QObject::connect(touchBar->touchBarAction(),
                                                                &QAction::changed,
                                                                [this]() { d->m_invalidate(); });
    const auto item = std::find_if(d->m_items.begin(), d->m_items.end(),
                                   [before](const Internal::TouchBarItem &item) {
                                       return item.action == before;
                                   });
    d->m_items.insert(item, {touchBar->id(), connection, touchBar->touchBarAction(), touchBar->d});
    d->m_invalidate();
}

void TouchBar::removeAction(QAction *action)
{
    const auto item = std::find_if(d->m_items.begin(), d->m_items.end(),
                                   [action](const Internal::TouchBarItem &item) {
                                       return item.action == action;
                                   });
    if (item != d->m_items.end()) {
        QObject::disconnect(item->updateConnection);
        d->m_items.erase(item);
    }
    d->m_invalidate();
}

void TouchBar::removeTouchBar(TouchBar *touchBar)
{
    removeAction(touchBar->touchBarAction());
}

void TouchBar::clear()
{
    for (const Internal::TouchBarItem &item : d->m_items)
        QObject::disconnect(item.updateConnection);
    d->m_items.clear();
    d->m_invalidate();
}

void TouchBar::setApplicationTouchBar()
{
    Internal::ApplicationDelegate::instance()->setApplicationTouchBar(d);
}

} // namespace Utils

using namespace Utils::Internal;

static NSImage *iconToTemplateNSImage(const QIcon &icon)
{
    // touch bar icons are max 18-22 pts big. our are always 22 pts big.
    const QPixmap pixmap = icon.pixmap(22);

    CGImageRef cgImage = pixmap.toImage().toCGImage();
    NSImage *image = [[NSImage alloc] initWithCGImage:cgImage size:NSZeroSize];
    CFRelease(cgImage);

    // The above code ignores devicePixelRatio, so fixup after the fact
    const CGFloat userWidth = pixmap.width() / pixmap.devicePixelRatio();
    const CGFloat userHeight = pixmap.height() / pixmap.devicePixelRatio();
    image.size = CGSizeMake(userWidth, userHeight); // scales the image
    [image setTemplate:YES];
    return image;
}

// NSButton that delegates trigger to QAction's trigger.
// State (enabled/visible) is not synced since that triggers a touch bar rebuild anyhow.

@interface QActionNSButton : NSButton
@property (readonly, atomic) QAction *qaction;
+ (QActionNSButton *)buttonWithQAction:(QAction *)qaction;
@end

@implementation QActionNSButton
@synthesize qaction = _qaction;

- (void)trigger:(id)sender
{
    Q_UNUSED(sender)
    self.qaction->trigger();
}

- (id)initWithQAction:(QAction *)qaction
{
    self = [super init];
    [self setButtonType:NSButtonTypeMomentaryPushIn];
    self.bezelStyle = NSBezelStyleRounded;
    self.target = self;
    self.action = @selector(trigger:);
    _qaction = qaction;
    if (!self.qaction->text().isEmpty())
        self.title = self.qaction->text().toNSString();
    if (!self.qaction->icon().isNull())
        self.image = iconToTemplateNSImage(self.qaction->icon());
    self.enabled = self.qaction->isEnabled();
    return self;
}

+ (QActionNSButton *)buttonWithQAction:(QAction *)qaction
{
    return [[[QActionNSButton alloc] initWithQAction:qaction] autorelease];
}

@end

@interface PopoverButton : NSButton
@property (readonly, atomic) TouchBarPrivate *parent;
+ (PopoverButton *)buttonWithTouchBar:(TouchBarPrivate *)parent;
@end

@implementation PopoverButton
@synthesize parent = _parent;

- (PopoverButton *)initWithTouchBar:(TouchBarPrivate *)parent
{
    self = [super init];
    [self setButtonType:NSButtonTypeMomentaryPushIn];
    self.bezelStyle = NSBezelStyleRounded;
    self.target = self;
    self.action = @selector(trigger:);
    _parent = parent;
    if (!self.parent->m_action.text().isEmpty())
        self.title = self.parent->m_action.text().toNSString();
    if (!self.parent->m_action.icon().isNull())
        self.image = iconToTemplateNSImage(self.parent->m_action.icon());
    return self;
}

- (void)trigger:(id)sender
{
    Q_UNUSED(sender)
    ApplicationDelegate::instance()->pushTouchBar(self.parent);
}

+ (PopoverButton *)buttonWithTouchBar:(TouchBarPrivate *)parent
{
    return [[[PopoverButton alloc] initWithTouchBar:parent] autorelease];
}

@end

@implementation TouchBarDelegate
@synthesize parent = _parent;
@synthesize closeButtonIdentifier;

- (id)initWithParent:(TouchBarPrivate *)parent
{
    self = [super init];
    _parent = parent;
    const QByteArray closeIdentifier = parent->m_id + ".close";
    self.closeButtonIdentifier = QString::fromUtf8(closeIdentifier).toNSString();
    return self;
}

- (NSTouchBar *)makeTouchBar
{
    NSTouchBar *touchBar = [[NSTouchBar alloc] init];
    touchBar.delegate = self;
    NSMutableArray<NSTouchBarItemIdentifier> *items
        = [[[NSMutableArray<NSTouchBarItemIdentifier> alloc] init] autorelease];
    if (self.parent->m_isSubBar) {
        // add a close button
        [items addObject:self.closeButtonIdentifier];
    }
    for (const TouchBarItem &item : self.parent->m_items) {
        // Touch bar items that are hidden still take up space in the touch bar, so
        // only include actions that are visible
        if (item.action && item.action->isVisible()) {
            [items addObject:QString::fromUtf8(item.id).toNSString()];
        }
    }
    touchBar.defaultItemIdentifiers = items;
    return touchBar;
}

- (void)popTouchBar:(id)sender
{
    Q_UNUSED(sender)
    ApplicationDelegate::instance()->popTouchBar();
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar
       makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    Q_UNUSED(touchBar)
    if ([identifier isEqualToString:self.closeButtonIdentifier]) {
        NSCustomTouchBarItem *item = [[[NSCustomTouchBarItem alloc] initWithIdentifier:identifier]
                                                                                       autorelease];
        NSButton *button = [NSButton buttonWithImage:iconToTemplateNSImage(Utils::Icons::MACOS_TOUCHBAR_CLEAR.icon())
                                              target:self
                                              action:@selector(popTouchBar:)];
        button.bordered = NO;
        item.view = button;
        return item;
    }
    const auto itemId = QString::fromNSString(identifier).toUtf8();
    const std::vector<TouchBarItem> &items = self.parent->m_items;
    const auto actionPos = std::find_if(items.begin(), items.end(),
                                        [itemId](const TouchBarItem &item) {
                                           return item.id == itemId;
                                        });
    Q_ASSERT(actionPos != items.end());
    if (actionPos->touchBar) {
        // The default popover item does not work for us, because it closes when the touch bar
        // is rebuilt on context changes.
        // Create a custom popover item that actually replaces the application touch bar with the
        // sub bar when activated
        actionPos->touchBar->m_isSubBar = true;
        NSCustomTouchBarItem *item = [[[NSCustomTouchBarItem alloc] initWithIdentifier:identifier]
                                                                                       autorelease];
        PopoverButton *button = [PopoverButton buttonWithTouchBar:actionPos->touchBar];
        item.view = button;
        // keep buttons for subcontainers visible
        item.visibilityPriority = NSTouchBarItemPriorityHigh;
        return item;
    } else if (actionPos->action) {
        NSCustomTouchBarItem *item = [[[NSCustomTouchBarItem alloc] initWithIdentifier:identifier]
                                                                                       autorelease];
        QActionNSButton *button = [QActionNSButton buttonWithQAction:actionPos->action];
        item.view = button;
        return item;
    }
    return nil;
}

@end
