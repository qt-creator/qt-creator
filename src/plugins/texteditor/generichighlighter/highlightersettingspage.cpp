/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include "manager.h"
#include "managedefinitionsdialog.h"
#include "ui_highlightersettingspage.h"

#include <coreplugin/icore.h>

#include <QMessageBox>
#include <QPointer>

using namespace TextEditor;
using namespace Internal;

struct HighlighterSettingsPage::HighlighterSettingsPagePrivate
{
    explicit HighlighterSettingsPagePrivate(Core::Id id);
    void ensureInitialized();

    bool m_initialized;
    const Core::Id m_id;
    const QString m_displayName;
    const QString m_settingsPrefix;

    HighlighterSettings m_settings;

    QPointer<QWidget> m_widget;
    Ui::HighlighterSettingsPage *m_page;
};

HighlighterSettingsPage::HighlighterSettingsPagePrivate::
HighlighterSettingsPagePrivate(Core::Id id) :
    m_initialized(false),
    m_id(id),
    m_displayName(tr("Generic Highlighter")),
    m_settingsPrefix(QLatin1String("Text")),
    m_page(nullptr)
{}

void HighlighterSettingsPage::HighlighterSettingsPagePrivate::ensureInitialized()
{
    if (m_initialized)
        return;
    m_initialized = true;
    m_settings.fromSettings(m_settingsPrefix, Core::ICore::settings());
}

HighlighterSettingsPage::HighlighterSettingsPage(Core::Id id, QObject *parent) :
    TextEditorOptionsPage(parent),
    m_requestHighlightFileRegistration(false),
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
        m_d->m_page->definitionFilesPath->addButton(tr("Download Definitions..."), this,
                                                    [this] { requestAvailableDefinitionsMetaData(); });
        m_d->m_page->fallbackDefinitionFilesPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
        m_d->m_page->fallbackDefinitionFilesPath->setHistoryCompleter(QLatin1String("TextEditor.Highlighter.History"));
        m_d->m_page->fallbackDefinitionFilesPath->addButton(tr("Autodetect"), this,
                                                    [this] { resetDefinitionsLocation(); });

        settingsToUI();

        connect(m_d->m_page->useFallbackLocation, &QAbstractButton::clicked,
                this, &HighlighterSettingsPage::setFallbackLocationState);
        connect(m_d->m_page->definitionFilesPath, &Utils::PathChooser::validChanged,
                this, &HighlighterSettingsPage::setDownloadDefinitionsState);
        connect(m_d->m_widget.data(), &QObject::destroyed,
                this, &HighlighterSettingsPage::ignoreDownloadReply);
    }
    return m_d->m_widget;
}

void HighlighterSettingsPage::apply()
{
    if (!m_d->m_page) // page was not shown
        return;
    if (settingsChanged())
        settingsFromUI();

    if (m_requestHighlightFileRegistration) {
        Manager::instance()->registerHighlightingFiles();
        m_requestHighlightFileRegistration = false;
    }
}

void HighlighterSettingsPage::finish()
{
    delete m_d->m_widget;
    if (!m_d->m_page) // page was not shown
        return;
    delete m_d->m_page;
    m_d->m_page = 0;
}

const HighlighterSettings &HighlighterSettingsPage::highlighterSettings() const
{
    m_d->ensureInitialized();
    return m_d->m_settings;
}

void HighlighterSettingsPage::settingsFromUI()
{
    m_d->ensureInitialized();
    if (!m_requestHighlightFileRegistration && (
        m_d->m_settings.definitionFilesPath() != m_d->m_page->definitionFilesPath->path() ||
        m_d->m_settings.fallbackDefinitionFilesPath() !=
            m_d->m_page->fallbackDefinitionFilesPath->path() ||
        m_d->m_settings.useFallbackLocation() != m_d->m_page->useFallbackLocation->isChecked())) {
        m_requestHighlightFileRegistration = true;
    }

    m_d->m_settings.setDefinitionFilesPath(m_d->m_page->definitionFilesPath->path());
    m_d->m_settings.setFallbackDefinitionFilesPath(m_d->m_page->fallbackDefinitionFilesPath->path());
    m_d->m_settings.setUseFallbackLocation(m_d->m_page->useFallbackLocation->isChecked());
    m_d->m_settings.setIgnoredFilesPatterns(m_d->m_page->ignoreEdit->text());
    m_d->m_settings.toSettings(m_d->m_settingsPrefix, Core::ICore::settings());
}

