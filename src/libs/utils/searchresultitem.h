// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QStringList>
#include <QVariant>

#include <optional>

namespace Utils {

namespace Search {

class QTCREATOR_UTILS_EXPORT TextPosition
{
public:
    int line = -1; // (0 or -1 for no line number)
    int column = -1; // 0-based starting position for a mark (-1 for no mark)

    bool operator<(const TextPosition &other) const
    { return line < other.line || (line == other.line && column < other.column); }
    bool operator==(const TextPosition &other) const;
    bool operator!=(const TextPosition &other) const { return !(operator==(other)); }
};

class QTCREATOR_UTILS_EXPORT TextRange
{
public:
    QString mid(const QString &text) const { return text.mid(begin.column, length(text)); }
    int length(const QString &text) const;

    TextPosition begin;
    TextPosition end;

    bool operator<(const TextRange &other) const { return begin < other.begin; }
    bool operator==(const TextRange &other) const;
    bool operator!=(const TextRange &other) const { return !(operator==(other)); }
};

} // namespace Search

class QTCREATOR_UTILS_EXPORT SearchResultColor
{
public:
    enum class Style { Default, Alt1, Alt2 };

    SearchResultColor() = default;
    SearchResultColor(const QColor &textBg, const QColor &textFg,
                      const QColor &highlightBg, const QColor &highlightFg,
                      const QColor &functionBg, const QColor &functionFg);

    QColor textBackground;
    QColor textForeground;
    QColor highlightBackground;
    QColor highlightForeground;
    QColor containingFunctionBackground;
    QColor containingFunctionForeground;

private:
    QTCREATOR_UTILS_EXPORT friend size_t qHash(Style style, uint seed);
};

using SearchResultColors = QHash<SearchResultColor::Style, SearchResultColor>;

class QTCREATOR_UTILS_EXPORT SearchResultItem
{
public:
    QStringList path() const { return m_path; }
    void setPath(const QStringList &path) { m_path = path; }
    void setFilePath(const Utils::FilePath &filePath) { m_path = {filePath.toUserOutput()}; }

    QString lineText() const { return m_lineText; }
    void setLineText(const QString &text) { m_lineText = text; }
    void setDisplayText(const QString &text);

    QIcon icon() const { return m_icon; }
    void setIcon(const QIcon &icon) { m_icon = icon; }

    QVariant userData() const { return m_userData; }
    void setUserData(const QVariant &userData) { m_userData = userData; }

    Search::TextRange mainRange() const { return m_mainRange; }
    void setMainRange(const Search::TextRange &mainRange) { m_mainRange = mainRange; }
    void setMainRange(int line, int column, int length);

    bool useTextEditorFont() const { return m_useTextEditorFont; }
    void setUseTextEditorFont(bool useTextEditorFont) { m_useTextEditorFont = useTextEditorFont; }

    SearchResultColor::Style style() const { return m_style; }
    void setStyle(SearchResultColor::Style style) { m_style = style; }

    bool selectForReplacement() const { return m_selectForReplacement; }
    void setSelectForReplacement(bool select) { m_selectForReplacement = select; }

    std::optional<QString> containingFunctionName() const { return m_containingFunctionName; }

    void setContainingFunctionName(const std::optional<QString> &containingFunctionName)
    {
        m_containingFunctionName = containingFunctionName;
    }

    bool operator==(const SearchResultItem &other) const;
    bool operator!=(const SearchResultItem &other) const { return !(operator==(other)); }

private:
    QStringList m_path; // hierarchy to the parent item of this item
    QString m_lineText; // text to show for the item itself
    QIcon m_icon; // icon to show in front of the item (by be null icon to hide)
    QVariant m_userData; // user data for identification of the item
    Search::TextRange m_mainRange;
    bool m_useTextEditorFont = false;
    bool m_selectForReplacement = true;
    SearchResultColor::Style m_style = SearchResultColor::Style::Default;
    std::optional<QString> m_containingFunctionName;
};

using SearchResultItems = QList<SearchResultItem>;

} // namespace Utils

Q_DECLARE_METATYPE(Utils::SearchResultItem)
Q_DECLARE_METATYPE(Utils::SearchResultItems)
Q_DECLARE_METATYPE(Utils::Search::TextPosition)
