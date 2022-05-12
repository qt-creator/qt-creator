/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "clangformatindenter.h"
#include "clangformatconstants.h"
#include "clangformatsettings.h"
#include "clangformatutils.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocumentlayout.h>
#include <utils/genericconstants.h>

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

Utils::optional<TabSettings> ClangFormatIndenter::tabSettings() const
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
    return !isBeautifierOnSaveActivated() && formatCodeInsteadOfIndent();
}

} // namespace ClangFormat
