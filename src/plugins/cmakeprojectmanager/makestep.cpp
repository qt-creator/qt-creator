/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "makestep.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"

#include <utils/qtcassert.h>
#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

MakeStep::MakeStep(CMakeProject *pro)
    : AbstractProcessStep(pro), m_pro(pro)
{
}

MakeStep::~MakeStep()
{
}

bool MakeStep::init(const QString &buildConfiguration)
{
    setEnabled(buildConfiguration, true);
    setWorkingDirectory(buildConfiguration, m_pro->buildDirectory(buildConfiguration));
    setCommand(buildConfiguration, "make"); // TODO give full path here?
    setArguments(buildConfiguration, value(buildConfiguration, "buildTargets").toStringList()); // TODO
    setEnvironment(buildConfiguration, m_pro->environment(buildConfiguration));
    return AbstractProcessStep::init(buildConfiguration);
}

void MakeStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

QString MakeStep::name()
{
    return Constants::MAKESTEP;
}

QString MakeStep::displayName()
{
    return "Make";
}

ProjectExplorer::BuildStepConfigWidget *MakeStep::createConfigWidget()
{
    return new MakeBuildStepConfigWidget(this);
}

bool MakeStep::immutable() const
{
    return true;
}

CMakeProject *MakeStep::project() const
{
    return m_pro;
}

bool MakeStep::buildsTarget(const QString &buildConfiguration, const QString &target) const
{
    return value(buildConfiguration, "buildTargets").toStringList().contains(target);
}

void MakeStep::setBuildTarget(const QString &buildConfiguration, const QString &target, bool on)
{
    QStringList old = value(buildConfiguration, "buildTargets").toStringList();
    if (on && !old.contains(target))
        setValue(buildConfiguration, "buildTargets", old << target);
    else if(!on && old.contains(target))
        setValue(buildConfiguration, "buildTargets", old.removeOne(target));
}

//
// CMakeBuildStepConfigWidget
//
MakeBuildStepConfigWidget::MakeBuildStepConfigWidget(MakeStep *makeStep)
    : m_makeStep(makeStep)
{
    QFormLayout *fl = new QFormLayout(this);
    setLayout(fl);

    m_targetsList = new QListWidget;
    fl->addRow("Targets:", m_targetsList);

    // TODO update this list also on rescans of the CMakeLists.txt
    CMakeProject *pro = m_makeStep->project();
    foreach(const QString& target, pro->targets()) {
        QListWidgetItem *item = new QListWidgetItem(target, m_targetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    connect(m_targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
}

void MakeBuildStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(m_buildConfiguration, item->text(), item->checkState() & Qt::Checked);
}

QString MakeBuildStepConfigWidget::displayName() const
{
    return "Make";
}

void MakeBuildStepConfigWidget::init(const QString &buildConfiguration)
{
    // TODO

    // disconnect to make the changes to the items
    disconnect(m_targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
    m_buildConfiguration = buildConfiguration;
    int count = m_targetsList->count();
    for(int i = 0; i < count; ++i) {
        QListWidgetItem *item = m_targetsList->item(i);
        item->setCheckState(m_makeStep->buildsTarget(buildConfiguration, item->text()) ? Qt::Checked : Qt::Unchecked);
    }
    // and connect again
    connect(m_targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));

}

//
// MakeBuildStepFactory
//

bool MakeBuildStepFactory::canCreate(const QString &name) const
{
    return (Constants::MAKESTEP == name);
}

ProjectExplorer::BuildStep *MakeBuildStepFactory::create(ProjectExplorer::Project *project, const QString &name) const
{
    Q_ASSERT(name == Constants::MAKESTEP);
    CMakeProject *pro = qobject_cast<CMakeProject *>(project);
    Q_ASSERT(pro);
    return new MakeStep(pro);
}

QStringList MakeBuildStepFactory::canCreateForProject(ProjectExplorer::Project * /* pro */) const
{
    return QStringList();
}

QString MakeBuildStepFactory::displayNameForName(const QString & /* name */) const
{
    return "Make";
}

