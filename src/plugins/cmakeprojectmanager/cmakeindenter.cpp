// Copyright (C) 2016 Jan Dalheimer <jan@dalheimer.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeindenter.h"

#include "cmakecodestyle.h"
#include "cmakecommandkeywords.h"

#include <cmakelang/cmakeformatter.h>
#include <cmakelang/cmakeindentation.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/tabsettings.h>

#include <utils/changeset.h>

#include <QTextDocument>

#include <optional>

using namespace TextEditor;

namespace CMakeProjectManager::Internal {

class CMakeIndenter final : public TextEditor::TextIndenter
{
public:
    explicit CMakeIndenter(QTextDocument *doc)
        : TextEditor::TextIndenter(doc)
    {
        setElectricCharacters("()");
    }

    int indentFor(const QTextBlock &block,
                  const TabSettingsData &tabSettings,
                  int cursorPositionInEditor = -1) final;
    IndentationForBlock indentationForBlocks(const QList<QTextBlock> &blocks,
                                             const TabSettingsData &tabSettings,
                                             int cursorPositionInEditor = -1) final;
    void indent(const QTextCursor &cursor,
                const QChar &typedChar,
                const TabSettingsData &tabSettings,
                int cursorPositionInEditor = -1) final;
    void reindent(const QTextCursor &cursor,
                  const TabSettingsData &tabSettings,
                  int cursorPositionInEditor = -1) final;
    Utils::EditOperations format(const RangesInLines &rangesInLines,
                                 FormattingMode mode = FormattingMode::Forced) final;
    void setCodeStylePreferences(ICodeStylePreferences *preferences) final;
    void setOverriddenPreferences(ICodeStylePreferences *preferences) final;
    void invalidateCache() final;

private:
    CMakeCodeStyleSettings codeStyleSettings() const;
    CMakeLang::Style styleFor(const CMakeCodeStyleSettings &settings,
                              const TabSettingsData &tabSettings);
    int levelFor(const QTextBlock &block);
    void indentSelection(const QTextCursor &cursor,
                         const TabSettingsData &tabSettings,
                         int cursorPositionInEditor);
    bool touches(const CMakeLang::Edit &edit, const RangesInLines &rangesInLines) const;

