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

#include <projectexplorer/projectexplorer.h>

#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

MakeStep::MakeStep(CMakeProject *pro)
    : AbstractMakeStep(pro), m_pro(pro), m_clean(false)
{
    m_percentProgress = QRegExp("^\\[\\s*(\\d*)%\\]");
}

MakeStep::~MakeStep()
{

}

void MakeStep::setClean(bool clean)
{
    m_clean = clean;
}

void MakeStep::restoreFromMap(const QMap<QString, QVariant> &map)
{
    if (map.value("clean").isValid() && map.value("clean").toBool())
        m_clean = true;
    ProjectExplorer::AbstractMakeStep::restoreFromMap(map);
}

void MakeStep::storeIntoMap(QMap<QString, QVariant> &map)
{
    if (m_clean)
        map["clean"] = true;
    ProjectExplorer::AbstractMakeStep::storeIntoMap(map);
}

void MakeStep::restoreFromMap(const QString &buildConfiguration, const QMap<QString, QVariant> &map)
{
    m_values[buildConfiguration].buildTargets = map["buildTargets"].toStringList();
    m_values[buildConfiguration].additionalArguments = map["additionalArguments"].toStringList();
    ProjectExplorer::AbstractMakeStep::restoreFromMap(buildConfiguration, map);
}

void MakeStep::storeIntoMap(const QString &buildConfiguration, QMap<QString, QVariant> &map)
{
    map["buildTargets"] = m_values.value(buildConfiguration).buildTargets;
    map["additionalArguments"] = m_values.value(buildConfiguration).additionalArguments;
    ProjectExplorer::AbstractMakeStep::storeIntoMap(buildConfiguration, map);
}


void MakeStep::addBuildConfiguration(const QString & name)
{
    m_values.insert(name, MakeStepSettings());
}

void MakeStep::removeBuildConfiguration(const QString & name)
{
    m_values.remove(name);
}

void MakeStep::copyBuildConfiguration(const QString &source, const QString &dest)
{
    m_values.insert(dest, m_values.value(source));
}

bool MakeStep::init(const QString &buildConfiguration)
{
    ProjectExplorer::BuildConfiguration *bc = m_pro->buildConfiguration(buildConfiguration);
    setBuildParser(m_pro->buildParser(bc));

    setEnabled(true);
    setWorkingDirectory(m_pro->buildDirectory(bc));

    setCommand(m_pro->toolChain(bc)->makeCommand());

    QStringList arguments = m_values.value(buildConfiguration).buildTargets;
    arguments << additionalArguments(buildConfiguration);
    setArguments(arguments); // TODO
    setEnvironment(m_pro->environment(bc));
    setIgnoreReturnValue(m_clean);

    return AbstractMakeStep::init(buildConfiguration);
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

ProjectExplorer::BuildStepConfigWidget *MakeStep::createConfigWidget()
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

CMakeProject *MakeStep::project() const
{
    return m_pro;
}

bool MakeStep::buildsTarget(const QString &buildConfiguration, const QString &target) const
{
    return m_values.value(buildConfiguration).buildTargets.contains(target);
}

void MakeStep::setBuildTarget(const QString &buildConfiguration, const QString &target, bool on)
{
    QStringList old = m_values.value(buildConfiguration).buildTargets;
    if (on && !old.contains(target))
        old << target;
    else if(!on && old.contains(target))
        old.removeOne(target);
    m_values[buildConfiguration].buildTargets = old;
}

QStringList MakeStep::additionalArguments(const QString &buildConfiguration) const
{
    return m_values.value(buildConfiguration).additionalArguments;
}

void MakeStep::setAdditionalArguments(const QString &buildConfiguration, const QStringList &list)
{
    m_values[buildConfiguration].additionalArguments = list;
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
    CMakeProject *pro = m_makeStep->project();
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
    m_makeStep->setAdditionalArguments(m_buildConfiguration, ProjectExplorer::Environment::parseCombinedArgString(m_additionalArguments->text()));
    updateDetails();
}

void MakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(m_buildConfiguration, item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

QString MakeStepConfigWidget::displayName() const
{
    return "Make";
}

void MakeStepConfigWidget::init(const QString &buildConfiguration)
{
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

    m_additionalArguments->setText(ProjectExplorer::Environment::joinArgumentList(m_makeStep->additionalArguments(m_buildConfiguration)));
    updateDetails();
}

void MakeStepConfigWidget::updateDetails()
{
    QStringList arguments = m_makeStep->m_values.value(m_buildConfiguration).buildTargets;
    arguments << m_makeStep->additionalArguments(m_buildConfiguration);
    m_summaryText = tr("<b>Make:</b> %1 %2")
                    .arg(m_makeStep->project()->toolChain(
                            m_makeStep->project()->buildConfiguration(m_buildConfiguration))
                            ->makeCommand(),
                         arguments.join(" "));
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

ProjectExplorer::BuildStep *MakeStepFactory::create(ProjectExplorer::Project *project, const QString &name) const
{
    Q_ASSERT(name == Constants::MAKESTEP);
    CMakeProject *pro = qobject_cast<CMakeProject *>(project);
    Q_ASSERT(pro);
    return new MakeStep(pro);
}

QStringList MakeStepFactory::canCreateForProject(ProjectExplorer::Project * /* pro */) const
{
    return QStringList();
}

QString MakeStepFactory::displayNameForName(const QString & /* name */) const
{
    return "Make";
}

