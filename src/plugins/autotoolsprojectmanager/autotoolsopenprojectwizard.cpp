/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "autotoolsopenprojectwizard.h"

#include <utils/pathchooser.h>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDir>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;

//////////////////////////////////////
// AutotoolsOpenProjectWizard class
//////////////////////////////////////
AutotoolsOpenProjectWizard::AutotoolsOpenProjectWizard(AutotoolsManager *manager,
                                                       const QString &sourceDirectory,
                                                       QWidget *parent) :
    Utils::Wizard(parent),
    m_manager(manager),
    m_sourceDirectory(sourceDirectory)
{
    QDir dir(m_sourceDirectory);
    m_buildDirectory = dir.absolutePath();

    setPage(BuildPathPageId, new BuildPathPage(this));

    setStartId(BuildPathPageId);
    setWindowTitle(tr("Autotools Wizard"));
}

AutotoolsManager *AutotoolsOpenProjectWizard::autotoolsManager() const
{
    return m_manager;
}

QString AutotoolsOpenProjectWizard::buildDirectory() const
{
    return m_buildDirectory;
}

QString AutotoolsOpenProjectWizard::sourceDirectory() const
{
    return m_sourceDirectory;
}

void AutotoolsOpenProjectWizard::setBuildDirectory(const QString &directory)
{
    m_buildDirectory = directory;
}

/////////////////////////
// BuildPathPage class
/////////////////////////
BuildPathPage::BuildPathPage(AutotoolsOpenProjectWizard *w) : QWizardPage(w),
    m_pc(new Utils::PathChooser)
{
    QFormLayout *fl = new QFormLayout;
    this->setLayout(fl);

    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("Please enter the directory in which you want to build your project. "
                      "Qt Creator recommends to not use the source directory for building. "
                      "This ensures that the source directory remains clean and enables multiple builds "
                      "with different settings."));
    fl->addWidget(label);
    m_pc->setHistoryCompleter(QLatin1String("AutoTools.BuildDir.History"));
    AutotoolsOpenProjectWizard *wiz = static_cast<AutotoolsOpenProjectWizard *>(wizard());
    m_pc->setBaseDirectory(wiz->sourceDirectory());
    m_pc->setPath(wiz->buildDirectory());
    connect(m_pc, &Utils::PathChooser::rawPathChanged, this, &BuildPathPage::buildDirectoryChanged);
    fl->addRow(tr("Build directory:"), m_pc);
    setTitle(tr("Build Location"));
}

void BuildPathPage::buildDirectoryChanged()
{
    static_cast<AutotoolsOpenProjectWizard *>(wizard())->setBuildDirectory(m_pc->path());
}
