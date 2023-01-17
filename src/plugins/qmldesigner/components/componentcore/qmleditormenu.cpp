// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0#include "qmleditormenu.h"

#include "qmleditormenu.h"

#include <QStyleOption>

class QmlEditorMenuPrivate
{
private:
    friend class QmlEditorMenu;
    bool iconVisibility = true;
};

QmlEditorMenu::QmlEditorMenu(QWidget *parent)
    : QMenu(parent)
    , d(new QmlEditorMenuPrivate)
{
}

QmlEditorMenu::QmlEditorMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent)
    , d(new QmlEditorMenuPrivate)
{
}

QmlEditorMenu::~QmlEditorMenu()
{
    delete d;
}

bool QmlEditorMenu::isValid(const QMenu *menu)
{
    return qobject_cast<const QmlEditorMenu *>(menu);
}

bool QmlEditorMenu::iconsVisible() const
{
    return d->iconVisibility;
}

void QmlEditorMenu::setIconsVisible(bool visible)
{
    if (d->iconVisibility == visible)
        return;

    d->iconVisibility = visible;
    if (isVisible()) {
        style()->unpolish(this);
        style()->polish(this);
    }
}

void QmlEditorMenu::initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const
{
    QMenu::initStyleOption(option, action);
    if (!d->iconVisibility)
        option->icon = {};
}

bool QmlEditorMenu::qmlEditorMenu() const
{
    return true;
}
