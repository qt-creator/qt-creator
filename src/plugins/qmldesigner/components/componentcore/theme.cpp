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

#include "theme.h"
#include "qmldesignericonprovider.h"

#include <qmldesignerplugin.h>

#include <utils/stylehelper.h>

#include <QRegExp>
#include <QPointer>
#include <qqml.h>

namespace {
QMap<QString, QColor> createDerivedDesignerColors()
{
    /* Define QmlDesigner colors and remove alpha channels */
    const QColor backgroundColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_BackgroundColor);
    const QColor panelStatusBarBackgroundColor = Utils::creatorTheme()->color(Utils::Theme::PanelStatusBarBackgroundColor);
    const QColor fancyToolButtonSelectedColor  = Utils::creatorTheme()->color(Utils::Theme::FancyToolButtonSelectedColor);
    const QColor darkerBackground = Utils::StyleHelper::alphaBlendedColors(panelStatusBarBackgroundColor, fancyToolButtonSelectedColor);
    const QColor fancyToolButtonHoverColor  = Utils::creatorTheme()->color(Utils::Theme::FancyToolButtonHoverColor);
    const QColor buttonColor = Utils::StyleHelper::alphaBlendedColors(panelStatusBarBackgroundColor, fancyToolButtonHoverColor);

    QColor tabLight = Utils::creatorTheme()->color(Utils::Theme::PanelTextColorLight);
    QColor tabDark = Utils::creatorTheme()->color(Utils::Theme::BackgroundColorDark);

    /* hack for light themes */
    /* The selected tab is always supposed to be lighter */
    if (tabDark.value() > tabLight.value()) {
        tabLight = tabDark.darker(110);
        tabDark = tabDark.darker(260);
    }
    return {{"QmlDesignerBackgroundColorDarker", darkerBackground},
            {"QmlDesignerBackgroundColorDarkAlternate", backgroundColor},
            {"QmlDesignerTabLight", tabLight},
            {"QmlDesignerTabDark", tabDark},
            {"QmlDesignerButtonColor", buttonColor},
            {"QmlDesignerBorderColor", Utils::creatorTheme()->color(Utils::Theme::SplitterColor)}
    };
}

} // namespace

namespace QmlDesigner {

Theme::Theme(Utils::Theme *originTheme, QObject *parent)
    : Utils::Theme(originTheme, parent)
    , m_derivedColors(createDerivedDesignerColors())
{
}

QColor Theme::evaluateColorAtThemeInstance(const QString &themeColorName)
{
    const QMetaObject &m = *metaObject();
    const QMetaEnum e = m.enumerator(m.indexOfEnumerator("Color"));
    for (int i = 0, total = e.keyCount(); i < total; ++i) {
        if (QString::fromLatin1(e.key(i)) == themeColorName)
            return color(static_cast<Utils::Theme::Color>(i)).name();
    }

    qWarning() << "error while evaluate " << themeColorName;
    return QColor();
}

Theme *Theme::instance()
{
    static QPointer<Theme> qmldesignerTheme =
        new Theme(Utils::creatorTheme(), QmlDesigner::QmlDesignerPlugin::instance());
    return qmldesignerTheme;
}

QString Theme::replaceCssColors(const QString &input)
{
    const QMap<QString, QColor> &map = instance()->m_derivedColors;
    QRegExp rx("creatorTheme\\.(\\w+);");

    int pos = 0;
    QString output = input;

    while ((pos = rx.indexIn(input, pos)) != -1) {
        const QString themeColorName = rx.cap(1);
        QColor color;
        if (map.contains(themeColorName))
            color = map.value(themeColorName);
        else
            color = instance()->evaluateColorAtThemeInstance(themeColorName);
        output.replace("creatorTheme." + rx.cap(1), color.name());
        pos += rx.matchedLength();
    }

    return output;
}

void Theme::setupTheme(QQmlEngine *engine)
{
    static const int typeIndex = qmlRegisterSingletonType<Utils::Theme>("QtQuickDesignerTheme", 1, 0,
        "Theme", [](QQmlEngine *, QJSEngine *) {
            return qobject_cast<QObject*>(new Theme(Utils::creatorTheme(), nullptr));
    });
    Q_UNUSED(typeIndex);

    engine->addImageProvider(QLatin1String("icons"), new QmlDesignerIconProvider());
}

QColor Theme::qmlDesignerBackgroundColorDarker() const
{
    return m_derivedColors.value("QmlDesignerBackgroundColorDarker");
}

QColor Theme::qmlDesignerBackgroundColorDarkAlternate() const
{
    return m_derivedColors.value("QmlDesignerBackgroundColorDarkAlternate");
}

QColor Theme::qmlDesignerTabLight() const
{
    return m_derivedColors.value("QmlDesignerTabLight");
}

QColor Theme::qmlDesignerTabDark() const
{
    return m_derivedColors.value("QmlDesignerTabDark");
}

QColor Theme::qmlDesignerButtonColor() const
{
    return m_derivedColors.value("QmlDesignerButtonColor");
}

QColor Theme::qmlDesignerBorderColor() const
{
    return m_derivedColors.value("QmlDesignerBorderColor");
}

} // namespace QmlDesigner
