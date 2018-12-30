/****************************************************************************
**
** Copyright (C) 2018 Jochen Seemann
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

#include "conaninstallstep.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>
#include <utils/qtcprocess.h>

#include <QVariantMap>
#include <QLineEdit>
#include <QFormLayout>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;
using namespace Utils;

namespace ConanPackageManager {
namespace Internal {

const char INSTALL_STEP_ID[] = "ConanPackageManager.InstallStep";
const char INSTALL_STEP_CONANFILE_SUBDIR_KEY[] = "ConanPackageManager.InstallStep.ConanfileSubdir";
const char INSTALL_STEP_ADDITIONAL_ARGUMENTS_KEY[] = "ConanPackageManager.InstallStep.AdditionalArguments";

ConanInstallStepFactory::ConanInstallStepFactory()
{
    registerStep<ConanInstallStep>(INSTALL_STEP_ID);
    setDisplayName(ConanInstallStep::tr("conan install", "Install packages specified in Conanfile.txt"));
}

ConanInstallStep::ConanInstallStep(BuildStepList *bsl, Utils::Id id) : AbstractProcessStep(bsl, id)
{
    setDefaultDisplayName(tr("conan install"));
}

bool ConanInstallStep::init()
{
    BuildConfiguration *bc = buildConfiguration();
    QTC_ASSERT(bc, return false);

    QList<ToolChain *> tcList = ToolChainKitAspect::toolChains(target()->kit());
    if (tcList.isEmpty())
        emit addTask(Task::compilerMissingTask());

    if (tcList.isEmpty() || !bc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    ProcessParameters *pp = processParameters();
    Utils::Environment env = bc->environment();
    Utils::Environment::setupEnglishOutput(&env);
    pp->setEnvironment(env);
    pp->setWorkingDirectory(bc->buildDirectory());
    pp->setCommandLine(Utils::CommandLine("conan", arguments()));

    return AbstractProcessStep::init();
}

void ConanInstallStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    formatter->addLineParsers(kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

BuildStepConfigWidget *ConanInstallStep::createConfigWidget()
{
    return new ConanInstallStepConfigWidget(this);
}

bool ConanInstallStep::immutable() const
{
    return false;
}

QStringList ConanInstallStep::arguments() const
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();

    if (!bc)
        return {};

    const Utils::FilePath conanFileDir = relativeSubdir().isEmpty()
                                             ? bc->buildDirectory()
                                             : bc->buildDirectory().pathAppended(relativeSubdir());

    const QString buildType = bc->buildType() == BuildConfiguration::Release ? QString("Release")
                                                                             : QString("Debug");
    const QString relativePath
        = bc->project()->projectDirectory().relativeChildPath(conanFileDir).toString();
    QStringList installArguments;
    installArguments << "install";
    if (!relativePath.isEmpty())
        installArguments << relativePath;
    installArguments << "-s";
    installArguments << "build_type=" + buildType;
    installArguments << conanFileDir.toString();
    installArguments << additionalArguments();
    return installArguments;
}

QString ConanInstallStep::relativeSubdir() const
{
    return m_relativeSubdir;
}

QStringList ConanInstallStep::additionalArguments() const
{
    return Utils::QtcProcess::splitArgs(m_additionalArguments);
}

void ConanInstallStep::setRelativeSubdir(const QString &subdir)
{
    if (subdir == m_relativeSubdir)
        return;

    m_relativeSubdir = subdir;
    emit relativeSubdirChanged(subdir);
}

void ConanInstallStep::setAdditionalArguments(const QString &list)
{
    if (list == m_additionalArguments)
        return;

    m_additionalArguments = list;

    emit additionalArgumentsChanged(list);
}

QVariantMap ConanInstallStep::toMap() const
{
    QVariantMap map = AbstractProcessStep::toMap();

    map.insert(INSTALL_STEP_CONANFILE_SUBDIR_KEY, m_relativeSubdir);
    map.insert(INSTALL_STEP_ADDITIONAL_ARGUMENTS_KEY, m_additionalArguments);
    return map;
}

bool ConanInstallStep::fromMap(const QVariantMap &map)
{
    m_relativeSubdir = map.value(INSTALL_STEP_CONANFILE_SUBDIR_KEY).toString();
    m_additionalArguments = map.value(INSTALL_STEP_ADDITIONAL_ARGUMENTS_KEY).toString();

    return BuildStep::fromMap(map);
}

///////////////////////////////
// ConanInstallStepConfigWidget class
///////////////////////////////
ConanInstallStepConfigWidget::ConanInstallStepConfigWidget(ConanInstallStep *installStep)
    : ProjectExplorer::BuildStepConfigWidget(installStep)
    , m_installStep(installStep)
    , m_summaryText()
    , m_relativeSubdir(nullptr)
    , m_additionalArguments(nullptr)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    m_relativeSubdir = new QLineEdit(this);
    fl->addRow(tr("conanfile.txt subdirectory:"), m_relativeSubdir);
    m_relativeSubdir->setText(m_installStep->relativeSubdir());

    m_additionalArguments = new QLineEdit(this);
    fl->addRow(tr("Additional arguments:"), m_additionalArguments);
    m_additionalArguments->setText(Utils::QtcProcess::joinArgs(m_installStep->additionalArguments()));

    updateDetails();

    connect(m_relativeSubdir, &QLineEdit::textChanged,
            m_installStep, &ConanInstallStep::setRelativeSubdir);
    connect(m_additionalArguments, &QLineEdit::textChanged,
            m_installStep, &ConanInstallStep::setAdditionalArguments);

    connect(m_installStep, &ConanInstallStep::relativeSubdirChanged,
            this, &ConanInstallStepConfigWidget::updateDetails);
    connect(m_installStep, &ConanInstallStep::additionalArgumentsChanged,
            this, &ConanInstallStepConfigWidget::updateDetails);
    connect(installStep->buildConfiguration(), &BuildConfiguration::environmentChanged,
            this, &ConanInstallStepConfigWidget::updateDetails);
}

QString ConanInstallStepConfigWidget::displayName() const
{
    return tr("conan install", "ConanInstallStep::ConanInstallStepConfigWidget display name.");
}

QString ConanInstallStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void ConanInstallStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_installStep->buildConfiguration();
    if (!bc)
        bc = m_installStep->target()->activeBuildConfiguration();
    QList<ToolChain *> tcList = ToolChainKitAspect::toolChains(m_installStep->target()->kit());

    if (!tcList.isEmpty()) {
        QString arguments = Utils::QtcProcess::joinArgs(m_installStep->arguments()
                                                        + m_installStep->additionalArguments());
        m_summaryText = "<b>conan install</b>: conan " + arguments;
        setSummaryText(m_summaryText);
    } else {
        m_summaryText = "<b>" + ToolChainKitAspect::msgNoToolChainInTarget()  + "</b>";
    }

    emit updateSummary();
}

} // namespace Internal
} // namespace ConanPackageManager
