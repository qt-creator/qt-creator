/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "makestep.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"
#include "cmakebuildconfiguration.h"

#include <projectexplorer/projectexplorer.h>

#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;

MakeStep::MakeStep(BuildConfiguration *bc)
    : AbstractMakeStep(bc), m_clean(false), m_futureInterface(0)
{
    m_percentProgress = QRegExp("^\\[\\s*(\\d*)%\\]");
}

MakeStep::MakeStep(MakeStep *bs, BuildConfiguration *bc)
    : AbstractMakeStep(bs, bc),
    m_clean(bs->m_clean),
    m_futureInterface(0),
    m_buildTargets(bs->m_buildTargets),
    m_additionalArguments(bs->m_buildTargets)
{

}

MakeStep::~MakeStep()
{

}

void MakeStep::setClean(bool clean)
{
    m_clean = clean;
}

void MakeStep::restoreFromGlobalMap(const QMap<QString, QVariant> &map)
{
    if (map.value("clean").isValid() && map.value("clean").toBool())
        m_clean = true;
    AbstractMakeStep::restoreFromGlobalMap(map);
}

void MakeStep::restoreFromLocalMap(const QMap<QString, QVariant> &map)
{
    m_buildTargets = map["buildTargets"].toStringList();
    m_additionalArguments = map["additionalArguments"].toStringList();
    if (map.value("clean").isValid() && map.value("clean").toBool())
        m_clean = true;
    AbstractMakeStep::restoreFromLocalMap(map);
}

void MakeStep::storeIntoLocalMap(QMap<QString, QVariant> &map)
{
    map["buildTargets"] = m_buildTargets;
    map["additionalArguments"] = m_additionalArguments;
    if (m_clean)
        map["clean"] = true;
    AbstractMakeStep::storeIntoLocalMap(map);
}

bool MakeStep::init()
{
    CMakeBuildConfiguration *bc = static_cast<CMakeBuildConfiguration *>(buildConfiguration());
    // TODO, we should probably have a member cmakeBuildConfiguration();

    setBuildParser(bc->buildParser());

    setEnabled(true);
    setWorkingDirectory(bc->buildDirectory());

    setCommand(bc->toolChain()->makeCommand());

    QStringList arguments = m_buildTargets;
    arguments << additionalArguments();
    setArguments(arguments);
    setEnvironment(bc->environment());
    setIgnoreReturnValue(m_clean);

    return AbstractMakeStep::init();
}

void MakeStep::run(QFutureInterface<bool> &fi)
{
    m_futureInterface = &fi;
    m_futureInterface->setProgressRange(0, 100);
    AbstractMakeStep::run(fi);
    m_futureInterface->setProgressValue(100);
    m_futureInterface->reportFinished();
    m_futureInterface = 0;
}

QString MakeStep::name()
{
    return Constants::MAKESTEP;
}

QString MakeStep::displayName()
{
    return "Make";
}

BuildStepConfigWidget *MakeStep::createConfigWidget()
{
    return new MakeStepConfigWidget(this);
}

bool MakeStep::immutable() const
{
    return true;
}

void MakeStep::stdOut(const QString &line)
{
    if (m_percentProgress.indexIn(line) != -1) {
        bool ok = false;
        int percent = m_percentProgress.cap(1).toInt(&ok);;
        if (ok)
            m_futureInterface->setProgressValue(percent);
    }
    AbstractMakeStep::stdOut(line);
}

bool MakeStep::buildsTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void MakeStep::setBuildTarget(const QString &target, bool on)
{
    QStringList old = m_buildTargets;
    if (on && !old.contains(target))
        old << target;
    else if(!on && old.contains(target))
        old.removeOne(target);
    m_buildTargets = old;
}

QStringList MakeStep::additionalArguments() const
{
    return m_additionalArguments;
}

void MakeStep::setAdditionalArguments(const QStringList &list)
{
    m_additionalArguments = list;
}

//
// MakeStepConfigWidget
//

MakeStepConfigWidget::MakeStepConfigWidget(MakeStep *makeStep)
    : m_makeStep(makeStep)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    m_additionalArguments = new QLineEdit(this);
    fl->addRow(tr("Additional arguments:"), m_additionalArguments);

    connect(m_additionalArguments, SIGNAL(textEdited(const QString &)), this, SLOT(additionalArgumentsEdited()));

    m_targetsList = new QListWidget;
    m_targetsList->setMinimumHeight(200);
    fl->addRow(tr("Targets:"), m_targetsList);

    // TODO update this list also on rescans of the CMakeLists.txt
    // TODO shouldn't be accessing project
    CMakeProject *pro = static_cast<CMakeProject *>(m_makeStep->buildConfiguration()->project());
    foreach(const QString& target, pro->targets()) {
        QListWidgetItem *item = new QListWidgetItem(target, m_targetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }

    connect(m_targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
}

void MakeStepConfigWidget::additionalArgumentsEdited()
{
    m_makeStep->setAdditionalArguments(Environment::parseCombinedArgString(m_additionalArguments->text()));
    updateDetails();
}

void MakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

QString MakeStepConfigWidget::displayName() const
{
    return "Make";
}

void MakeStepConfigWidget::init()
{
    // disconnect to make the changes to the items
    disconnect(m_targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
    int count = m_targetsList->count();
    for(int i = 0; i < count; ++i) {
        QListWidgetItem *item = m_targetsList->item(i);
        item->setCheckState(m_makeStep->buildsTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }
    // and connect again
    connect(m_targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));

    m_additionalArguments->setText(Environment::joinArgumentList(m_makeStep->additionalArguments()));
    updateDetails();
}

void MakeStepConfigWidget::updateDetails()
{
    QStringList arguments = m_makeStep->m_buildTargets;
    arguments << m_makeStep->additionalArguments();

    CMakeBuildConfiguration *bc = static_cast<CMakeBuildConfiguration *>(m_makeStep->buildConfiguration());
    m_summaryText = tr("<b>Make:</b> %1 %2").arg(bc->toolChain()->makeCommand(), arguments.join(" "));
    emit updateSummary();
}

QString MakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

//
// MakeStepFactory
//

bool MakeStepFactory::canCreate(const QString &name) const
{
    return (Constants::MAKESTEP == name);
}

BuildStep *MakeStepFactory::create(BuildConfiguration *bc, const QString &name) const
{
    Q_ASSERT(name == Constants::MAKESTEP);
    return new MakeStep(bc);
}

BuildStep *MakeStepFactory::clone(BuildStep *bs, BuildConfiguration *bc) const
{
    return new MakeStep(static_cast<MakeStep *>(bs), bc);
}

QStringList MakeStepFactory::canCreateForProject(Project * /* pro */) const
{
    return QStringList();
}

QString MakeStepFactory::displayNameForName(const QString & /* name */) const
{
    return "Make";
}

