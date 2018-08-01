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

#include <QApplication>
#include <QRegExp>
#include <QScreen>
#include <QPointer>
#include <qqml.h>

namespace QmlDesigner {

Theme::Theme(Utils::Theme *originTheme, QObject *parent)
    : Utils::Theme(originTheme, parent)
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

    qWarning() << Q_FUNC_INFO << "error while evaluating" << themeColorName;
    return {};
}

Theme *Theme::instance()
{
    static QPointer<Theme> qmldesignerTheme =
        new Theme(Utils::creatorTheme(), QmlDesigner::QmlDesignerPlugin::instance());
    return qmldesignerTheme;
}

QString Theme::replaceCssColors(const QString &input)
{
    QRegExp rx("creatorTheme\\.(\\w+)");

    int pos = 0;
    QString output = input;

    while ((pos = rx.indexIn(input, pos)) != -1) {
        const QString themeColorName = rx.cap(1);

        if (themeColorName == "smallFontPixelSize") {
            output.replace("creatorTheme." + themeColorName, QString::number(instance()->smallFontPixelSize()) + "px");
        } else if (themeColorName == "captionFontPixelSize") {
            output.replace("creatorTheme." + themeColorName, QString::number(instance()->captionFontPixelSize()) + "px");
        } else {
            const QColor color = instance()->evaluateColorAtThemeInstance(themeColorName);
            output.replace("creatorTheme." + rx.cap(1), color.name());
        }
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

QColor Theme::getColor(Theme::Color role)
{

    return instance()->color(role);

}

int Theme::smallFontPixelSize() const
{
    if (highPixelDensity())
        return 13;
    return 9;
}

int Theme::captionFontPixelSize() const
{
    if (highPixelDensity())
        return 14;
    return 11;
}

bool Theme::highPixelDensity() const
{
    return qApp->primaryScreen()->logicalDotsPerInch() > 100;
}

QPixmap Theme::getPixmap(const QString &id)
{
    return QmlDesignerIconProvider::getPixmap(id);
}

QColor Theme::qmlDesignerBackgroundColorDarker() const
{
    return getColor(QmlDesigner_BackgroundColorDarker);
}

QColor Theme::qmlDesignerBackgroundColorDarkAlternate() const
{
    return getColor(QmlDesigner_BackgroundColorDarkAlternate);
}

QColor Theme::qmlDesignerTabLight() const
{
    return getColor(QmlDesigner_TabLight);
}

QColor Theme::qmlDesignerTabDark() const
{
    return getColor(QmlDesigner_TabDark);
}

QColor Theme::qmlDesignerButtonColor() const
{
    return getColor(QmlDesigner_ButtonColor);
}

QColor Theme::qmlDesignerBorderColor() const
{
    return getColor(QmlDesigner_BorderColor);
}

} // namespace QmlDesigner
