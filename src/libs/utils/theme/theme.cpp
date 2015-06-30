/****************************************************************************
**
** Copyright (C) 2015 Thorben Kroeger <thorbenkroeger@gmail.com>.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "theme.h"
#include "theme_p.h"
#include "../algorithm.h"
#include "../qtcassert.h"

#include <QApplication>
#include <QFileInfo>
#include <QMetaEnum>
#include <QPalette>
#include <QSettings>

namespace Utils {

static Theme *m_creatorTheme = 0;

ThemePrivate::ThemePrivate()
    : widgetStyle(Theme::StyleDefault)
{
    const QMetaObject &m = Theme::staticMetaObject;
    colors.resize        (m.enumerator(m.indexOfEnumerator("Color")).keyCount());
    imageFiles.resize    (m.enumerator(m.indexOfEnumerator("ImageFile")).keyCount());
    gradients.resize     (m.enumerator(m.indexOfEnumerator("Gradient")).keyCount());
    flags.resize         (m.enumerator(m.indexOfEnumerator("Flag")).keyCount());
}

Theme *creatorTheme()
{
    return m_creatorTheme;
}

void setCreatorTheme(Theme *theme)
{
    if (m_creatorTheme == theme)
        return;
    delete m_creatorTheme;
    m_creatorTheme = theme;
}

Theme::Theme(const QString &name, QObject *parent)
  : QObject(parent)
  , d(new ThemePrivate)
{
    d->name = name;
}

Theme::~Theme()
{
    delete d;
}

Theme::WidgetStyle Theme::widgetStyle() const
{
    return d->widgetStyle;
}

QStringList Theme::preferredStyles() const
{
    return d->preferredStyles;
}

bool Theme::flag(Theme::Flag f) const
{
    return d->flags[f];
}

QColor Theme::color(Theme::Color role) const
{
    return d->colors[role].first;
}

QString Theme::imageFile(Theme::ImageFile imageFile, const QString &fallBack) const
{
    const QString &file = d->imageFiles.at(imageFile);
    return file.isEmpty() ? fallBack : file;
}

QGradientStops Theme::gradient(Theme::Gradient role) const
{
    return d->gradients[role];
}

QPair<QColor, QString> Theme::readNamedColor(const QString &color) const
{
    if (d->palette.contains(color))
        return qMakePair(d->palette[color], color);
    if (color == QLatin1String("style"))
        return qMakePair(QColor(), QString());

    bool ok = true;
    const QRgb rgba = color.toLongLong(&ok, 16);
    if (!ok) {
        qWarning("Color \"%s\" is neither a named color nor a valid color", qPrintable(color));
        return qMakePair(Qt::black, QString());
    }
    return qMakePair(QColor::fromRgba(rgba), QString());
}

QString Theme::filePath() const
{
    return d->fileName;
}

QString Theme::name() const
{
    return d->name;
}

void Theme::setName(const QString &name)
{
    d->name = name;
}

QVariantHash Theme::values() const
{
    QVariantHash result;
    const QMetaObject &m = *metaObject();
    {
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("Color"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            const QPair<QColor, QString> &var = d->colors.at(i);
            result.insert(key, var.first);
        }
    }
    {
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("Flag"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            result.insert(key, flag(static_cast<Theme::Flag>(i)));
        }
    }
    {
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("WidgetStyle"));
        result.insert(QLatin1String("WidgetStyle"), QLatin1String(e.valueToKey(widgetStyle())));
    }
    return result;
}

static QColor readColor(const QString &color)
{
    bool ok = true;
    const QRgb rgba = color.toLongLong(&ok, 16);
    return QColor::fromRgba(rgba);
}

static QString writeColor(const QColor &color)
{
    return QString::number(color.rgba(), 16);
}

// reading, writing of .creatortheme ini file ////////////////////////////////
void Theme::writeSettings(const QString &filename) const
{
    QSettings settings(filename, QSettings::IniFormat);

    const QMetaObject &m = *metaObject();
    {
        settings.setValue(QLatin1String("ThemeName"), d->name);
        settings.setValue(QLatin1String("PreferredStyles"), d->preferredStyles);
    }
    {
        settings.beginGroup(QLatin1String("Palette"));
        for (int i = 0, total = d->colors.size(); i < total; ++i) {
            const QPair<QColor, QString> var = d->colors[i];
            if (var.second.isEmpty())
                continue;
            settings.setValue(var.second, writeColor(var.first));
        }
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("Colors"));
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("Color"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            const QPair<QColor, QString> var = d->colors[i];
            if (!var.second.isEmpty())
                settings.setValue(key, var.second); // named color
            else
                settings.setValue(key, writeColor(var.first));
        }
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("ImageFiles"));
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("ImageFile"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            const QString &var = d->imageFiles.at(i);
            if (!var.isEmpty())
                settings.setValue(key, var);
        }
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("Gradients"));
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("Gradient"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            QGradientStops stops = gradient(static_cast<Theme::Gradient>(i));
            settings.beginWriteArray(key);
            int k = 0;
            foreach (const QGradientStop stop, stops) {
                settings.setArrayIndex(k);
                settings.setValue(QLatin1String("pos"), stop.first);
                settings.setValue(QLatin1String("color"), writeColor(stop.second));
                ++k;
            }
            settings.endArray();
        }
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("Flags"));
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("Flag"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            settings.setValue(key, flag(static_cast<Theme::Flag>(i)));
        }
        settings.endGroup();
    }

    {
        settings.beginGroup(QLatin1String("Style"));
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("WidgetStyle"));
        settings.setValue(QLatin1String("WidgetStyle"), QLatin1String(e.valueToKey(widgetStyle ())));
        settings.endGroup();
    }
}

void Theme::readSettings(QSettings &settings)
{
    d->fileName = settings.fileName();
    const QMetaObject &m = *metaObject();

    {
        d->name = settings.value(QLatin1String("ThemeName"), QLatin1String("unnamed")).toString();
        d->preferredStyles = settings.value(QLatin1String("PreferredStyles")).toStringList();
    }
    {
        settings.beginGroup(QLatin1String("Palette"));
        foreach (const QString &key, settings.allKeys()) {
            QColor c = readColor(settings.value(key).toString());
            d->palette[key] = c;
        }
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("Style"));
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("WidgetStyle"));
        QString val = settings.value(QLatin1String("WidgetStyle")).toString();
        d->widgetStyle = static_cast<Theme::WidgetStyle>(e.keysToValue (val.toLatin1().data()));
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("Colors"));
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("Color"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            if (!settings.contains(key)) {
                qWarning("Theme \"%s\" misses color setting for key \"%s\".",
                         qPrintable(d->fileName), qPrintable(key));
                continue;
            }
            d->colors[i] = readNamedColor(settings.value(key).toString());
        }
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("ImageFiles"));
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("ImageFile"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            d->imageFiles[i] = settings.value(key).toString();
        }
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("Gradients"));
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("Gradient"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            QGradientStops stops;
            int size = settings.beginReadArray(key);
            for (int j = 0; j < size; ++j) {
                settings.setArrayIndex(j);
                QTC_ASSERT(settings.contains(QLatin1String("pos")), return);;
                double pos = settings.value(QLatin1String("pos")).toDouble();
                QTC_ASSERT(settings.contains(QLatin1String("color")), return);;
                QColor c = readColor(settings.value(QLatin1String("color")).toString());
                stops.append(qMakePair(pos, c));
            }
            settings.endArray();
            d->gradients[i] = stops;
        }
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("Flags"));
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("Flag"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            QTC_ASSERT(settings.contains(key), return);;
            d->flags[i] = settings.value(key).toBool();
        }
        settings.endGroup();
    }
}

QPalette Theme::initialPalette()
{
    static QPalette palette = QApplication::palette();
    return palette;
}

QPalette Theme::palette() const
{
    QPalette pal = initialPalette();
    if (!flag(DerivePaletteFromTheme))
        return pal;

    // FIXME: introduce some more color roles for this

    pal.setColor(QPalette::Window,          color(Theme::BackgroundColorNormal));
    pal.setBrush(QPalette::WindowText,      color(Theme::TextColorNormal));
    pal.setColor(QPalette::Base,            color(Theme::BackgroundColorNormal));
    pal.setColor(QPalette::AlternateBase,   color(Theme::BackgroundColorAlternate));
    pal.setColor(QPalette::Button,          color(Theme::BackgroundColorDark));
    pal.setColor(QPalette::BrightText,      Qt::red);
    pal.setBrush(QPalette::Text,            color(Theme::TextColorNormal));
    pal.setBrush(QPalette::ButtonText,      color(Theme::TextColorNormal));
    pal.setBrush(QPalette::ToolTipBase,     color(Theme::BackgroundColorSelected));
    pal.setColor(QPalette::Highlight,       color(Theme::BackgroundColorSelected));
    pal.setColor(QPalette::Dark,            color(Theme::BackgroundColorDark));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    pal.setColor(QPalette::ToolTipText,     color(Theme::TextColorNormal));
    pal.setColor(QPalette::Link,            color(Theme::TextColorLink));
    pal.setColor(QPalette::LinkVisited,     color(Theme::TextColorLinkVisited));
    pal.setColor(QPalette::Disabled, QPalette::Window,     color(Theme::BackgroundColorDisabled));
    pal.setBrush(QPalette::Disabled, QPalette::WindowText, color(Theme::TextColorDisabled));
    pal.setColor(QPalette::Disabled, QPalette::Base,       color(Theme::BackgroundColorDisabled));
    pal.setBrush(QPalette::Disabled, QPalette::Text,       color(Theme::TextColorDisabled));
    return pal;
}

} // namespace Utils
