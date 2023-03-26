// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAction>
#include <QByteArray>

#include <functional>
#include <vector>

namespace Utils {
namespace Internal {
class TouchBarPrivate;
} // Internal
} // Utils

#import <AppKit/NSTouchBar.h>

@interface TouchBarDelegate : NSObject <NSTouchBarDelegate>
@property (atomic) Utils::Internal::TouchBarPrivate *parent;
@property (retain, atomic) NSString *closeButtonIdentifier;

- (id)initWithParent:(Utils::Internal::TouchBarPrivate *)parent;
- (NSTouchBar *)makeTouchBar;
@end

namespace Utils {
namespace Internal {

struct TouchBarItem
{
    QByteArray id;
    QMetaObject::Connection updateConnection;
    QAction *action = nullptr;
    TouchBarPrivate *touchBar = nullptr;
};

class TouchBarPrivate
{
public:
    QByteArray m_id;
    QAction m_action;
    std::vector<TouchBarItem> m_items;
    std::function<void()> m_invalidate;
    TouchBarDelegate *m_delegate;
    bool m_isSubBar = false;
};

} // Internal
} // Utils