void HighlighterSettingsPage::settingsToUI()
{
    m_d->ensureInitialized();
    m_d->m_page->definitionFilesPath->setPath(m_d->m_settings.definitionFilesPath());
    m_d->m_page->fallbackDefinitionFilesPath->setPath(m_d->m_settings.fallbackDefinitionFilesPath());
    m_d->m_page->useFallbackLocation->setChecked(m_d->m_settings.useFallbackLocation());
    m_d->m_page->ignoreEdit->setText(m_d->m_settings.ignoredFilesPatterns());

    setFallbackLocationState(m_d->m_page->useFallbackLocation->isChecked());
    setDownloadDefinitionsState(m_d->m_page->definitionFilesPath->isValid());
}

void HighlighterSettingsPage::resetDefinitionsLocation()
{
    const QString &location = findFallbackDefinitionsLocation();
    if (location.isEmpty())
        QMessageBox::information(0, tr("Autodetect Definitions"),
                                 tr("No pre-installed definitions could be found."));
    else
        m_d->m_page->fallbackDefinitionFilesPath->setPath(location);
}

void HighlighterSettingsPage::requestAvailableDefinitionsMetaData()
{
    setDownloadDefinitionsState(false);

    connect(Manager::instance(), &Manager::definitionsMetaDataReady,
            this, &HighlighterSettingsPage::manageDefinitions, Qt::UniqueConnection);
    connect(Manager::instance(), &Manager::errorDownloadingDefinitionsMetaData,
            this, &HighlighterSettingsPage::showError, Qt::UniqueConnection);
    Manager::instance()->downloadAvailableDefinitionsMetaData();
}

void HighlighterSettingsPage::ignoreDownloadReply()
{
    disconnect(Manager::instance(), &Manager::definitionsMetaDataReady,
               this, &HighlighterSettingsPage::manageDefinitions);
    disconnect(Manager::instance(), &Manager::errorDownloadingDefinitionsMetaData,
               this, &HighlighterSettingsPage::showError);
}

void HighlighterSettingsPage::manageDefinitions(const QList<DefinitionMetaDataPtr> &metaData)
{
    ManageDefinitionsDialog dialog(metaData,
                                   m_d->m_page->definitionFilesPath->path() + QLatin1Char('/'),
                                   m_d->m_page->definitionFilesPath->buttonAtIndex(1)->window());
    if (dialog.exec() && !m_requestHighlightFileRegistration)
        m_requestHighlightFileRegistration = true;
    setDownloadDefinitionsState(m_d->m_page->definitionFilesPath->isValid());
}

void HighlighterSettingsPage::showError()
{
    QMessageBox::critical(m_d->m_page->definitionFilesPath->buttonAtIndex(1)->window(),
                          tr("Error connecting to server."),
                          tr("Not possible to retrieve data."));
    setDownloadDefinitionsState(m_d->m_page->definitionFilesPath->isValid());
}

void HighlighterSettingsPage::setFallbackLocationState(bool checked)
{
    m_d->m_page->fallbackDefinitionFilesPath->setEnabled(checked);
}

void HighlighterSettingsPage::setDownloadDefinitionsState(bool valid)
{
    m_d->m_page->definitionFilesPath->buttonAtIndex(1)->setEnabled(valid);
}

bool HighlighterSettingsPage::settingsChanged() const
{
    m_d->ensureInitialized();
    if (m_d->m_settings.definitionFilesPath() != m_d->m_page->definitionFilesPath->path())
        return true;
    if (m_d->m_settings.fallbackDefinitionFilesPath() !=
            m_d->m_page->fallbackDefinitionFilesPath->path())
        return true;
    if (m_d->m_settings.useFallbackLocation() != m_d->m_page->useFallbackLocation->isChecked())
        return true;
    if (m_d->m_settings.ignoredFilesPatterns() != m_d->m_page->ignoreEdit->text())
        return true;
    return false;
}
