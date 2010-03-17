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

#include "buildconfigdialog.h"
#include "project.h"
#include "runconfiguration.h"
#include "buildconfiguration.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QFormLayout>

namespace ProjectExplorer {
namespace Internal {

BuildConfigDialog::BuildConfigDialog(Project *project, QWidget *parent)
    : QDialog(parent),
    m_project(project)
{
    QVBoxLayout *vlayout = new QVBoxLayout;
    setLayout(vlayout);
    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    m_changeBuildConfiguration = buttonBox->addButton(tr("Change build configuration && continue"),
        QDialogButtonBox::ActionRole);
    m_cancel = buttonBox->addButton(tr("Cancel"),
        QDialogButtonBox::RejectRole);
    m_justContinue = buttonBox->addButton(tr("Continue anyway"),
        QDialogButtonBox::AcceptRole);
    connect(m_changeBuildConfiguration, SIGNAL(clicked()), this, SLOT(buttonClicked()));
    connect(m_cancel, SIGNAL(clicked()), this, SLOT(buttonClicked()));
    connect(m_justContinue, SIGNAL(clicked()), this, SLOT(buttonClicked()));
    setWindowTitle(tr("Run configuration does not match build configuration"));
    QLabel *shortText = new QLabel(tr(
            "The active build configuration builds a target "
            "that cannot be used by the active run configuration."
            ));
    vlayout->addWidget(shortText);
    QLabel *descriptiveText = new QLabel(tr(
        "This can happen if the active build configuration "
        "uses the wrong Qt version and/or tool chain for the active run configuration "
        "(for example, running in Symbian emulator requires building with the WINSCW tool chain)."
    ));
    descriptiveText->setWordWrap(true);
    vlayout->addWidget(descriptiveText);
    m_configCombo = new QComboBox;

    RunConfiguration *activeRun = m_project->activeTarget()->activeRunConfiguration();
    foreach (BuildConfiguration *config, m_project->activeTarget()->buildConfigurations()) {
        if (activeRun->isEnabled(config)) {
            m_configCombo->addItem(config->displayName(), QVariant::fromValue(config));
        }
    }
    if (m_configCombo->count() == 0) {
        m_configCombo->addItem(tr("No valid build configuration found."));
        m_configCombo->setEnabled(false);
        m_changeBuildConfiguration->setEnabled(false);
    }

    QFormLayout *formlayout = new QFormLayout;
    formlayout->addRow(tr("Active run configuration"),
                       // ^ avoiding a new translatable string for active run configuration
                       new QLabel(activeRun->displayName()));
    formlayout->addRow(tr("Choose build configuration:"), m_configCombo);
    vlayout->addLayout(formlayout);
    vlayout->addWidget(buttonBox);
    m_cancel->setDefault(true);
}

BuildConfiguration *BuildConfigDialog::selectedBuildConfiguration() const
{
    int index = m_configCombo->currentIndex();
    if (index < 0)
        return 0;
    return m_configCombo->itemData(index, Qt::UserRole).value<BuildConfiguration*>();
}

void BuildConfigDialog::buttonClicked()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (button == m_changeBuildConfiguration) {
        done(ChangeBuild);
    } else if (button == m_cancel) {
        done(Cancel);
    } else if (button == m_justContinue) {
        done(Continue);
    }
}

} // namespace Internal
} // namespace ProjectExplorer
