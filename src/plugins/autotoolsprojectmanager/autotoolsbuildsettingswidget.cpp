/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "autotoolsbuildsettingswidget.h"
#include "autotoolsproject.h"
#include "autotoolsbuildconfiguration.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchainmanager.h>

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QFormLayout>
#include <QtGui/QComboBox>
#include <QtCore/QPointer>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

AutotoolsBuildSettingsWidget::AutotoolsBuildSettingsWidget(AutotoolsTarget *target) :
    m_target(target),
    m_pathChooser(0),
    m_toolChainChooser(0),
    m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, 0, 0, 0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setEnabled(true);
    m_pathChooser->setExpectedKind(Utils::PathChooser::Directory);
    m_pathChooser->setBaseDirectory(m_target->autotoolsProject()->projectDirectory());
    fl->addRow(tr("Build directory:"), m_pathChooser);
    connect(m_pathChooser, SIGNAL(changed(QString)), this, SLOT(buildDirectoryChanged()));

    // tool chain
    m_toolChainChooser = new QComboBox;
    m_toolChainChooser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    updateToolChainList();

    fl->addRow(tr("Tool chain:"), m_toolChainChooser);
    connect(m_toolChainChooser, SIGNAL(activated(int)), this, SLOT(toolChainSelected(int)));
    connect(m_target->autotoolsProject(), SIGNAL(toolChainChanged(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainChanged(ProjectExplorer::ToolChain*)));
    connect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(updateToolChainList()));
    connect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(updateToolChainList()));
}

QString AutotoolsBuildSettingsWidget::displayName() const
{
    return QLatin1String("Autotools Manager");
}

void AutotoolsBuildSettingsWidget::init(BuildConfiguration *bc)
{
    m_buildConfiguration = static_cast<AutotoolsBuildConfiguration *>(bc);
    m_pathChooser->setPath(m_buildConfiguration->buildDirectory());
}

void AutotoolsBuildSettingsWidget::buildDirectoryChanged()
{
    m_buildConfiguration->setBuildDirectory(m_pathChooser->rawPath());
}

void AutotoolsBuildSettingsWidget::toolChainSelected(int index)
{
    using namespace ProjectExplorer;

    ToolChain *tc = static_cast<ToolChain *>(m_toolChainChooser->itemData(index).value<void *>());
    m_target->autotoolsProject()->setToolChain(tc);
}

void AutotoolsBuildSettingsWidget::toolChainChanged(ProjectExplorer::ToolChain *tc)
{
    for (int i = 0; i < m_toolChainChooser->count(); ++i) {
        ToolChain *currentTc = static_cast<ToolChain *>(m_toolChainChooser->itemData(i).value<void *>());
        if (currentTc != tc)
            continue;
        m_toolChainChooser->setCurrentIndex(i);
        return;
    }
}

void AutotoolsBuildSettingsWidget::updateToolChainList()
{
    m_toolChainChooser->clear();

    QList<ToolChain *> tcs = ToolChainManager::instance()->toolChains();
    if (!m_target->autotoolsProject()->toolChain()) {
        m_toolChainChooser->addItem(tr("<Invalid tool chain>"), qVariantFromValue(static_cast<void *>(0)));
        m_toolChainChooser->setCurrentIndex(0);
    }
    foreach (ToolChain *tc, tcs) {
        m_toolChainChooser->addItem(tc->displayName(), qVariantFromValue(static_cast<void *>(tc)));
        if (m_target->autotoolsProject()->toolChain()
                && m_target->autotoolsProject()->toolChain()->id() == tc->id())
            m_toolChainChooser->setCurrentIndex(m_toolChainChooser->count() - 1);
    }
}
