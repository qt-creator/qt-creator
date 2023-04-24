// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinetheme.h"

#include <utils/icon.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QIcon>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QQuickImageProvider>
#include <QtQml>

using namespace Utils;

namespace Timeline {

class TimelineImageIconProvider : public QQuickImageProvider
{
public:
    TimelineImageIconProvider()
        : QQuickImageProvider(Pixmap)
    {
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        Q_UNUSED(requestedSize)

        const QStringList idElements = id.split(QLatin1Char('/'));

        QTC_ASSERT(!idElements.isEmpty(), return QPixmap());
        const QString &iconName = idElements.first();
        const QIcon::Mode iconMode = (idElements.count() > 1
                                      && idElements.at(1) == QLatin1String("disabled"))
                ? QIcon::Disabled : QIcon::Normal;

        QIcon icon;
        if (iconName == "prev")
            icon = Icons::PREV_TOOLBAR.icon();
        else if (iconName == "next")
            icon = Icons::NEXT_TOOLBAR.icon();
        else if (iconName == "zoom")
            icon = Icons::ZOOM_TOOLBAR.icon();
        else if (iconName == "rangeselection")
            icon = Icon({{":/qt/qml/QtCreator/Tracing/ico_rangeselection.png", Theme::IconsBaseColor}}).icon();
        else if (iconName == "rangeselected")
            icon = Icon({{":/qt/qml/QtCreator/Tracing/ico_rangeselected.png", Theme::IconsBaseColor}}).icon();
        else if (iconName == "selectionmode")
            icon = Icon({{":/qt/qml/QtCreator/Tracing/ico_selectionmode.png", Theme::IconsBaseColor}}).icon();
        else if (iconName == "edit")
            icon = Icon({{":/qt/qml/QtCreator/Tracing/ico_edit.png", Theme::IconsBaseColor}}).icon();
        else if (iconName == "lock_open")
            icon = Icons::UNLOCKED_TOOLBAR.icon();
        else if (iconName == "lock_closed")
            icon = Icons::LOCKED_TOOLBAR.icon();
        else if (iconName == "range_handle")
            icon = Icon({{":/qt/qml/QtCreator/Tracing/range_handle.png", Theme::IconsBaseColor}}).icon();
        else if (iconName == "note")
            icon = Icons::INFO_TOOLBAR.icon();
        else if (iconName == "split")
            icon = Icons::SPLIT_HORIZONTAL_TOOLBAR.icon();
        else if (iconName == "close_split")
            icon = Icons::CLOSE_SPLIT_TOP.icon();
        else if (iconName == "close_window")
            icon = Icons::CLOSE_TOOLBAR.icon();
        else if (iconName == "arrowdown")
            icon = Icons::ARROW_DOWN.icon();
        else if (iconName == "arrowup")
            icon = Icons::ARROW_UP.icon();

        const QSize iconSize(16, 16);
        const QPixmap result = icon.pixmap(iconSize, iconMode);

        if (size)
            *size = result.size();
        return result;
    }
};

TimelineTheme::TimelineTheme(QObject *parent)
    : Utils::Theme(Utils::creatorTheme(), parent)
{
}

void TimelineTheme::setupTheme(QQmlEngine *engine)
{
    engine->addImageProvider(QLatin1String("icons"), new TimelineImageIconProvider);
}

bool TimelineTheme::compactToolbar() const
{
    return StyleHelper::toolbarStyle() == StyleHelper::ToolbarStyleCompact;
}

int TimelineTheme::toolBarHeight() const
{
    return StyleHelper::navigationWidgetHeight();
}

} // namespace Timeline
