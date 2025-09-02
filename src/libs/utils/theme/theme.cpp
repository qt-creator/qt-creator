// Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "theme.h"
#include "theme_p.h"
#include "../hostosinfo.h"
#include "../qtcassert.h"
#include "filepath.h"
#include "stylehelper.h"
#ifdef Q_OS_MACOS
#import "theme_mac.h"
#endif

#include <QApplication>
#include <QMetaEnum>
#include <QPalette>
#include <QSettings>
#include <QStyleHints>

namespace Utils {

static Theme *m_creatorTheme = nullptr;
static std::optional<QPalette> m_initialPalette;

ThemePrivate::ThemePrivate()
    : defaultToolbarStyle(StyleHelper::ToolbarStyle::Compact)
{
    const QMetaObject &m = Theme::staticMetaObject;
    colors.resize        (m.enumerator(m.indexOfEnumerator("Color")).keyCount());
    imageFiles.resize    (m.enumerator(m.indexOfEnumerator("ImageFile")).keyCount());
    flags.resize         (m.enumerator(m.indexOfEnumerator("Flag")).keyCount());
}

Theme *creatorTheme()
{
    return m_creatorTheme;
}

// Convenience
QColor creatorColor(Theme::Color role)
{
    return m_creatorTheme->color(role);
}

static bool isOverridingPalette(const Theme *theme)
{
    if (theme->flag(Theme::DerivePaletteFromTheme))
        return true;
    if (theme->flag(Theme::DerivePaletteFromThemeIfNeeded)) {
        const Qt::ColorScheme systemTheme = qGuiApp->styleHints()->colorScheme();
        return systemTheme != Qt::ColorScheme::Unknown && systemTheme != theme->colorScheme();
    }
    return false;
}

void setThemeApplicationPalette()
{
    if (m_creatorTheme && isOverridingPalette(m_creatorTheme))
        QApplication::setPalette(m_creatorTheme->palette());
}

static void setMacAppearance(Theme *theme)
{
#ifdef Q_OS_MACOS
    // Match the native UI theme and palette with the creator
    // theme by forcing light aqua for light creator themes
    // and dark aqua for dark themes.
    if (theme)
        Internal::forceMacAppearance(theme->colorScheme() == Qt::ColorScheme::Dark);
#else
    Q_UNUSED(theme)
#endif
}

void setCreatorTheme(Theme *theme)
{
    if (m_creatorTheme == theme)
        return;
    delete m_creatorTheme;
    m_creatorTheme = theme;

    setMacAppearance(theme);
    setThemeApplicationPalette();
}

Theme::Theme(const QString &id, QObject *parent)
  : QObject(parent)
  , d(new ThemePrivate)
{
    d->id = id;
}

Theme::Theme(Theme *originTheme, QObject *parent)
    : QObject(parent)
    , d(new ThemePrivate(*(originTheme->d)))
{
}

Theme::~Theme()
{
    if (this == m_creatorTheme)
        m_creatorTheme = nullptr;

    delete d;
}

QStringList Theme::preferredStyles() const
{
    // Force Fusion style if we have a dark theme on Windows or Linux,
    // because the default QStyle might not be up for it
    if (!HostOsInfo::isMacHost() && d->preferredStyles.isEmpty()
        && colorScheme() == Qt::ColorScheme::Dark)
        return {"Fusion"};
    return d->preferredStyles;
}

QString Theme::defaultTextEditorColorScheme() const
{
    return d->defaultTextEditorColorScheme;
}

StyleHelper::ToolbarStyle Theme::defaultToolbarStyle() const
{
    return d->defaultToolbarStyle;
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
    const auto color = d->colors[role];
    if (HostOsInfo::isMacHost() && !d->enforceAccentColorOnMacOS.isEmpty()
        && color.second == d->enforceAccentColorOnMacOS)
        return initialPalette().color(QPalette::Highlight);
    return d->colors[role].first;
}

QString Theme::imageFile(Theme::ImageFile imageFile, const QString &fallBack) const
{
    const QString &file = d->imageFiles.at(imageFile);
    return file.isEmpty() ? fallBack : file;
}

QColor Theme::readNamedColorNoWarning(const QString &color) const
{
    const auto it = d->palette.constFind(color);
    if (it != d->palette.constEnd())
        return it.value();
    if (color == QLatin1String("style"))
        return {};

    const QColor col('#' + color);
    if (!col.isValid()) {
        return {};
    }
    return {col};
}

QPair<QColor, QString> Theme::readNamedColor(const QString &color) const
{
    const auto it = d->palette.constFind(color);
    if (it != d->palette.constEnd())
        return {it.value(), color};
    if (color == QLatin1String("style"))
        return {};

    const QColor col('#' + color);
    if (!col.isValid()) {
        qWarning("Color \"%s\" is neither a named color nor a valid color", qPrintable(color));
        return {Qt::black, {}};
    }
    return {col, {}};
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

void Theme::readSettingsInternal(QSettings &settings)
{
    const QStringList includes = settings.value("Includes").toStringList();

    for (const QString &include : includes) {
        FilePath path = FilePath::fromString(d->fileName);
        const Utils::FilePath includedPath = path.parentDir().pathAppended(include);

        if (includedPath.exists()) {
            QSettings themeSettings(includedPath.toUrlishString(), QSettings::IniFormat);
            readSettingsInternal(themeSettings);
        } else {
            qWarning("Theme \"%s\" misses include \"%s\".",
                     qPrintable(d->fileName),
                     qPrintable(includedPath.toUserOutput()));
        }
    }

    const QMetaObject &m = *metaObject();

    {
        d->displayName = settings.value(QLatin1String("ThemeName"), QLatin1String("unnamed"))
                             .toString();
        d->preferredStyles = settings.value(QLatin1String("PreferredStyles")).toStringList();
        d->preferredStyles.removeAll(QString());
        d->defaultTextEditorColorScheme
            = settings.value(QLatin1String("DefaultTextEditorColorScheme")).toString();
        d->defaultToolbarStyle =
                settings.value(QLatin1String("DefaultToolbarStyle")).toString() == "Relaxed"
                ? StyleHelper::ToolbarStyle::Relaxed : StyleHelper::ToolbarStyle::Compact;
        d->enforceAccentColorOnMacOS = settings.value("EnforceAccentColorOnMacOS").toString();
    }

    {
        settings.beginGroup(QLatin1String("Palette"));
        const QStringList allKeys = settings.allKeys();
        for (const QString &key : allKeys) {
            d->unresolvedPalette[key] = settings.value(key).toString();
        }

        settings.endGroup();
    }
    {
        settings.beginGroup(QLatin1String("Colors"));
        const QStringList allKeys = settings.allKeys();
        for (const QString &key : allKeys) {
            d->unresolvedPalette[key] = settings.value(key).toString();
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
        settings.beginGroup(QLatin1String("Flags"));
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("Flag"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            d->flags[i] = settings.value(key).toBool();
        }
        settings.endGroup();
    }
}

void Theme::readSettings(QSettings &settings)
{
    d->fileName = settings.fileName();

    readSettingsInternal(settings);

    int oldInvalidColors = 1;
    int unresolvedColors = 0;

    const QStringList allKeys = d->unresolvedPalette.keys();
    do {
        oldInvalidColors = unresolvedColors;
        unresolvedColors = 0;
        for (const QString &key : allKeys) {
            const QColor currentColor = d->palette[key];
            if (currentColor.isValid())
                continue;
            QColor color = readNamedColorNoWarning(d->unresolvedPalette.value(key));
            if (!color.isValid())
                ++unresolvedColors;
            d->palette[key] = color;
        }

        //If all colors are resolved or in the last step no new color has been resolved break.
    } while (unresolvedColors > 0 && oldInvalidColors != unresolvedColors);

    if (unresolvedColors > 0) { //Show warnings for unresolved colors ad set them to black.
        for (const QString &key : allKeys)
            d->palette[key] = readNamedColor(d->unresolvedPalette.value(key)).first;
    }

    const QMetaObject &m = *metaObject();

    QMetaEnum e = m.enumerator(m.indexOfEnumerator("Color"));
    for (int i = 0, total = e.keyCount(); i < total; ++i) {
        const QString key = QLatin1String(e.key(i));
        if (!d->unresolvedPalette.contains(key)) {
            if (i < PaletteWindow || i > PaletteAccentDisabled)
                qWarning("Theme \"%s\" misses color setting for key \"%s\".",
                         qPrintable(d->fileName),
                         qPrintable(key));
            continue;
        }
        d->colors[i] = readNamedColor(d->unresolvedPalette.value(key));
    }
}

Qt::ColorScheme Theme::colorScheme() const
{
    return flag(Theme::DarkUserInterface) ? Qt::ColorScheme::Dark
                                          : Qt::ColorScheme::Light;
}

Qt::ColorScheme Theme::systemColorScheme()
{
    static const Qt::ColorScheme initialColorScheme = qGuiApp->styleHints()->colorScheme();
    return initialColorScheme;
}

// If you copy QPalette, default values stay at default, even if that default is different
// within the context of different widgets. Create deep copy.
static QPalette copyPalette(const QPalette &p)
{
    QPalette res;
    for (int group = 0; group < QPalette::NColorGroups; ++group) {
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            res.setBrush(QPalette::ColorGroup(group),
                         QPalette::ColorRole(role),
                         p.brush(QPalette::ColorGroup(group), QPalette::ColorRole(role)));
        }
    }
    return res;
}

void Theme::setInitialPalette(Theme *initTheme)
{
    systemColorScheme(); // initialize value for system mode
    setMacAppearance(initTheme);
    initialPalette();
}

void Theme::setHelpMenu(QMenu *menu)
{
#ifdef Q_OS_MACOS
    Internal::setMacOSHelpMenu(menu);
#else
    Q_UNUSED(menu)
#endif
}

Result<Theme::Color> Theme::colorToken(const QString &tokenName,
                                             [[maybe_unused]] TokenFlags flags)
{
    const QString colorName = "Token_" + tokenName;
    static const QMetaEnum colorEnum = QMetaEnum::fromType<Theme::Color>();
    bool ok = false;
    const Color result = static_cast<Color>(colorEnum.keyToValue(colorName.toLatin1(), &ok));
    if (!ok)
        return ResultError(QString::fromLatin1("%1 - Color token \"%2\" not found.")
                               .arg(Q_FUNC_INFO).arg(tokenName));
    return result;
}

Theme::Color Theme::highlightFor(Color role)
{
    QTC_ASSERT(creatorTheme(), return role);
    static const QMap<QRgb, Theme::Color> map = {
        { creatorColor(Theme::Token_Text_Muted).rgba(), Theme::Token_Text_Default},
    };
    return map.value(creatorColor(role).rgba(), role);
}

QPalette Theme::initialPalette()
{
    if (!m_initialPalette) {
        m_initialPalette = copyPalette(QApplication::palette());
        QApplication::setPalette(*m_initialPalette);
    }
    return *m_initialPalette;
}

QPalette Theme::palette() const
{
    QPalette pal = initialPalette();
    if (!isOverridingPalette(this))
        return pal;

    const static struct {
        Color themeColor;
        QPalette::ColorRole paletteColorRole;
        QPalette::ColorGroup paletteColorGroup;
        bool setColorRoleAsBrush;
    } mapping[] = {
        {PaletteWindow,                    QPalette::Window,           QPalette::All,      false},
        {PaletteWindowDisabled,            QPalette::Window,           QPalette::Disabled, false},
        {PaletteWindowText,                QPalette::WindowText,       QPalette::All,      true},
        {PaletteWindowTextDisabled,        QPalette::WindowText,       QPalette::Disabled, true},
        {PaletteBase,                      QPalette::Base,             QPalette::All,      false},
        {PaletteBaseDisabled,              QPalette::Base,             QPalette::Disabled, false},
        {PaletteAlternateBase,             QPalette::AlternateBase,    QPalette::All,      false},
        {PaletteAlternateBaseDisabled,     QPalette::AlternateBase,    QPalette::Disabled, false},
        {PaletteToolTipBase,               QPalette::ToolTipBase,      QPalette::All,      true},
        {PaletteToolTipBaseDisabled,       QPalette::ToolTipBase,      QPalette::Disabled, true},
        {PaletteToolTipText,               QPalette::ToolTipText,      QPalette::All,      false},
        {PaletteToolTipTextDisabled,       QPalette::ToolTipText,      QPalette::Disabled, false},
        {PaletteText,                      QPalette::Text,             QPalette::All,      true},
        {PaletteTextDisabled,              QPalette::Text,             QPalette::Disabled, true},
        {PaletteButton,                    QPalette::Button,           QPalette::All,      false},
        {PaletteButtonDisabled,            QPalette::Button,           QPalette::Disabled, false},
        {PaletteButtonText,                QPalette::ButtonText,       QPalette::All,      true},
        {PaletteButtonTextDisabled,        QPalette::ButtonText,       QPalette::Disabled, true},
        {PaletteBrightText,                QPalette::BrightText,       QPalette::All,      false},
        {PaletteBrightTextDisabled,        QPalette::BrightText,       QPalette::Disabled, false},
        {PaletteHighlight,                 QPalette::Highlight,        QPalette::All,      true},
        {PaletteHighlightDisabled,         QPalette::Highlight,        QPalette::Disabled, true},
        {PaletteHighlightedText,           QPalette::HighlightedText,  QPalette::All,      true},
        {PaletteHighlightedTextDisabled,   QPalette::HighlightedText,  QPalette::Disabled, true},
        {PaletteLink,                      QPalette::Link,             QPalette::All,      false},
        {PaletteLinkDisabled,              QPalette::Link,             QPalette::Disabled, false},
        {PaletteLinkVisited,               QPalette::LinkVisited,      QPalette::All,      false},
        {PaletteLinkVisitedDisabled,       QPalette::LinkVisited,      QPalette::Disabled, false},
        {PaletteLight,                     QPalette::Light,            QPalette::All,      false},
        {PaletteLightDisabled,             QPalette::Light,            QPalette::Disabled, false},
        {PaletteMidlight,                  QPalette::Midlight,         QPalette::All,      false},
        {PaletteMidlightDisabled,          QPalette::Midlight,         QPalette::Disabled, false},
        {PaletteDark,                      QPalette::Dark,             QPalette::All,      false},
        {PaletteDarkDisabled,              QPalette::Dark,             QPalette::Disabled, false},
        {PaletteMid,                       QPalette::Mid,              QPalette::All,      false},
        {PaletteMidDisabled,               QPalette::Mid,              QPalette::Disabled, false},
        {PaletteShadow,                    QPalette::Shadow,           QPalette::All,      false},
        {PaletteShadowDisabled,            QPalette::Shadow,           QPalette::Disabled, false},
        {PalettePlaceholderText,           QPalette::PlaceholderText,  QPalette::All,      false},
        {PalettePlaceholderTextDisabled,   QPalette::PlaceholderText,  QPalette::Disabled, false},
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        {PaletteAccent,                    QPalette::NoRole,           QPalette::All,      false},
        {PaletteAccentDisabled,            QPalette::NoRole,           QPalette::Disabled, false},
#else
        {PaletteAccent,                    QPalette::Accent,           QPalette::All,      false},
        {PaletteAccentDisabled,            QPalette::Accent,           QPalette::Disabled, false},
#endif
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
