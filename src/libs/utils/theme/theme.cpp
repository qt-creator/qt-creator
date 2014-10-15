/****************************************************************************
**
** Copyright (C) 2014 Thorben Kroeger <thorbenkroeger@gmail.com>.
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "theme.h"
#include "theme_p.h"
#include "../qtcassert.h"

#include <QApplication>
#include <QFileInfo>
#include <QMetaEnum>
#include <QSettings>

namespace Utils {

static Theme *m_creatorTheme = 0;

Theme *creatorTheme()
{
    return m_creatorTheme;
}

void setCreatorTheme(Theme *theme)
{
    // TODO: memory management of theme object
    m_creatorTheme = theme;
}

Theme::Theme(QObject *parent)
  : QObject(parent)
  , d(new ThemePrivate)
{
}

Theme::~Theme()
{
    delete d;
}

void Theme::drawIndicatorBranch(QPainter *painter, const QRect &rect, QStyle::State state) const
{
    Q_UNUSED(painter);
    Q_UNUSED(rect);
    Q_UNUSED(state);
}

Theme::WidgetStyle Theme::widgetStyle() const
{
    return d->widgetStyle;
}

bool Theme::flag(Theme::Flag f) const
{
    return d->flags[f];
}

QColor Theme::color(Theme::ColorRole role) const
{
    return d->colors[role].first;
}

QGradientStops Theme::gradient(Theme::GradientRole role) const
{
    return d->gradientStops[role];
}

QString Theme::iconOverlay(Theme::MimeType mimetype) const
{
    return d->iconOverlays[mimetype];
}

QString Theme::dpiSpecificImageFile(const QString &fileName) const
{
    return dpiSpecificImageFile(fileName, QLatin1String(""));
}

QString Theme::dpiSpecificImageFile(const QString &fileName, const QString &themePrefix) const
{
    // See QIcon::addFile()
    const QFileInfo fi(fileName);

    bool at2x = (qApp->devicePixelRatio() > 1.0);

    const QString at2xFileName = fi.path() + QStringLiteral("/")
        + fi.completeBaseName() + QStringLiteral("@2x.") + fi.suffix();
    const QString themedAt2xFileName = fi.path() + QStringLiteral("/") + themePrefix
        + fi.completeBaseName() + QStringLiteral("@2x.") + fi.suffix();
    const QString themedFileName = fi.path() + QStringLiteral("/")  + themePrefix
        + fi.completeBaseName() + QStringLiteral(".")    + fi.suffix();

    if (at2x) {
        if (QFile::exists(themedAt2xFileName))
            return themedAt2xFileName;
        else if (QFile::exists(themedFileName))
            return themedFileName;
        else if (QFile::exists(at2xFileName))
            return at2xFileName;
        return fileName;
    } else {
        if (QFile::exists(themedFileName))
            return themedFileName;
        return fileName;
    }
}

QPair<QColor, QString> Theme::readNamedColor(const QString &color) const
{
    if (d->palette.contains(color))
        return qMakePair(d->palette[color], color);

    bool ok = true;
    const QRgb rgba = color.toLongLong(&ok, 16);
    if (!ok) {
        qWarning("Color '%s' is neither a named color nor a valid color", qPrintable(color));
        return qMakePair(Qt::black, QString());
    }
    return qMakePair(QColor::fromRgba(rgba), QString());
}

QString Theme::imageFile(const QString &fileName) const
{
    return fileName;
}

QString Theme::fileName() const
{
    return d->fileName;
}

void Theme::setName(const QString &name)
{
    d->name = name;
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
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("ColorRole"));
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
        settings.beginGroup(QLatin1String("Gradients"));
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("GradientRole"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            QGradientStops stops = gradient(static_cast<Theme::GradientRole>(i));
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
        settings.beginGroup(QLatin1String("IconOverlay"));
        const QMetaEnum e = m.enumerator(m.indexOfEnumerator("MimeType"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            settings.setValue(key, iconOverlay(static_cast<Theme::MimeType>(i)));
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
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("ColorRole"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            QTC_ASSERT(settings.contains(key), return);;
            d->colors[i] = readNamedColor(settings.value(key).toString());
        }
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("Gradients"));
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("GradientRole"));
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
            d->gradientStops[i] = stops;
        }
        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("IconOverlay"));
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("MimeType"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            QTC_ASSERT(settings.contains(key), return);;
            d->iconOverlays[i] = settings.value(key).toString();
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

QIcon Theme::standardIcon(QStyle::StandardPixmap standardPixmap, const QStyleOption *opt, const QWidget *widget) const
{
    Q_UNUSED(standardPixmap);
    Q_UNUSED(opt);
    Q_UNUSED(widget);
    return QIcon();
}

QPalette Theme::palette(const QPalette &base) const
{
    if (!flag(DerivePaletteFromTheme))
        return base;

    // FIXME: introduce some more color roles for this

    QPalette pal = base;
    pal.setColor(QPalette::All, QPalette::Window,        color(Theme::BackgroundColorNormal));
    pal.setBrush(QPalette::All, QPalette::WindowText,    color(Theme::TextColorNormal));
    pal.setColor(QPalette::All, QPalette::Base,          color(Theme::BackgroundColorNormal));
    pal.setColor(QPalette::All, QPalette::AlternateBase, color(Theme::BackgroundColorAlternate));
    pal.setColor(QPalette::All, QPalette::Button,        color(Theme::BackgroundColorDark));
    pal.setColor(QPalette::All, QPalette::BrightText,    Qt::red);
    pal.setBrush(QPalette::All, QPalette::Text,          color(Theme::TextColorNormal));
    pal.setBrush(QPalette::All, QPalette::ButtonText,    color(Theme::TextColorNormal));
    pal.setBrush(QPalette::All, QPalette::ToolTipBase,   color(Theme::BackgroundColorSelected));
    pal.setColor(QPalette::Highlight,                    color(Theme::BackgroundColorSelected));
    pal.setColor(QPalette::HighlightedText,              Qt::white);
    pal.setColor(QPalette::ToolTipText,                  color(Theme::TextColorNormal));
    return pal;
}

} // namespace Utils
