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


#include "gitorioushostwizardpage.h"
#include "gitorioushostwidget.h"
#include "gitorious.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtGui/QVBoxLayout>

static const char *settingsGroupC = "Gitorious";
static const char *selectionKeyC = "/SelectedHost";

namespace Gitorious {
namespace Internal {

// Ensure Gitorious  is populated and create widget in right order.
static GitoriousHostWidget *createHostWidget()
{
    // First time? Populate gitorious from settings.
    // If there is still no host, add "gitorious.org"
    Gitorious &gitorious = Gitorious::instance();
    const QSettings *settings = Core::ICore::instance()->settings();
    const QString group = QLatin1String(settingsGroupC);
    if (!gitorious.hostCount()) {
        gitorious.restoreSettings(group, settings);
        if (!gitorious.hostCount())
            gitorious.addHost(Gitorious::gitoriousOrg());
    }
    // Now create widget
    GitoriousHostWidget *rc = new GitoriousHostWidget;
    // Restore selection
    const int selectedRow = settings->value(group + QLatin1String(selectionKeyC)).toInt();
    if (selectedRow >= 0 && selectedRow < gitorious.hostCount())
        rc->selectRow(selectedRow);
    return rc;
}

GitoriousHostWizardPage::GitoriousHostWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_widget(createHostWidget())
{
    connect(m_widget, SIGNAL(validChanged()), this, SIGNAL(completeChanged()));
    QVBoxLayout *lt = new QVBoxLayout;
    lt->addWidget(m_widget);
    setLayout(lt);
    setSubTitle(tr("Select a host."));
}

GitoriousHostWizardPage::~GitoriousHostWizardPage()
{
    // Write out settings + selected row.
    QSettings *settings = Core::ICore::instance()->settings();
    if (m_widget->isHostListDirty())
        Gitorious::instance().saveSettings(QLatin1String(settingsGroupC), settings);
    if (m_widget->isValid())
        settings->setValue(QLatin1String(settingsGroupC) + QLatin1String(selectionKeyC), m_widget->selectedRow());
}

bool GitoriousHostWizardPage::isComplete() const
{
    return m_widget->isValid();
}

int GitoriousHostWizardPage::selectedHostIndex() const
{
    return m_widget->selectedRow();
}

} // namespace Internal
} // namespace Gitorious
