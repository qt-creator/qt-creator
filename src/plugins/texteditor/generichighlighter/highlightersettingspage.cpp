/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "highlightersettingspage.h"
#include "highlightersettings.h"
#include "manager.h"
#include "managedefinitionsdialog.h"
#include "ui_highlightersettingspage.h"

#include <coreplugin/icore.h>

#include <QMessageBox>

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

    QString m_searchKeywords;

    HighlighterSettings m_settings;

    Ui::HighlighterSettingsPage *m_page;
};

HighlighterSettingsPage::HighlighterSettingsPagePrivate::
HighlighterSettingsPagePrivate(Core::Id id) :
    m_initialized(false),
    m_id(id),
    m_displayName(tr("Generic Highlighter")),
    m_settingsPrefix(QLatin1String("Text")),
    m_page(0)
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
    m_requestMimeTypeRegistration(false),
    m_d(new HighlighterSettingsPagePrivate(id))
{
    setId(m_d->m_id);
    setDisplayName(m_d->m_displayName);
}

HighlighterSettingsPage::~HighlighterSettingsPage()
{
    delete m_d;
}

QWidget *HighlighterSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_d->m_page = new Ui::HighlighterSettingsPage;
    m_d->m_page->setupUi(w);
    m_d->m_page->definitionFilesPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_d->m_page->definitionFilesPath->addButton(tr("Download Definitions..."), this,
                                               SLOT(requestAvailableDefinitionsMetaData()));
    m_d->m_page->fallbackDefinitionFilesPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_d->m_page->fallbackDefinitionFilesPath->addButton(tr("Autodetect"), this,
                                                       SLOT(resetDefinitionsLocation()));

    settingsToUI();

    if (m_d->m_searchKeywords.isEmpty()) {
        QTextStream(&m_d->m_searchKeywords) << m_d->m_page->definitionFilesGroupBox->title()
            << m_d->m_page->locationLabel->text()
            << m_d->m_page->useFallbackLocation->text()
            << m_d->m_page->ignoreLabel->text();
    }

    connect(m_d->m_page->useFallbackLocation, SIGNAL(clicked(bool)),
            this, SLOT(setFallbackLocationState(bool)));
    connect(m_d->m_page->definitionFilesPath, SIGNAL(validChanged(bool)),
            this, SLOT(setDownloadDefinitionsState(bool)));
    connect(w, SIGNAL(destroyed()), this, SLOT(ignoreDownloadReply()));

    return w;
}

void HighlighterSettingsPage::apply()
{
    if (!m_d->m_page) // page was not shown
        return;
    if (settingsChanged())
        settingsFromUI();

    if (m_requestMimeTypeRegistration) {
        Manager::instance()->registerMimeTypes();
        m_requestMimeTypeRegistration = false;
    }
}

void HighlighterSettingsPage::finish()
{
    if (!m_d->m_page) // page was not shown
        return;
    delete m_d->m_page;
    m_d->m_page = 0;
}

bool HighlighterSettingsPage::matches(const QString &s) const
{
    return m_d->m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

const HighlighterSettings &HighlighterSettingsPage::highlighterSettings() const
{
    m_d->ensureInitialized();
    return m_d->m_settings;
}

void HighlighterSettingsPage::settingsFromUI()
{
    m_d->ensureInitialized();
    if (!m_requestMimeTypeRegistration && (
        m_d->m_settings.definitionFilesPath() != m_d->m_page->definitionFilesPath->path() ||
        m_d->m_settings.fallbackDefinitionFilesPath() !=
            m_d->m_page->fallbackDefinitionFilesPath->path() ||
        m_d->m_settings.useFallbackLocation() != m_d->m_page->useFallbackLocation->isChecked())) {
        m_requestMimeTypeRegistration = true;
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

    connect(Manager::instance(),
            SIGNAL(definitionsMetaDataReady(QList<Internal::HighlightDefinitionMetaData>)),
            this,
            SLOT(manageDefinitions(QList<Internal::HighlightDefinitionMetaData>)),
            Qt::UniqueConnection);
    connect(Manager::instance(), SIGNAL(errorDownloadingDefinitionsMetaData()),
            this, SLOT(showError()), Qt::UniqueConnection);
    Manager::instance()->downloadAvailableDefinitionsMetaData();
}

void HighlighterSettingsPage::ignoreDownloadReply()
{
    disconnect(Manager::instance(),
               SIGNAL(definitionsMetaDataReady(QList<Internal::HighlightDefinitionMetaData>)),
               this,
               SLOT(manageDefinitions(QList<Internal::HighlightDefinitionMetaData>)));
    disconnect(Manager::instance(), SIGNAL(errorDownloadingDefinitionsMetaData()),
               this, SLOT(showError()));
}

void HighlighterSettingsPage::manageDefinitions(const QList<HighlightDefinitionMetaData> &metaData)
{
    ManageDefinitionsDialog dialog(metaData,
                                   m_d->m_page->definitionFilesPath->path() + QLatin1Char('/'),
                                   m_d->m_page->definitionFilesPath->buttonAtIndex(1)->window());
    if (dialog.exec() && !m_requestMimeTypeRegistration)
        m_requestMimeTypeRegistration = true;
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
