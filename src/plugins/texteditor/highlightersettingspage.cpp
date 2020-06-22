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

#include "highlightersettingspage.h"
#include "highlightersettings.h"
#include "highlighter.h"
#include "ui_highlightersettingspage.h"

#include <coreplugin/icore.h>

#include <QDir>
#include <QMessageBox>
#include <QPointer>

using namespace TextEditor;
using namespace Internal;

struct HighlighterSettingsPage::HighlighterSettingsPagePrivate
{
    Q_DECLARE_TR_FUNCTIONS(TextEditor::Internal::HighlighterSettingsPage)

public:
    HighlighterSettingsPagePrivate();
    void ensureInitialized();
    void migrateGenericHighlighterFiles();

    bool m_initialized = false;
    const QString m_settingsPrefix;

    HighlighterSettings m_settings;

    QPointer<QWidget> m_widget;
    Ui::HighlighterSettingsPage *m_page = nullptr;
};

HighlighterSettingsPage::HighlighterSettingsPagePrivate::HighlighterSettingsPagePrivate()
    : m_settingsPrefix("Text")
{}

void HighlighterSettingsPage::HighlighterSettingsPagePrivate::migrateGenericHighlighterFiles()
{
    QDir userDefinitionPath(m_settings.definitionFilesPath());
    if (userDefinitionPath.mkdir("syntax")) {
        const auto link = Utils::HostOsInfo::isAnyUnixHost()
                              ? static_cast<bool(*)(const QString &, const QString &)>(&QFile::link)
                              : static_cast<bool(*)(const QString &, const QString &)>(&QFile::copy);

        for (const QFileInfo &file : userDefinitionPath.entryInfoList({"*.xml"}, QDir::Files))
            link(file.filePath(), file.absolutePath() + "/syntax/" + file.fileName());
    }
}

void HighlighterSettingsPage::HighlighterSettingsPagePrivate::ensureInitialized()
{
    if (m_initialized)
        return;
    m_initialized = true;
    m_settings.fromSettings(m_settingsPrefix, Core::ICore::settings());
    migrateGenericHighlighterFiles();
}

HighlighterSettingsPage::HighlighterSettingsPage()
    : m_d(new HighlighterSettingsPagePrivate)
{
    setId(Constants::TEXT_EDITOR_HIGHLIGHTER_SETTINGS);
    setDisplayName(HighlighterSettingsPagePrivate::tr("Generic Highlighter"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("TextEditor", "Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
}

HighlighterSettingsPage::~HighlighterSettingsPage()
{
    delete m_d;
}

QWidget *HighlighterSettingsPage::widget()
{
    if (!m_d->m_widget) {
        m_d->m_widget = new QWidget;
        m_d->m_page = new Ui::HighlighterSettingsPage;
        m_d->m_page->setupUi(m_d->m_widget);
        m_d->m_page->definitionFilesPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
        m_d->m_page->definitionFilesPath->setHistoryCompleter(QLatin1String("TextEditor.Highlighter.History"));
        connect(m_d->m_page->downloadDefinitions,
                &QPushButton::pressed,
                [label = QPointer<QLabel>(m_d->m_page->updateStatus)]() {
                    Highlighter::downloadDefinitions([label](){
                        if (label)
                            label->setText(HighlighterSettingsPagePrivate::tr("Download finished"));
                    });
                });
        connect(m_d->m_page->reloadDefinitions, &QPushButton::pressed, []() {
            Highlighter::reload();
        });
        connect(m_d->m_page->resetCache, &QPushButton::clicked, []() {
            Highlighter::clearDefinitionForDocumentCache();
        });

        settingsToUI();
    }
    return m_d->m_widget;
}

void HighlighterSettingsPage::apply()
{
    if (!m_d->m_page) // page was not shown
        return;
    if (settingsChanged())
        settingsFromUI();
}

void HighlighterSettingsPage::finish()
{
    delete m_d->m_widget;
    if (!m_d->m_page) // page was not shown
        return;
    delete m_d->m_page;
    m_d->m_page = nullptr;
}

const HighlighterSettings &HighlighterSettingsPage::highlighterSettings() const
{
    m_d->ensureInitialized();
    return m_d->m_settings;
}

void HighlighterSettingsPage::settingsFromUI()
{
    m_d->ensureInitialized();
    m_d->m_settings.setDefinitionFilesPath(m_d->m_page->definitionFilesPath->filePath().toString());
    m_d->m_settings.setIgnoredFilesPatterns(m_d->m_page->ignoreEdit->text());
    m_d->m_settings.toSettings(m_d->m_settingsPrefix, Core::ICore::settings());
}

void HighlighterSettingsPage::settingsToUI()
{
    m_d->ensureInitialized();
    m_d->m_page->definitionFilesPath->setPath(m_d->m_settings.definitionFilesPath());
    m_d->m_page->ignoreEdit->setText(m_d->m_settings.ignoredFilesPatterns());
}

bool HighlighterSettingsPage::settingsChanged() const
{
    m_d->ensureInitialized();
    return (m_d->m_settings.definitionFilesPath() != m_d->m_page->definitionFilesPath->filePath().toString())
            || (m_d->m_settings.ignoredFilesPatterns() != m_d->m_page->ignoreEdit->text());
}
