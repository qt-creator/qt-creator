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

using namespace TextEditor::Internal;
using namespace Utils;

namespace TextEditor {

class HighlighterSettingsPage::HighlighterSettingsPagePrivate
{
    Q_DECLARE_TR_FUNCTIONS(TextEditor::Internal::HighlighterSettingsPage)

public:
    HighlighterSettingsPagePrivate() = default;

    void ensureInitialized();
    void migrateGenericHighlighterFiles();

    bool m_initialized = false;
    const QString m_settingsPrefix{"Text"};

    HighlighterSettings m_settings;

    QPointer<QWidget> m_widget;
    Ui::HighlighterSettingsPage *m_page = nullptr;
};


void HighlighterSettingsPage::HighlighterSettingsPagePrivate::migrateGenericHighlighterFiles()
{
    QDir userDefinitionPath(m_settings.definitionFilesPath().toString());
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
    : d(new HighlighterSettingsPagePrivate)
{
    setId(Constants::TEXT_EDITOR_HIGHLIGHTER_SETTINGS);
    setDisplayName(HighlighterSettingsPagePrivate::tr("Generic Highlighter"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("TextEditor", "Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
}

HighlighterSettingsPage::~HighlighterSettingsPage()
{
    delete d;
}

QWidget *HighlighterSettingsPage::widget()
{
    if (!d->m_widget) {
        d->m_widget = new QWidget;
        d->m_page = new Ui::HighlighterSettingsPage;
        d->m_page->setupUi(d->m_widget);
        d->m_page->definitionFilesPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
        d->m_page->definitionFilesPath->setHistoryCompleter(QLatin1String("TextEditor.Highlighter.History"));
        connect(d->m_page->downloadDefinitions,
                &QPushButton::pressed,
                [label = QPointer<QLabel>(d->m_page->updateStatus)]() {
                    Highlighter::downloadDefinitions([label](){
                        if (label)
                            label->setText(HighlighterSettingsPagePrivate::tr("Download finished"));
                    });
                });
        connect(d->m_page->reloadDefinitions, &QPushButton::pressed, []() {
            Highlighter::reload();
        });
        connect(d->m_page->resetCache, &QPushButton::clicked, []() {
            Highlighter::clearDefinitionForDocumentCache();
        });

        settingsToUI();
    }
    return d->m_widget;
}

void HighlighterSettingsPage::apply()
{
    if (!d->m_page) // page was not shown
        return;
    if (settingsChanged())
        settingsFromUI();
}

void HighlighterSettingsPage::finish()
{
    delete d->m_widget;
    if (!d->m_page) // page was not shown
        return;
    delete d->m_page;
    d->m_page = nullptr;
}

const HighlighterSettings &HighlighterSettingsPage::highlighterSettings() const
{
    d->ensureInitialized();
    return d->m_settings;
}

void HighlighterSettingsPage::settingsFromUI()
{
    d->ensureInitialized();
    d->m_settings.setDefinitionFilesPath(d->m_page->definitionFilesPath->filePath());
    d->m_settings.setIgnoredFilesPatterns(d->m_page->ignoreEdit->text());
    d->m_settings.toSettings(d->m_settingsPrefix, Core::ICore::settings());
}

void HighlighterSettingsPage::settingsToUI()
{
    d->ensureInitialized();
    d->m_page->definitionFilesPath->setFilePath(d->m_settings.definitionFilesPath());
    d->m_page->ignoreEdit->setText(d->m_settings.ignoredFilesPatterns());
}

bool HighlighterSettingsPage::settingsChanged() const
{
    d->ensureInitialized();
    return d->m_settings.definitionFilesPath() != d->m_page->definitionFilesPath->filePath()
        || d->m_settings.ignoredFilesPatterns() != d->m_page->ignoreEdit->text();
}

} // TextEditor
