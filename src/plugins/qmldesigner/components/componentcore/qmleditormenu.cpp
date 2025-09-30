// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmleditormenu.h"

#include "componentcoretracing.h"
#include "designeractionmanager.h"
#include "designericons.h"

#include <utils/hostosinfo.h>

#include <QApplication>
#include <QStyleOption>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

class QmlEditorMenu::Private
{
private:
    friend QmlEditorMenu;
    friend QmlEditorStyleObject;

    bool iconVisibility = true;
    int maxIconWidth = 0;

    inline static QIcon cascadeLeft;
    inline static QIcon cascadeRight;
    inline static QIcon tick;
    inline static QIcon backspaceIcon;
};

QmlEditorMenu::QmlEditorMenu(QWidget *parent)
    : QMenu(parent)
    , d(std::make_unique<Private>())
{
    NanotraceHR::Tracer tracer{"qml editor menu constructor", category()};
}

QmlEditorMenu::QmlEditorMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent)
    , d(std::make_unique<Private>())
{
    NanotraceHR::Tracer tracer{"qml editor menu constructor with title",
                               category(),
                               keyValue("title", title)};
}

QmlEditorMenu::~QmlEditorMenu()
{
    NanotraceHR::Tracer tracer{"qml editor menu destructor", category()};
}

bool QmlEditorMenu::isValid(const QMenu *menu)
{
    NanotraceHR::Tracer tracer{"qml editor menu is valid", category()};

    return qobject_cast<const QmlEditorMenu *>(menu);
}

bool QmlEditorMenu::iconsVisible() const
{
    NanotraceHR::Tracer tracer{"qml editor menu icons visible", category()};

    return d->iconVisibility;
}

void QmlEditorMenu::setIconsVisible(bool visible)
{
    NanotraceHR::Tracer tracer{"qml editor menu set icons visible",
                               category(),
                               keyValue("visible", visible)};

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
    NanotraceHR::Tracer tracer{"qml editor menu init style option", category()};

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
    NanotraceHR::Tracer tracer{"qml editor menu qml editor menu", category()};

    return true;
}

QmlEditorStyleObject *QmlEditorStyleObject::instance()
{
    NanotraceHR::Tracer tracer{"qml editor style object instance", category()};

    static QmlEditorStyleObject *s_instance = nullptr;
    if (!s_instance)
        s_instance = new QmlEditorStyleObject;
    return s_instance;
}

QIcon QmlEditorStyleObject::cascadeIconLeft() const
{
    NanotraceHR::Tracer tracer{"qml editor style object cascade icon left", category()};

    return QmlEditorMenu::Private::cascadeLeft;
}

QIcon QmlEditorStyleObject::cascadeIconRight() const
{
    NanotraceHR::Tracer tracer{"qml editor style object cascade icon right", category()};

    return QmlEditorMenu::Private::cascadeRight;
}

QIcon QmlEditorStyleObject::tickIcon() const
{
    NanotraceHR::Tracer tracer{"qml editor style object tick icon", category()};

    return QmlEditorMenu::Private::tick;
}

QIcon QmlEditorStyleObject::backspaceIcon() const
{
    NanotraceHR::Tracer tracer{"qml editor style object backspace icon", category()};

    return QmlEditorMenu::Private::backspaceIcon;
}

QmlEditorStyleObject::QmlEditorStyleObject()
    : QObject(qApp)
{
    NanotraceHR::Tracer tracer{"qml editor style object constructor", category()};

    QIcon downIcon = DesignerActionManager::instance()
            .contextIcon(DesignerIcons::MinimalDownArrowIcon);

    QmlEditorMenu::Private::cascadeLeft = DesignerIcons::rotateIcon(downIcon, 90);
    QmlEditorMenu::Private::cascadeRight = DesignerIcons::rotateIcon(downIcon, -90);
    QmlEditorMenu::Private::tick = DesignerActionManager::instance().contextIcon(
        DesignerIcons::SimpleCheckIcon);
    QmlEditorMenu::Private::backspaceIcon = DesignerActionManager::instance().contextIcon(
        DesignerIcons::BackspaceIcon);
}

} // namespace QmlDesigner
