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

#include "theming.h"

#include <utils/theme/theme.h>

#include <QRegExp>

namespace QmlDesigner {

void Theming::insertTheme(QQmlPropertyMap *map)
{
    const QVariantHash creatorTheme = Utils::creatorTheme()->values();
    for (auto it = creatorTheme.constBegin(); it != creatorTheme.constEnd(); ++it)
        map->insert(it.key(), it.value());

    /* Custom Colors */

    QColor darkerBackground = Utils::creatorTheme()->color(Utils::Theme::BackgroundColorDark);

    if (darkerBackground.value() < 40)
        darkerBackground = darkerBackground.lighter(150);

    map->insert("QmlDesignerBackgroundColorDarker", darkerBackground.darker(120));
    map->insert("QmlDesignerBackgroundColorLighter", darkerBackground.lighter(140));

    if (darkerBackground.value() < 100)
        map->insert("QmlDesignerBackgroundColorDarkAlternate", darkerBackground.lighter(120));
    else
        map->insert("QmlDesignerBackgroundColorDarkAlternate", darkerBackground.lighter(110));

    Utils::creatorTheme()->color(Utils::Theme::PanelTextColorLight);
    QColor tabLight = Utils::creatorTheme()->color(Utils::Theme::PanelTextColorLight);
    QColor tabDark = Utils::creatorTheme()->color(Utils::Theme::BackgroundColorDark);

    /* hack for light themes */
    /* The selected tab is always supposed to be lighter */
    if (tabDark.value() > tabLight.value()) {
        QColor swap = tabLight.lighter(180);
        tabLight = tabDark.darker(110);
        tabDark =  tabDark.darker(260);
    }

    map->insert("QmlDesignerTabLight", tabLight);
    map->insert("QmlDesignerTabDark", tabDark);
}

QString Theming::replaceCssColors(const QString &input)
{
    QQmlPropertyMap map;
    insertTheme(&map);
    QRegExp rx("creatorTheme\\.(\\w+);");

    int pos = 0;
    QString output = input;

    while ((pos = rx.indexIn(input, pos)) != -1) {
          const QString color = rx.cap(1);
          output.replace("creatorTheme." + rx.cap(1), map.value(color).toString());
          pos += rx.matchedLength();
      }

    return output;

}

} // namespace QmlDesigner
