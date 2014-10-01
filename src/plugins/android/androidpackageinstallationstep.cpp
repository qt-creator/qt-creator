/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidpackageinstallationstep.h"
#include "androidmanager.h"
#include "androidconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/gnumakeparser.h>
#include <utils/hostosinfo.h>
#include <qmakeprojectmanager/qmakeparser.h>

#include <QDir>

using namespace Android::Internal;

const Core::Id AndroidPackageInstallationStep::Id = Core::Id("Qt4ProjectManager.AndroidPackageInstallationStep");
namespace {
const char ANDROIDDIRECTORY[] = "Android.AndroidPackageInstallationStep.AndroidDirectory";
}

AndroidPackageInstallationStep::AndroidPackageInstallationStep(AndroidDirectory mode,ProjectExplorer::BuildStepList *bsl)
    : AbstractProcessStep(bsl, Id), m_androidDirectory(mode)
{
    const QString name = tr("Copy application data");
    setDefaultDisplayName(name);
    setDisplayName(name);
}

AndroidPackageInstallationStep::AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bc, AndroidPackageInstallationStep *other)
    : AbstractProcessStep(bc, other)
{ }

bool AndroidPackageInstallationStep::init()
{
    ProjectExplorer::BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();
    QString dirPath;
    if (m_androidDirectory == ProjectDirectory)
        dirPath = AndroidManager::dirPath(target()).toString();
    else
        dirPath = bc->buildDirectory().appendPath(QLatin1String(Constants::ANDROID_BUILDDIRECTORY)).toString();
    if (Utils::HostOsInfo::isWindowsHost())
        if (bc->environment().searchInPath(QLatin1String("sh.exe")).isEmpty())
            dirPath = QDir::toNativeSeparators(dirPath);

    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());

    ProjectExplorer::ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    pp->setCommand(tc->makeCommand(bc->environment()));
    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    pp->setEnvironment(env);
    pp->setArguments(QString::fromLatin1("INSTALL_ROOT=\"%1\" install").arg(dirPath));
    pp->resolveAll();
    setOutputParser(new ProjectExplorer::GnuMakeParser());
    ProjectExplorer::IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    m_androidDirToClean = m_androidDirectory == BuildDirectory ? dirPath : QString();

    return AbstractProcessStep::init();
}

bool AndroidPackageInstallationStep::fromMap(const QVariantMap &map)
{
    if (!AbstractProcessStep::fromMap(map))
        return false;
    m_androidDirectory = AndroidDirectory(map.value(QLatin1String(ANDROIDDIRECTORY)).toInt());
    return true;
}

void AndroidPackageInstallationStep::run(QFutureInterface<bool> &fi)
{
    QString error;
    Utils::FileName androidDir = Utils::FileName::fromString(m_androidDirToClean);
    if (!m_androidDirToClean.isEmpty()&& androidDir.toFileInfo().exists()) {
        emit addOutput(tr("Removing directory %1").arg(m_androidDirToClean), MessageOutput);
        if (!Utils::FileUtils::removeRecursively(androidDir, &error)) {
            emit addOutput(error, ErrorOutput);
            fi.reportResult(false);
            emit finished();
            return;
        }
    }
    AbstractProcessStep::run(fi);
}

QVariantMap AndroidPackageInstallationStep::toMap() const
{
    QVariantMap map = AbstractProcessStep::toMap();
    map.insert(QLatin1String(ANDROIDDIRECTORY), m_androidDirectory);
    return map;
}

ProjectExplorer::BuildStepConfigWidget *AndroidPackageInstallationStep::createConfigWidget()
{
    return new AndroidPackageInstallationStepWidget(this);
}


bool AndroidPackageInstallationStep::immutable() const
{
    return true;
}

//
// AndroidPackageInstallationStepWidget
//

AndroidPackageInstallationStepWidget::AndroidPackageInstallationStepWidget(AndroidPackageInstallationStep *step)
    : m_step(step)
{

}

QString AndroidPackageInstallationStepWidget::summaryText() const
{
    return tr("<b>Make install</b>");
}

QString AndroidPackageInstallationStepWidget::displayName() const
{
    return tr("Make install");
}

bool AndroidPackageInstallationStepWidget::showWidget() const
{
    return false;
}
