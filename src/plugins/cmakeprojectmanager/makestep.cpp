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

#include "makestep.h"

#include "cmakeprojectconstants.h"
#include "cmakeproject.h"
#include "cmaketarget.h"
#include "cmakebuildconfiguration.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/gnumakeparser.h>

#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char * const MS_ID("CMakeProjectManager.MakeStep");

const char * const CLEAN_KEY("CMakeProjectManager.MakeStep.Clean");
const char * const BUILD_TARGETS_KEY("CMakeProjectManager.MakeStep.BuildTargets");
const char * const ADDITIONAL_ARGUMENTS_KEY("CMakeProjectManager.MakeStep.AdditionalArguments");
}

// TODO: Move progress information into an IOutputParser!

MakeStep::MakeStep(BuildConfiguration *bc) :
    AbstractProcessStep(bc, QLatin1String(MS_ID)), m_clean(false),
    m_futureInterface(0)
{
    ctor();
}

MakeStep::MakeStep(BuildConfiguration *bc, const QString &id) :
    AbstractProcessStep(bc, id), m_clean(false),
    m_futureInterface(0)
{
    ctor();
}

MakeStep::MakeStep(BuildConfiguration *bc, MakeStep *bs) :
    AbstractProcessStep(bc, bs),
    m_clean(bs->m_clean),
    m_futureInterface(0),
    m_buildTargets(bs->m_buildTargets),
    m_additionalArguments(bs->m_buildTargets)
{
    ctor();
}

void MakeStep::ctor()
{
    m_percentProgress = QRegExp("^\\[\\s*(\\d*)%\\]");
    setDisplayName(tr("Make", "CMakeProjectManager::MakeStep display name."));
}

MakeStep::~MakeStep()
{
}

CMakeBuildConfiguration *MakeStep::cmakeBuildConfiguration() const
{
    return static_cast<CMakeBuildConfiguration *>(buildConfiguration());
}

void MakeStep::setClean(bool clean)
{
    m_clean = clean;
}

QVariantMap MakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(QLatin1String(CLEAN_KEY), m_clean);
    map.insert(QLatin1String(BUILD_TARGETS_KEY), m_buildTargets);
    map.insert(QLatin1String(ADDITIONAL_ARGUMENTS_KEY), m_additionalArguments);
    return map;
}

bool MakeStep::fromMap(const QVariantMap &map)
{
    m_clean = map.value(QLatin1String(CLEAN_KEY)).toBool();
    m_buildTargets = map.value(QLatin1String(BUILD_TARGETS_KEY)).toStringList();
    m_additionalArguments = map.value(QLatin1String(ADDITIONAL_ARGUMENTS_KEY)).toStringList();

    return BuildStep::fromMap(map);
}


bool MakeStep::init()
{
    CMakeBuildConfiguration *bc = cmakeBuildConfiguration();

    setEnabled(true);
    setWorkingDirectory(bc->buildDirectory());

    setCommand(bc->toolChain()->makeCommand());

    QStringList arguments = m_buildTargets;
    arguments << additionalArguments();
    setArguments(arguments);
    setEnvironment(bc->environment());
    setIgnoreReturnValue(m_clean);

    setOutputParser(new ProjectExplorer::GnuMakeParser(workingDirectory()));
    if (bc->toolChain())
        appendOutputParser(bc->toolChain()->outputParser());

    return AbstractProcessStep::init();
}

void MakeStep::run(QFutureInterface<bool> &fi)
{
    m_futureInterface = &fi;
    m_futureInterface->setProgressRange(0, 100);
    AbstractProcessStep::run(fi);
    m_futureInterface->setProgressValue(100);
    m_futureInterface->reportFinished();
    m_futureInterface = 0;
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
    AbstractProcessStep::stdOutput(line);
}