    CommandKeywords m_keywords;
    ICodeStylePreferences *m_codeStyle = nullptr;
    std::optional<CMakeLang::Indentation> m_indentation;
    CMakeCodeStyleSettings m_settings;
    int m_revision = -1;
};

void CMakeIndenter::setCodeStylePreferences(ICodeStylePreferences *preferences)
{
    m_codeStyle = preferences;
    invalidateCache();
}

// What the preview of the preferences hands its page-local style over with.
void CMakeIndenter::setOverriddenPreferences(ICodeStylePreferences *preferences)
{
    setCodeStylePreferences(preferences);
}

CMakeCodeStyleSettings CMakeIndenter::codeStyleSettings() const
{
    if (!m_codeStyle)
        return {};
    return m_codeStyle->currentValue().value<CMakeCodeStyleSettings>();
}

CMakeLang::Style CMakeIndenter::styleFor(const CMakeCodeStyleSettings &settings,
                                         const TabSettingsData &tabSettings)
{
    m_keywords.refresh();

    CMakeLang::Style style;
    style.indentKeywordValues = settings.indentKeywordValues;
    style.spaceBeforeControlParen = settings.spaceBeforeControlParen;
    style.spaceBeforeCommandParen = settings.spaceBeforeCommandParen;
    style.keepCommentColumn = settings.keepCommentColumn;
    style.isKeyword = [this](const QString &command, const QString &argument) {
        return m_keywords.contains(command, argument);
    };
    style.indentation = [tabSettings](int level) {
        return tabSettings.indentationString(0, level * tabSettings.m_indentSize, 0);
    };
    return style;
}

int CMakeIndenter::levelFor(const QTextBlock &block)
{
    return m_indentation ? m_indentation->levelAt(block.blockNumber() + 1) : 0;
}

int CMakeIndenter::indentFor(const QTextBlock &block,
                             const TabSettingsData &tabSettings,
                             int /*cursorPositionInEditor*/)
{
    // Changing a setting leaves the document as it was, so the style takes
    // part in telling whether what is cached still holds.
    const CMakeCodeStyleSettings settings = codeStyleSettings();
    if (!m_indentation || m_revision != m_doc->revision() || settings != m_settings) {
        const QString source = m_doc->toPlainText();
        m_indentation.emplace(source, styleFor(settings, tabSettings));
        m_revision = m_doc->revision();
        m_settings = settings;
    }

    const int level = levelFor(block);
    if (level == CMakeLang::Indentation::Keep)
        return -1;
    return level * tabSettings.m_indentSize;
}

IndentationForBlock CMakeIndenter::indentationForBlocks(const QList<QTextBlock> &blocks,
                                                        const TabSettingsData &tabSettings,
                                                        int /*cursorPositionInEditor*/)
{
    IndentationForBlock result;
    for (const QTextBlock &block : blocks) {
        // A line whose whitespace belongs to a value keeps the indentation it
        // has: the callers of this have no way of being told to leave a line
        // alone.
        const int indentation = indentFor(block, tabSettings);
        result.insert(block.blockNumber(),
                      indentation < 0 ? tabSettings.indentationColumn(block.text())
                                      : indentation);
    }
    return result;
}

// The indentation of a line follows from the tokens above it, not from the way
// those lines are laid out, so the whole selection is measured against the text
// as it stands before the first line of it moves.
void CMakeIndenter::indentSelection(const QTextCursor &cursor,
                                    const TabSettingsData &tabSettings,
                                    int cursorPositionInEditor)
{
    const QTextBlock end = m_doc->findBlock(cursor.selectionEnd()).next();

    QList<QPair<int, int>> indentations;
    for (QTextBlock block = m_doc->findBlock(cursor.selectionStart());
         block.isValid() && block != end;
         block = block.next()) {
        indentations.append({block.blockNumber(),
                             indentFor(block, tabSettings, cursorPositionInEditor)});
    }

    for (const auto &[number, indentation] : std::as_const(indentations)) {
        if (indentation >= 0)
            tabSettings.indentLine(m_doc->findBlockByNumber(number), indentation);
    }
}

void CMakeIndenter::indent(const QTextCursor &cursor,
                           const QChar &typedChar,
                           const TabSettingsData &tabSettings,
                           int cursorPositionInEditor)
{
    if (cursor.hasSelection())
        indentSelection(cursor, tabSettings, cursorPositionInEditor);
    else
        indentBlock(cursor.block(), typedChar, tabSettings, cursorPositionInEditor);
}

void CMakeIndenter::reindent(const QTextCursor &cursor,
                             const TabSettingsData &tabSettings,
                             int cursorPositionInEditor)
{
    indent(cursor, QChar::Null, tabSettings, cursorPositionInEditor);
}

bool CMakeIndenter::touches(const CMakeLang::Edit &edit, const RangesInLines &rangesInLines) const
{
    const int lastLine = m_doc->blockCount();
    const int first = qBound(1, m_doc->findBlock(edit.position).blockNumber() + 1, lastLine);
    const int last = qBound(first,
                            m_doc->findBlock(edit.position + edit.length).blockNumber() + 1,
                            lastLine);

    for (const RangeInLines &range : rangesInLines) {
        if (range.startLine <= last && first <= range.endLine)
            return true;
    }
    return false;
}

Utils::EditOperations CMakeIndenter::format(const RangesInLines &rangesInLines, FormattingMode)
{
    if (rangesInLines.empty())
        return {};

    // The style of the document says how it is indented; the settings of the
    // editor only have to answer for a document that has none.
    const TabSettingsData tabSettings = m_codeStyle
                                            ? m_codeStyle->currentTabSettings()
                                            : TabSettingsData::settingsForFile(m_fileName);
    const QString source = m_doc->toPlainText();

    Utils::ChangeSet changeSet;
    const CMakeLang::Style style = styleFor(codeStyleSettings(), tabSettings);
    for (const CMakeLang::Edit &edit : CMakeLang::formattingEdits(source, style)) {
        if (touches(edit, rangesInLines))
            changeSet.replace(edit.position, edit.position + edit.length, edit.text);
    }
    if (changeSet.isEmpty())
        return {};

    const Utils::EditOperations operations = changeSet.operationList();

    QTextCursor cursor(m_doc);
    cursor.beginEditBlock();
    changeSet.apply(&cursor);
    cursor.endEditBlock();

    return operations;
}

void CMakeIndenter::invalidateCache()
{
    m_indentation.reset();
    m_revision = -1;
}

TextEditor::Indenter *createCMakeIndenter(QTextDocument *doc)
{
    return new CMakeIndenter(doc);
}

} // CMakeProjectManager::Internal
