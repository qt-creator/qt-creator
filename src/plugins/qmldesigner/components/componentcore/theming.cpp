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
#include "qmldesignericonprovider.h"

#include <utils/theme/theme.h>
#include <utils/stylehelper.h>

#include <QRegExp>

namespace QmlDesigner {

const QVariantMap &Theming::theme()
{
    static QVariantMap map;
    if (!map.isEmpty())
        return map;

    map = Utils::creatorTheme()->values();

    /* Define QmlDesigner colors and remove alpha channels */
    const QColor backgroundColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_BackgroundColor);
    const QColor panelStatusBarBackgroundColor = Utils::creatorTheme()->color(Utils::Theme::PanelStatusBarBackgroundColor);
    const QColor fancyToolButtonSelectedColor  = Utils::creatorTheme()->color(Utils::Theme::FancyToolButtonSelectedColor);
    const QColor darkerBackground = Utils::StyleHelper::alphaBlendedColors(panelStatusBarBackgroundColor, fancyToolButtonSelectedColor);
    const QColor fancyToolButtonHoverColor  = Utils::creatorTheme()->color(Utils::Theme::FancyToolButtonHoverColor);
    const QColor buttonColor = Utils::StyleHelper::alphaBlendedColors(panelStatusBarBackgroundColor, fancyToolButtonHoverColor);

    Utils::creatorTheme()->color(Utils::Theme::PanelTextColorLight);
    QColor tabLight = Utils::creatorTheme()->color(Utils::Theme::PanelTextColorLight);
    QColor tabDark = Utils::creatorTheme()->color(Utils::Theme::BackgroundColorDark);

    /* hack for light themes */
    /* The selected tab is always supposed to be lighter */
    if (tabDark.value() > tabLight.value()) {
        tabLight = tabDark.darker(110);
        tabDark =  tabDark.darker(260);
    }

    map.insert("QmlDesignerBackgroundColorDarker", darkerBackground);
    map.insert("QmlDesignerBackgroundColorDarkAlternate", backgroundColor);
    map.insert("QmlDesignerTabLight", tabLight);
    map.insert("QmlDesignerTabDark", tabDark);
    map.insert("QmlDesignerButtonColor", buttonColor);
    map.insert("QmlDesignerBorderColor", Utils::creatorTheme()->color(Utils::Theme::SplitterColor));
    return map;
}

QString Theming::replaceCssColors(const QString &input)
{
    const QVariantMap &map = theme();
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

void Theming::registerIconProvider(QQmlEngine *engine)
{
    engine->addImageProvider(QLatin1String("icons"), new QmlDesignerIconProvider());
}

} // namespace QmlDesigner