bool MakeStep::buildsBuildTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void MakeStep::setBuildTarget(const QString &buildTarget, bool on)
{
    QStringList old = m_buildTargets;
    if (on && !old.contains(buildTarget))
        old << buildTarget;
    else if(!on && old.contains(buildTarget))
        old.removeOne(buildTarget);
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

    m_buildTargetsList = new QListWidget;
    m_buildTargetsList->setMinimumHeight(200);
    fl->addRow(tr("Targets:"), m_buildTargetsList);

    // TODO update this list also on rescans of the CMakeLists.txt
    // TODO shouldn't be accessing project
    CMakeProject *pro = m_makeStep->cmakeBuildConfiguration()->cmakeTarget()->cmakeProject();
    foreach(const QString& buildTarget, pro->buildTargetTitles()) {
        QListWidgetItem *item = new QListWidgetItem(buildTarget, m_buildTargetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }

    connect(m_buildTargetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
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
    return tr("Make", "CMakeProjectManager::MakeStepConfigWidget display name.");
}

void MakeStepConfigWidget::init()
{
    // disconnect to make the changes to the items
    disconnect(m_buildTargetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
    int count = m_buildTargetsList->count();
    for(int i = 0; i < count; ++i) {
        QListWidgetItem *item = m_buildTargetsList->item(i);
        item->setCheckState(m_makeStep->buildsBuildTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }
    // and connect again
    connect(m_buildTargetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));

    m_additionalArguments->setText(Environment::joinArgumentList(m_makeStep->additionalArguments()));
    updateDetails();

    CMakeProject *pro = m_makeStep->cmakeBuildConfiguration()->cmakeTarget()->cmakeProject();
    connect(pro, SIGNAL(buildTargetsChanged()),
            this, SLOT(buildTargetsChanged()));
}

void MakeStepConfigWidget::buildTargetsChanged()
{
    disconnect(m_buildTargetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
    m_buildTargetsList->clear();
    CMakeProject *pro = m_makeStep->cmakeBuildConfiguration()->cmakeTarget()->cmakeProject();
    foreach(const QString& buildTarget, pro->buildTargetTitles()) {
        QListWidgetItem *item = new QListWidgetItem(buildTarget, m_buildTargetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_makeStep->buildsBuildTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }
    connect(m_buildTargetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
    updateSummary();
}

void MakeStepConfigWidget::updateDetails()
{
    QStringList arguments = m_makeStep->m_buildTargets;
    arguments << m_makeStep->additionalArguments();

    CMakeBuildConfiguration *bc = m_makeStep->cmakeBuildConfiguration();
    ProjectExplorer::ToolChain *tc = bc->toolChain();
    if (tc)
        m_summaryText = tr("<b>Make:</b> %1 %2").arg(tc->makeCommand(), arguments.join(QString(QLatin1Char(' '))));
    else
        m_summaryText = tr("<b>Unknown Toolchain</b>");
    emit updateSummary();
}

QString MakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

//
// MakeStepFactory
//

MakeStepFactory::MakeStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

MakeStepFactory::~MakeStepFactory()
{
}

bool MakeStepFactory::canCreate(BuildConfiguration *parent, const QString &id) const
{
    if (!qobject_cast<CMakeBuildConfiguration *>(parent))
        return false;
    return QLatin1String(MS_ID) == id;
}

BuildStep *MakeStepFactory::create(BuildConfiguration *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new MakeStep(parent);
}

bool MakeStepFactory::canClone(BuildConfiguration *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

BuildStep *MakeStepFactory::clone(BuildConfiguration *parent, BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new MakeStep(parent, static_cast<MakeStep *>(source));
}

bool MakeStepFactory::canRestore(BuildConfiguration *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

BuildStep *MakeStepFactory::restore(BuildConfiguration *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    MakeStep *bs(new MakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList MakeStepFactory::availableCreationIds(ProjectExplorer::BuildConfiguration *parent) const
{
    if (!qobject_cast<CMakeBuildConfiguration *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(MS_ID);
}

QString MakeStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(MS_ID))
        return tr("Make", "Display name for CMakeProjectManager::MakeStep id.");
    return QString();
}
