/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "timelinetheme.h"

#include <utils/icon.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <utils/theme/theme.h>

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

        Icon icon;
        if (iconName == "prev")
            icon = Icons::PREV_TOOLBAR;
        else if (iconName == "next")
            icon = Icons::NEXT_TOOLBAR;
        else if (iconName == "zoom")
            icon = Icons::ZOOM_TOOLBAR;
        else if (iconName == "rangeselection")
            icon = Icon({{":/QtCreator/Tracing/ico_rangeselection.png", Theme::IconsBaseColor}});
        else if (iconName == "rangeselected")
            icon = Icon({{":/QtCreator/Tracing/ico_rangeselected.png", Theme::IconsBaseColor}});
        else if (iconName == "selectionmode")
            icon = Icon({{":/QtCreator/Tracing/ico_selectionmode.png", Theme::IconsBaseColor}});
        else if (iconName == "edit")
            icon = Icon({{":/QtCreator/Tracing/ico_edit.png", Theme::IconsBaseColor}});
        else if (iconName == "lock_open")
            icon = Icons::UNLOCKED_TOOLBAR;
        else if (iconName == "lock_closed")
            icon = Icons::LOCKED_TOOLBAR;
        else if (iconName == "range_handle")
            icon = Icon({{":/QtCreator/Tracing/range_handle.png", Theme::IconsBaseColor}});
        else if (iconName == "note")
            icon = Icons::INFO_TOOLBAR;
        else if (iconName == "split")
            icon = Icons::SPLIT_HORIZONTAL_TOOLBAR;
        else if (iconName == "close_split")
            icon = Icons::CLOSE_SPLIT_TOP;
        else if (iconName == "close_window")
            icon = Icons::CLOSE_TOOLBAR;

        const QSize iconSize(16, 16);
        const QPixmap result = icon.icon().pixmap(iconSize, iconMode);

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
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
    static const int typeIndex = qmlRegisterSingletonType<Utils::Theme>(
                "QtCreator.Tracing", 1, 0, "Theme",
                [](QQmlEngine *, QJSEngine *){ return Utils::proxyTheme(); });
    Q_UNUSED(typeIndex)
#endif // Qt < 6.2
    engine->addImageProvider(QLatin1String("icons"), new TimelineImageIconProvider);
}

} // namespace Timeline
