/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2016 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_THEMEDATA_P_H
#define KSYNTAXHIGHLIGHTING_THEMEDATA_P_H

#include "textstyledata_p.h"
#include "theme.h"

#include <QHash>
#include <QSharedData>

namespace KSyntaxHighlighting
{
/**
 * Data container for a Theme.
 */
class ThemeData : public QSharedData
{
public:
    static ThemeData *get(const Theme &theme);

    /**
     * Default constructor, creating an uninitialized ThemeData instance.
     */
    ThemeData();

    /**
     * Load the Theme data from the file @p filePath.
     * Note, that @p filePath either is a local file, or a qt resource location.
     */
    bool load(const QString &filePath);

    /**
     * Returns the unique name of this Theme.
     */
    QString name() const;

    /**
     * Returns the revision of this Theme.
     * The revision in a .theme file should be increased with every change.
     */
    int revision() const;

    /**
     * Returns @c true if this Theme is read-only.
     * Typically, themes that are shipped by default are read-only.
     */
    bool isReadOnly() const;

    /**
     * Returns the full path and filename to this Theme.
     * Themes from the Qt resource return the Qt resource path.
     * Themes from disk return the local path.
     *
     * If the theme is invalid (isValid()), an empty string is returned.
     */
    QString filePath() const;

    /**
     * Returns the text color to be used for @p style.
     * @c 0 is returned for styles that do not specify a text color,
     * use the default text color in that case.
     */
    QRgb textColor(Theme::TextStyle style) const;

    /**
     * Returns the text color for selected to be used for @p style.
     * @c 0 is returned for styles that do not specify a selected text color,
     * use the textColor() in that case.
     */
    QRgb selectedTextColor(Theme::TextStyle style) const;

    /**
     * Returns the background color to be used for @p style.
     * @c 0 is returned for styles that do not specify a background color,
     * use the default background color in that case.
     */
    QRgb backgroundColor(Theme::TextStyle style) const;

    /**
     * Returns the background color for selected text to be used for @p style.
     * @c 0 is returned for styles that do not specify a selected background
     * color, use the default backgroundColor() in that case.
     */
    QRgb selectedBackgroundColor(Theme::TextStyle style) const;

    /**
     * Returns whether the given style should be shown in bold.
     */
    bool isBold(Theme::TextStyle style) const;

    /**
     * Returns whether the given style should be shown in italic.
     */
    bool isItalic(Theme::TextStyle style) const;

    /**
     * Returns whether the given style should be shown underlined.
     */
    bool isUnderline(Theme::TextStyle style) const;

    /**
     * Returns whether the given style should be shown struck through.
     */
    bool isStrikeThrough(Theme::TextStyle style) const;

public:
    /**
     * Returns the editor color for the requested @p role.
     */
    QRgb editorColor(Theme::EditorColorRole role) const;

    /**
     * Returns the TextStyle override of a specific "itemData" with attributeName
     * in the syntax definition called definitionName.
     *
     * If no override exists, a valid TextStyleData with the respective default
     * TextStyle will be used, so the returned value is always valid.
     */
    TextStyleData textStyleOverride(const QString &definitionName, const QString &attributeName) const;

private:
    int m_revision = 0;
    QString m_name;

    //! Path to the file where the theme came from.
    //! This is either a resource location (":/themes/Default.theme"), or a file
    //! on disk (in a read-only or a writeable location).
    QString m_filePath;

    //! TextStyles
    TextStyleData m_textStyles[Theme::Others + 1];

    //! style overrides for individual itemData entries
    //! definition name -> attribute name -> style
    QHash<QString, QHash<QString, TextStyleData>> m_textStyleOverrides;

    //! Editor area colors
    QRgb m_editorColors[Theme::TemplateReadOnlyPlaceholder + 1];
};

}

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(KSyntaxHighlighting::TextStyleData, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

#endif // KSYNTAXHIGHLIGHTING_THEMEDATA_P_H
