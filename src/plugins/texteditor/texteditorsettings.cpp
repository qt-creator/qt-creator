/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "texteditorsettings.h"

#include "displaysettings.h"
#include "generalsettingspage.h"
#include "fontsettingspage.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtGui/QApplication>

using namespace TextEditor;
using namespace TextEditor::Constants;

TextEditorSettings *TextEditorSettings::m_instance = 0;

TextEditorSettings::TextEditorSettings(Internal::TextEditorPlugin *plugin,
                                       QObject *parent)
    : QObject(parent)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();

    // Note: default background colors are coming from FormatDescription::background()

    // Add font preference page
    FormatDescriptions formatDescriptions;
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_TEXT), tr("Text")));

    // Special categories
    const QPalette p = QApplication::palette();
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_SELECTION), tr("Selection"), p.color(QPalette::HighlightedText)));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_LINE_NUMBER), tr("Line Number")));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_SEARCH_RESULT), tr("Search Result")));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_SEARCH_SCOPE), tr("Search Scope")));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_PARENTHESES), tr("Parentheses")));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_CURRENT_LINE), tr("Current Line")));

    // Standard categories
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_NUMBER), tr("Number"), Qt::darkBlue));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_STRING), tr("String"), Qt::darkGreen));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_TYPE), tr("Type"), Qt::darkMagenta));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_KEYWORD), tr("Keyword"), Qt::darkYellow));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_OPERATOR), tr("Operator")));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_PREPROCESSOR), tr("Preprocessor"), Qt::darkBlue));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_LABEL), tr("Label"), Qt::darkRed));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_COMMENT), tr("Comment"), Qt::darkGreen));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_DISABLED_CODE), tr("Disabled Code"), Qt::gray));

    // Diff categories
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_ADDED_LINE), tr("Added Line"), QColor(0, 170, 0)));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_REMOVED_LINE), tr("Removed Line"), Qt::red));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_DIFF_FILE), tr("Diff File"), Qt::darkBlue));
    formatDescriptions.push_back(FormatDescription(QLatin1String(C_DIFF_LOCATION), tr("Diff Location"), Qt::blue));

    m_fontSettingsPage = new FontSettingsPage(formatDescriptions,
                                              QLatin1String("TextEditor"),
                                              tr("Text Editor"),
                                              plugin->core());
    pm->addObject(m_fontSettingsPage);

    // Add the GUI used to configure the tab, storage and display settings
    TextEditor::GeneralSettingsPageParameters generalSettingsPageParameters;
    generalSettingsPageParameters.name = tr("General");
    generalSettingsPageParameters.category = QLatin1String("TextEditor");
    generalSettingsPageParameters.trCategory = tr("Text Editor");
    generalSettingsPageParameters.settingsPrefix = QLatin1String("text");
    m_generalSettingsPage = new GeneralSettingsPage(plugin->core(), generalSettingsPageParameters, this);
    pm->addObject(m_generalSettingsPage);

    connect(m_fontSettingsPage, SIGNAL(changed(TextEditor::FontSettings)),
            this, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)));
    connect(m_generalSettingsPage, SIGNAL(tabSettingsChanged(TextEditor::TabSettings)),
            this, SIGNAL(tabSettingsChanged(TextEditor::TabSettings)));
    connect(m_generalSettingsPage, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            this, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)));
    connect(m_generalSettingsPage, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            this, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)));
}

TextEditorSettings::~TextEditorSettings()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    pm->removeObject(m_generalSettingsPage);
    pm->removeObject(m_fontSettingsPage);
    delete m_fontSettingsPage;

    m_instance = 0;
}

TextEditorSettings *TextEditorSettings::instance()
{
    return m_instance;
}

FontSettings TextEditorSettings::fontSettings() const
{
    return m_fontSettingsPage->fontSettings();
}

TabSettings TextEditorSettings::tabSettings() const
{
    return m_generalSettingsPage->tabSettings();
}

StorageSettings TextEditorSettings::storageSettings() const
{
    return m_generalSettingsPage->storageSettings();
}

DisplaySettings TextEditorSettings::displaySettings() const
{
    return m_generalSettingsPage->displaySettings();
}
