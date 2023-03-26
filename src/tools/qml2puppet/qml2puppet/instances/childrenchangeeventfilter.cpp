// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "childrenchangeeventfilter.h"

#include <QEvent>

namespace QmlDesigner {
namespace Internal {

ChildrenChangeEventFilter::ChildrenChangeEventFilter(QObject *parent)
    : QObject(parent)
{
}


bool ChildrenChangeEventFilter::eventFilter(QObject * /*object*/, QEvent *event)
{
    switch (event->type()) {
        case QEvent::ChildAdded:
        case QEvent::ChildRemoved:
            {
                QChildEvent *childEvent = static_cast<QChildEvent*>(event);
                emit childrenChanged(childEvent->child()); break;
            }
        default: break;
    }

    return false;
}
} // namespace Internal
} // namespace QmlDesigner
