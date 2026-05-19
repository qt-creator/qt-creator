// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"
#include "texteditorsupport_global.h"

#include <utils/aspects.h>
#include <utils/filepath.h>
#include <utils/qtcsettings.h>
#include <utils/store.h>

#include <QTextBlock>

#include <functional>

namespace TextEditor {

class ICodeStylePreferences;

// Tab settings: Data type the GeneralSettingsPage acts on
// with some convenience functions for formatting.
class TEXTEDITORSUPPORT_EXPORT TabSettingsData
{
public:
    enum TabPolicy {
        SpacesOnlyTabPolicy = 0,
        TabsOnlyTabPolicy
    };

    // This enum must match the indexes of continuationAlignBehavior widget
    enum ContinuationAlignBehavior {
        NoContinuationAlign = 0,
        ContinuationAlignWithSpaces = 1,
        ContinuationAlignWithIndent = 2
    };

    TabSettingsData() = default;
    TabSettingsData(TabPolicy tabPolicy, int tabSize,
                int indentSize, ContinuationAlignBehavior continuationAlignBehavior);

    void toMap(Utils::Store &map) const;
    void fromMap(const Utils::Store &map);

    TabSettingsData autoDetect(const QTextDocument *document) const;

    int columnAt(const QString &text, int position) const;
    int columnAtCursorPosition(const QTextCursor &cursor) const;
    int positionAtColumn(const QString &text, int column, int *offset = nullptr, bool allowOverstep = false) const;
    int columnCountForText(const QString &text, int startColumn = 0) const;
    int indentedColumn(int column, bool doIndent = true) const;
    QString indentationString(int startColumn, int targetColumn, int padding) const;
    int indentationColumn(const QString &text) const;
    static int maximumPadding(const QString &text);

    void indentLine(const QTextBlock &block, int newIndent, int padding = 0) const;
    void reindentLine(QTextBlock block, int delta) const;

    bool isIndentationClean(const QTextBlock &block, const int indent) const;

    friend bool operator==(const TabSettingsData &t1, const TabSettingsData &t2) { return t1.equals(t2); }
    friend bool operator!=(const TabSettingsData &t1, const TabSettingsData &t2) { return !t1.equals(t2); }

    static int firstNonSpace(const QString &text);
    static QString indentationString(const QString &text);
    static bool onlySpace(const QString &text) { return firstNonSpace(text) == text.size(); }
    static int spacesLeftFromPosition(const QString &text, int position);
    static bool cursorIsAtBeginningOfLine(const QTextCursor &cursor);
    static int trailingWhitespaces(const QString &text);
    static void removeTrailingWhitespace(const QTextBlock &block);

    bool m_autoDetect = true;
    TabPolicy m_tabPolicy = SpacesOnlyTabPolicy;
    int m_tabSize = 8;
    int m_indentSize = 4;
    ContinuationAlignBehavior m_continuationAlignBehavior = ContinuationAlignWithSpaces;

    bool equals(const TabSettingsData &ts) const;

    using Retriever = std::function<TabSettingsData(const Utils::FilePath &)>;
    static void setRetriever(const Retriever &retriever);
    static TabSettingsData settingsForFile(const Utils::FilePath &filePath);
};

class TEXTEDITOR_EXPORT TabSettings : public Utils::AspectContainer
{
    Q_OBJECT

public:
    enum CodingStyleLink {
        CppLink,
        QtQuickLink
    };

    TabSettings();
    ~TabSettings() override;

    void setPreferences(ICodeStylePreferences *preferences);

    TabSettingsData data() const;

    void setCodingStyleWarningVisible(bool visible);
    void setData(const TabSettingsData &s);

signals:
    void settingsChanged(const TextEditor::TabSettingsData &);
    void codingStyleLinkClicked(TextEditor::TabSettings::CodingStyleLink link);

private:
    void codingStyleLinkActivated(const QString &linkString);
    void slotCurrentPreferencesChanged(ICodeStylePreferences *preferences);
    void slotTabSettingsChanged(const TabSettingsData &settings);

    ICodeStylePreferences *m_preferences = nullptr;

    Utils::BoolAspect autoDetect{this};
    Utils::SelectionAspect tabPolicy{this};
    Utils::IntegerAspect tabSize{this};
    Utils::IntegerAspect indentSize{this};
    Utils::SelectionAspect continuationAlignBehavior{this};

    QLabel *m_codingStyleWarning;
};

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::TabSettingsData)
Q_DECLARE_METATYPE(TextEditor::TabSettingsData::TabPolicy)
Q_DECLARE_METATYPE(TextEditor::TabSettingsData::ContinuationAlignBehavior)
