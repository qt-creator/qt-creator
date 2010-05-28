/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "highlightersettingspage.h"
#include "highlightersettings.h"
#include "manager.h"
#include "ui_highlightersettingspage.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>

using namespace TextEditor;
using namespace Internal;

struct HighlighterSettingsPage::HighlighterSettingsPagePrivate
{
    explicit HighlighterSettingsPagePrivate(const QString &id);

    const QString m_id;
    const QString m_displayName;
    const QString m_settingsPrefix;

    QString m_searchKeywords;

    HighlighterSettings m_settings;

    Ui::HighlighterSettingsPage m_page;
};

HighlighterSettingsPage::HighlighterSettingsPagePrivate::
HighlighterSettingsPagePrivate(const QString &id) :
    m_id(id),
    m_displayName(tr("Generic Highlighter")),
    m_settingsPrefix(QLatin1String("text"))
{}

HighlighterSettingsPage::HighlighterSettingsPage(const QString &id, QObject *parent) :
    TextEditorOptionsPage(parent),
    m_d(new HighlighterSettingsPagePrivate(id))
{
    if (QSettings *s = Core::ICore::instance()->settings())
        m_d->m_settings.fromSettings(m_d->m_settingsPrefix, s);

    connect(this, SIGNAL(definitionsLocationChanged()),
            Manager::instance(), SLOT(registerMimeTypes()));
}

HighlighterSettingsPage::~HighlighterSettingsPage()
{
    delete m_d;
}

QString HighlighterSettingsPage::id() const
{
    return m_d->m_id;
}

QString HighlighterSettingsPage::displayName() const
{
    return m_d->m_displayName;
}

QWidget *HighlighterSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_d->m_page.setupUi(w);
    m_d->m_page.definitionFilesPath->setExpectedKind(Utils::PathChooser::Directory);

    settingsToUI();

    if (m_d->m_searchKeywords.isEmpty()) {
        QTextStream(&m_d->m_searchKeywords) << m_d->m_page.definitionFilesGroupBox->title()
            << m_d->m_page.locationLabel->text()
            << m_d->m_page.alertWhenNoDefinition->text();
    }

    connect(m_d->m_page.resetButton, SIGNAL(clicked()), this, SLOT(resetDefinitionsLocation()));
    connect(m_d->m_page.downloadNoteLabel, SIGNAL(linkActivated(QString)),
            Manager::instance(), SLOT(openDefinitionsUrl(QString)));

    return w;
}

void HighlighterSettingsPage::apply()
{
    if (settingsChanged())
        settingsFromUI();
}

bool HighlighterSettingsPage::matches(const QString &s) const
{
    return m_d->m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

const HighlighterSettings &HighlighterSettingsPage::highlighterSettings() const
{
    return m_d->m_settings;
}

void HighlighterSettingsPage::settingsFromUI()
{    
    bool locationChanged = false;
    if (m_d->m_settings.m_definitionFilesPath != m_d->m_page.definitionFilesPath->path())
        locationChanged = true;

    m_d->m_settings.m_definitionFilesPath = m_d->m_page.definitionFilesPath->path();
    m_d->m_settings.m_alertWhenNoDefinition = m_d->m_page.alertWhenNoDefinition->isChecked();
    if (QSettings *s = Core::ICore::instance()->settings())
        m_d->m_settings.toSettings(m_d->m_settingsPrefix, s);

    if (locationChanged)
        emit definitionsLocationChanged();
}

void HighlighterSettingsPage::settingsToUI()
{
    m_d->m_page.definitionFilesPath->setPath(m_d->m_settings.m_definitionFilesPath);
    m_d->m_page.alertWhenNoDefinition->setChecked(m_d->m_settings.m_alertWhenNoDefinition);
}

void HighlighterSettingsPage::resetDefinitionsLocation()
{
    m_d->m_page.definitionFilesPath->setPath(findDefinitionsLocation());
}

bool HighlighterSettingsPage::settingsChanged() const
{    
    if (m_d->m_settings.m_definitionFilesPath != m_d->m_page.definitionFilesPath->path())
        return true;
    if (m_d->m_settings.m_alertWhenNoDefinition != m_d->m_page.alertWhenNoDefinition->isChecked())
        return true;
    return false;
}
