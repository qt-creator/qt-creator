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
    explicit HighlighterSettingsPagePrivate(Core::Id id);
    void ensureInitialized();
    void migrateGenericHighlighterFiles();

    bool m_initialized = false;
    const Core::Id m_id;
    const QString m_displayName;
    const QString m_settingsPrefix;

    HighlighterSettings m_settings;

    QPointer<QWidget> m_widget;
    Ui::HighlighterSettingsPage *m_page = nullptr;
};

HighlighterSettingsPage::HighlighterSettingsPagePrivate::
HighlighterSettingsPagePrivate(Core::Id id)
    : m_id(id)
    , m_displayName(tr("Generic Highlighter"))
    , m_settingsPrefix("Text")
{}

void HighlighterSettingsPage::HighlighterSettingsPagePrivate::migrateGenericHighlighterFiles()
{
    QDir userDefinitionPath(m_settings.definitionFilesPath());
    if (userDefinitionPath.mkdir("syntax")) {
        const auto link = Utils::HostOsInfo::isAnyUnixHost()
                              ? QOverload<const QString &, const QString &>::of(&QFile::link)
                              : QOverload<const QString &, const QString &>::of(&QFile::copy);

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

HighlighterSettingsPage::HighlighterSettingsPage(Core::Id id, QObject *parent) :
    TextEditorOptionsPage(parent),
    m_d(new HighlighterSettingsPagePrivate(id))
{
    setId(m_d->m_id);
    setDisplayName(m_d->m_displayName);
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
        connect(m_d->m_page->updateDefinitions,
                &QPushButton::pressed,
                [label = QPointer<QLabel>(m_d->m_page->updateStatus)]() {
                    Highlighter::updateDefinitions([label](){
                        if (label)
                            label->setText(tr("Update finished"));
                    });
                });
        connect(m_d->m_page->resetCache, &QPushButton::clicked, []() {
            Highlighter::clearDefintionForDocumentCache();
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
    m_d->m_settings.setDefinitionFilesPath(m_d->m_page->definitionFilesPath->path());
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
    return (m_d->m_settings.definitionFilesPath() != m_d->m_page->definitionFilesPath->path())
            || (m_d->m_settings.ignoredFilesPatterns() != m_d->m_page->ignoreEdit->text());
}
