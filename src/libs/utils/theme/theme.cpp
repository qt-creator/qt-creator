/****************************************************************************
**
** Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
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

Theme *proxyTheme()
{
    return new Theme(*(new ThemePrivate(*(m_creatorTheme->d))));
}

void setCreatorTheme(Theme *theme)
{
    if (m_creatorTheme == theme)
        return;
    delete m_creatorTheme;
    m_creatorTheme = theme;
    if (theme && theme->flag(Theme::ApplyThemePaletteGlobally))
        QApplication::setPalette(theme->palette());
}

Theme::Theme(const QString &id, QObject *parent)
  : QObject(parent)
  , d(new ThemePrivate)
{
    d->id = id;
}

Theme::Theme(ThemePrivate &dd, QObject *parent) : QObject(parent), d(&dd)
{
}

Theme::~Theme()
{
    delete d;
}

QStringList Theme::preferredStyles() const
{
    return d->preferredStyles;
}

QString Theme::defaultTextEditorColorScheme() const
{
    return d->defaultTextEditorColorScheme;
}

QString Theme::id() const
{
    return d->id;
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

QString Theme::displayName() const
{
    return d->displayName;
}

void Theme::setDisplayName(const QString &name)
{
    d->displayName = name;
}

const QVariantMap &Theme::values() const
{
    if (d->values.isEmpty()) {
        const QMetaObject &m = *metaObject();
        {
            const QMetaEnum e = m.enumerator(m.indexOfEnumerator("Color"));
            for (int i = 0, total = e.keyCount(); i < total; ++i) {
                const QString key = QLatin1String(e.key(i));
                const QPair<QColor, QString> &var = d->colors.at(i);
                d->values.insert(key, var.first);
            }
        }
        {
            const QMetaEnum e = m.enumerator(m.indexOfEnumerator("Flag"));
            for (int i = 0, total = e.keyCount(); i < total; ++i) {
                const QString key = QLatin1String(e.key(i));
                d->values.insert(key, flag(static_cast<Theme::Flag>(i)));
            }
        }
    }
    return d->values;
}

static QColor readColor(const QString &color)
{
    bool ok = true;
    const QRgb rgba = color.toLongLong(&ok, 16);
    return QColor::fromRgba(rgba);
}

void Theme::readSettings(QSettings &settings)
{
    d->fileName = settings.fileName();
    const QMetaObject &m = *metaObject();

    {
        d->displayName = settings.value(QLatin1String("ThemeName"), QLatin1String("unnamed")).toString();
        d->preferredStyles = settings.value(QLatin1String("PreferredStyles")).toStringList();
        d->preferredStyles.removeAll(QLatin1String(""));
        d->defaultTextEditorColorScheme =
                settings.value(QLatin1String("DefaultTextEditorColorScheme")).toString();
    }
    {
        settings.beginGroup(QLatin1String("Palette"));
        foreach (const QString &key, settings.allKeys())
            d->palette[key] = readNamedColor(settings.value(key).toString()).first;
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("Colors"));
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("Color"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            if (!settings.contains(key)) {
                if (i < PaletteWindow || i > PaletteShadowDisabled)
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

    const static struct {
        Color themeColor;
        QPalette::ColorRole paletteColorRole;
        QPalette::ColorGroup paletteColorGroup;
        bool setColorRoleAsBrush;
    } mapping[] = {
        { PaletteWindow,                    QPalette::Window,           QPalette::All,      false},
        { PaletteWindowDisabled,            QPalette::Window,           QPalette::Disabled, false},
        { PaletteWindowText,                QPalette::WindowText,       QPalette::All,      true},
        { PaletteWindowTextDisabled,        QPalette::WindowText,       QPalette::Disabled, true},
        { PaletteBase,                      QPalette::Base,             QPalette::All,      false},
        { PaletteBaseDisabled,              QPalette::Base,             QPalette::Disabled, false},
        { PaletteAlternateBase,             QPalette::AlternateBase,    QPalette::All,      false},
        { PaletteAlternateBaseDisabled,     QPalette::AlternateBase,    QPalette::Disabled, false},
        { PaletteToolTipBase,               QPalette::ToolTipBase,      QPalette::All,      true},
        { PaletteToolTipBaseDisabled,       QPalette::ToolTipBase,      QPalette::Disabled, true},
        { PaletteToolTipText,               QPalette::ToolTipText,      QPalette::All,      false},
        { PaletteToolTipTextDisabled,       QPalette::ToolTipText,      QPalette::Disabled, false},
        { PaletteText,                      QPalette::Text,             QPalette::All,      true},
        { PaletteTextDisabled,              QPalette::Text,             QPalette::Disabled, true},
        { PaletteButton,                    QPalette::Button,           QPalette::All,      false},
        { PaletteButtonDisabled,            QPalette::Button,           QPalette::Disabled, false},
        { PaletteButtonText,                QPalette::ButtonText,       QPalette::All,      true},
        { PaletteButtonTextDisabled,        QPalette::ButtonText,       QPalette::Disabled, true},
        { PaletteBrightText,                QPalette::BrightText,       QPalette::All,      false},
        { PaletteBrightTextDisabled,        QPalette::BrightText,       QPalette::Disabled, false},
        { PaletteHighlight,                 QPalette::Highlight,        QPalette::All,      true},
        { PaletteHighlightDisabled,         QPalette::Highlight,        QPalette::Disabled, true},
        { PaletteHighlightedText,           QPalette::HighlightedText,  QPalette::All,      true},
        { PaletteHighlightedTextDisabled,   QPalette::HighlightedText,  QPalette::Disabled, true},
        { PaletteLink,                      QPalette::Link,             QPalette::All,      false},
        { PaletteLinkDisabled,              QPalette::Link,             QPalette::Disabled, false},
        { PaletteLinkVisited,               QPalette::LinkVisited,      QPalette::All,      false},
        { PaletteLinkVisitedDisabled,       QPalette::LinkVisited,      QPalette::Disabled, false},
        { PaletteLight,                     QPalette::Light,            QPalette::All,      false},
        { PaletteLightDisabled,             QPalette::Light,            QPalette::Disabled, false},
        { PaletteMidlight,                  QPalette::Midlight,         QPalette::All,      false},
        { PaletteMidlightDisabled,          QPalette::Midlight,         QPalette::Disabled, false},
        { PaletteDark,                      QPalette::Dark,             QPalette::All,      false},
        { PaletteDarkDisabled,              QPalette::Dark,             QPalette::Disabled, false},
        { PaletteMid,                       QPalette::Mid,              QPalette::All,      false},
        { PaletteMidDisabled,               QPalette::Mid,              QPalette::Disabled, false},
        { PaletteShadow,                    QPalette::Shadow,           QPalette::All,      false},
        { PaletteShadowDisabled,            QPalette::Shadow,           QPalette::Disabled, false}
    };

    for (auto entry: mapping) {
        const QColor themeColor = color(entry.themeColor);
        // Use original color if color is not defined in theme.
        if (themeColor.isValid()) {
            if (entry.setColorRoleAsBrush)
                // TODO: Find out why sometimes setBrush is used
                pal.setBrush(entry.paletteColorGroup, entry.paletteColorRole, themeColor);
            else
                pal.setColor(entry.paletteColorGroup, entry.paletteColorRole, themeColor);
        }
    }

    return pal;
}

} // namespace Utils
