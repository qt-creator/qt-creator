// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatindenter.h"
#include "clangformatconstants.h"
#include "clangformatsettings.h"
#include "clangformatutils.h"

#include <coreplugin/icore.h>

#include <cppeditor/cppcodestylepreferencesfactory.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/genericconstants.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <texteditor/tabsettings.h>
#include <texteditor/textdocumentlayout.h>

using namespace clang;
using namespace format;
using namespace TextEditor;

namespace ClangFormat {

static bool isBeautifierPluginActivated()
{
    const QVector<ExtensionSystem::PluginSpec *> specs = ExtensionSystem::PluginManager::plugins();
    return std::find_if(specs.begin(),
                        specs.end(),
                        [](ExtensionSystem::PluginSpec *spec) {
                            return spec->name() == "Beautifier";
                        })
           != specs.end();
}

static bool isBeautifierOnSaveActivated()
{
    if (!isBeautifierPluginActivated())
        return false;

    QSettings *s = Core::ICore::settings();
    bool activated = false;
    s->beginGroup(Utils::Constants::BEAUTIFIER_SETTINGS_GROUP);
    s->beginGroup(Utils::Constants::BEAUTIFIER_GENERAL_GROUP);
    if (s->value(Utils::Constants::BEAUTIFIER_AUTO_FORMAT_ON_SAVE, false).toBool())
        activated = true;
    s->endGroup();
    s->endGroup();
    return activated;
}

ClangFormatIndenter::ClangFormatIndenter(QTextDocument *doc)
    : ClangFormatBaseIndenter(doc)
{}

bool ClangFormatIndenter::formatCodeInsteadOfIndent() const
{
    return ClangFormatSettings::instance().mode() == ClangFormatSettings::Mode::Formatting;
}

std::optional<TabSettings> ClangFormatIndenter::tabSettings() const
{
    FormatStyle style = styleForFile();
    TabSettings tabSettings;

    switch (style.UseTab) {
    case FormatStyle::UT_Never:
        tabSettings.m_tabPolicy = TabSettings::SpacesOnlyTabPolicy;
        break;
    case FormatStyle::UT_Always:
        tabSettings.m_tabPolicy = TabSettings::TabsOnlyTabPolicy;
        break;
    default:
        tabSettings.m_tabPolicy = TabSettings::MixedTabPolicy;
    }

    tabSettings.m_tabSize = static_cast<int>(style.TabWidth);
    tabSettings.m_indentSize = static_cast<int>(style.IndentWidth);

    if (style.AlignAfterOpenBracket == FormatStyle::BAS_DontAlign)
        tabSettings.m_continuationAlignBehavior = TabSettings::NoContinuationAlign;
    else
        tabSettings.m_continuationAlignBehavior = TabSettings::ContinuationAlignWithIndent;

    return tabSettings;
}

int ClangFormatIndenter::lastSaveRevision() const
{
    auto *layout = qobject_cast<TextEditor::TextDocumentLayout *>(m_doc->documentLayout());
    if (!layout)
        return 0;
    return layout->lastSaveRevision;
}

bool ClangFormatIndenter::formatOnSave() const
{
    return ClangFormatSettings::instance().formatOnSave() && !isBeautifierOnSaveActivated()
           && formatCodeInsteadOfIndent();
}

bool ClangFormatIndenter::formatWhileTyping() const
{
    return ClangFormatSettings::instance().formatWhileTyping() && formatCodeInsteadOfIndent();
}

// ClangFormatIndenterWrapper

ClangFormatForwardingIndenter::ClangFormatForwardingIndenter(QTextDocument *doc)
    : TextEditor::Indenter(doc)
    , m_clangFormatIndenter(std::make_unique<ClangFormatIndenter>(doc))
    , m_cppIndenter(CppEditor::CppCodeStylePreferencesFactory().createIndenter(doc))
{}

ClangFormatForwardingIndenter::~ClangFormatForwardingIndenter() = default;

void ClangFormatForwardingIndenter::setFileName(const Utils::FilePath &fileName)
{
    m_fileName = fileName;
    m_clangFormatIndenter->setFileName(fileName);
    m_cppIndenter->setFileName(fileName);
}

TextEditor::Indenter *ClangFormatForwardingIndenter::currentIndenter() const
{
    const ProjectExplorer::Project *projectForFile
        = ProjectExplorer::SessionManager::projectForFile(m_fileName);

    ClangFormatSettings::Mode mode = projectForFile ? static_cast<ClangFormatSettings::Mode>(
                                         projectForFile->namedSettings(Constants::MODE_ID).toInt())
                                                    : ClangFormatSettings::instance().mode();

    if (mode == ClangFormatSettings::Disable)
        return m_cppIndenter.get();

    return m_clangFormatIndenter.get();
}

bool ClangFormatForwardingIndenter::isElectricCharacter(const QChar &ch) const
{
    return currentIndenter()->isElectricCharacter(ch);
}

void ClangFormatForwardingIndenter::setCodeStylePreferences(
    TextEditor::ICodeStylePreferences *preferences)
{
    currentIndenter()->setCodeStylePreferences(preferences);
}

void ClangFormatForwardingIndenter::invalidateCache()
{
    currentIndenter()->invalidateCache();
}

int ClangFormatForwardingIndenter::indentFor(const QTextBlock &block,
                                          const TextEditor::TabSettings &tabSettings,
                                          int cursorPositionInEditor)
{
    return currentIndenter()->indentFor(block, tabSettings, cursorPositionInEditor);
}

int ClangFormatForwardingIndenter::visualIndentFor(const QTextBlock &block,
                                                const TextEditor::TabSettings &tabSettings)
{
    return currentIndenter()->visualIndentFor(block, tabSettings);
}

void ClangFormatForwardingIndenter::autoIndent(const QTextCursor &cursor,
                                            const TextEditor::TabSettings &tabSettings,
                                            int cursorPositionInEditor)
{
    currentIndenter()->autoIndent(cursor, tabSettings, cursorPositionInEditor);
}

Utils::Text::Replacements ClangFormatForwardingIndenter::format(
    const TextEditor::RangesInLines &rangesInLines)
{
    return currentIndenter()->format(rangesInLines);
}


bool ClangFormatForwardingIndenter::formatOnSave() const
{
    return currentIndenter()->formatOnSave();
}

TextEditor::IndentationForBlock ClangFormatForwardingIndenter::indentationForBlocks(
    const QVector<QTextBlock> &blocks,
    const TextEditor::TabSettings &tabSettings,
    int cursorPositionInEditor)
{
    return currentIndenter()->indentationForBlocks(blocks, tabSettings, cursorPositionInEditor);
}

std::optional<TextEditor::TabSettings> ClangFormatForwardingIndenter::tabSettings() const
{
    return currentIndenter()->tabSettings();
}

void ClangFormatForwardingIndenter::indentBlock(const QTextBlock &block,
                                             const QChar &typedChar,
                                             const TextEditor::TabSettings &tabSettings,
                                             int cursorPositionInEditor)
{
    currentIndenter()->indentBlock(block, typedChar, tabSettings, cursorPositionInEditor);
}

void ClangFormatForwardingIndenter::indent(const QTextCursor &cursor,
                                        const QChar &typedChar,
                                        const TextEditor::TabSettings &tabSettings,
                                        int cursorPositionInEditor)
{
    currentIndenter()->indent(cursor, typedChar, tabSettings, cursorPositionInEditor);
}

void ClangFormatForwardingIndenter::reindent(const QTextCursor &cursor,
                                          const TextEditor::TabSettings &tabSettings,
                                          int cursorPositionInEditor)
{
    currentIndenter()->reindent(cursor, tabSettings, cursorPositionInEditor);
}

std::optional<int> ClangFormatForwardingIndenter::margin() const
{
    return currentIndenter()->margin();
}

} // namespace ClangFormat
