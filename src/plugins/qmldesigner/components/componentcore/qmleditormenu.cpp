// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0#include "qmleditormenu.h"

#include "qmleditormenu.h"

#include "designeractionmanager.h"
#include "designericons.h"

#include <utils/hostosinfo.h>

#include <QApplication>
#include <QStyleOption>

using namespace QmlDesigner;

class QmlDesigner::QmlEditorMenuPrivate
{
private:
    friend class QmlDesigner::QmlEditorMenu;
    friend class QmlDesigner::QmlEditorStyleObject;

    bool iconVisibility = true;
    int maxIconWidth = 0;

    static QIcon cascadeLeft;
    static QIcon cascadeRight;
    static QIcon tick;
    static QIcon backspaceIcon;
};

QIcon QmlEditorMenuPrivate::cascadeLeft;
QIcon QmlEditorMenuPrivate::cascadeRight;
QIcon QmlEditorMenuPrivate::tick;
QIcon QmlEditorMenuPrivate::backspaceIcon;

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
    emit iconVisibilityChanged(visible);

    if (isVisible()) {
        style()->unpolish(this);
        style()->polish(this);
    }
}

void QmlEditorMenu::initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const
{
    if (option->maxIconWidth == 0)
        d->maxIconWidth = 0;

    QMenu::initStyleOption(option, action);

#ifndef QT_NO_SHORTCUT
    if (!action->isShortcutVisibleInContextMenu() && !action->shortcut().isEmpty()) {
        int tabIndex = option->text.indexOf("\t");
        if (tabIndex < 0)
            option->text += QLatin1String("\t") + action->shortcut().toString(QKeySequence::NativeText);
    }
#endif
    if (d->iconVisibility) {
        if (Utils::HostOsInfo::isMacHost()) {
            if (qApp->testAttribute(Qt::AA_DontShowIconsInMenus))
                option->icon = action->icon();
        } else {
            option->icon = action->isIconVisibleInMenu() ? action->icon() : QIcon();
        }
    } else {
        option->icon = {};
    }

    if (!option->icon.isNull() && (d->maxIconWidth == 0))
        d->maxIconWidth = style()->pixelMetric(QStyle::PM_SmallIconSize, option, this);

    option->maxIconWidth = d->maxIconWidth;
    option->styleObject = QmlEditorStyleObject::instance();
}

bool QmlEditorMenu::qmlEditorMenu() const
{
    return true;
}

QmlEditorStyleObject *QmlEditorStyleObject::instance()
{
    static QmlEditorStyleObject *s_instance = nullptr;
    if (!s_instance)
        s_instance = new QmlEditorStyleObject;
    return s_instance;
}

QIcon QmlEditorStyleObject::cascadeIconLeft() const
{
    return QmlEditorMenuPrivate::cascadeLeft;
}

QIcon QmlEditorStyleObject::cascadeIconRight() const
{
    return QmlEditorMenuPrivate::cascadeRight;
}

QIcon QmlEditorStyleObject::tickIcon() const
{
    return QmlEditorMenuPrivate::tick;
}

QIcon QmlEditorStyleObject::backspaceIcon() const
{
    return QmlEditorMenuPrivate::backspaceIcon;
}

QmlEditorStyleObject::QmlEditorStyleObject()
    : QObject(qApp)
{
    QIcon downIcon = DesignerActionManager::instance()
            .contextIcon(DesignerIcons::MinimalDownArrowIcon);

    QmlEditorMenuPrivate::cascadeLeft = DesignerIcons::rotateIcon(downIcon, 90);
    QmlEditorMenuPrivate::cascadeRight = DesignerIcons::rotateIcon(downIcon, -90);
    QmlEditorMenuPrivate::tick = DesignerActionManager::instance()
            .contextIcon(DesignerIcons::SimpleCheckIcon);
    QmlEditorMenuPrivate::backspaceIcon = DesignerActionManager::instance()
            .contextIcon(DesignerIcons::BackspaceIcon);
}
