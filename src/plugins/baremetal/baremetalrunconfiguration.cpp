/****************************************************************************
**
** Copyright (C) 2015 Tim Sander <tim@krieglstein.org>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baremetalrunconfiguration.h"

#include "baremetalrunconfigurationwidget.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtoutputformatter.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

const char ArgumentsKey[] = "Qt4ProjectManager.MaemoRunConfiguration.Arguments";
const char ProFileKey[] = "Qt4ProjectManager.MaemoRunConfiguration.ProFile";
const char WorkingDirectoryKey[] = "BareMetal.RunConfig.WorkingDirectory";


BareMetalRunConfiguration::BareMetalRunConfiguration(Target *parent, BareMetalRunConfiguration *other)
    : RunConfiguration(parent, other),
      m_projectFilePath(other->m_projectFilePath),
      m_arguments(other->m_arguments),
      m_workingDirectory(other->m_workingDirectory)
{
    init();
}

BareMetalRunConfiguration::BareMetalRunConfiguration(Target *parent,
                                                     const Core::Id id,
                                                     const QString &projectFilePath)
    : RunConfiguration(parent, id),
      m_projectFilePath(projectFilePath)
{
    init();
}

void BareMetalRunConfiguration::init()
{
    setDefaultDisplayName(defaultDisplayName());

    connect(target(), &Target::deploymentDataChanged,
            this, &BareMetalRunConfiguration::handleBuildSystemDataUpdated);
    connect(target(), &Target::applicationTargetsChanged,
            this, &BareMetalRunConfiguration::handleBuildSystemDataUpdated);
    connect(target(), &Target::kitChanged,
            this, &BareMetalRunConfiguration::handleBuildSystemDataUpdated); // Handles device changes, etc.
}

bool BareMetalRunConfiguration::isEnabled() const
{
    m_disabledReason.clear(); // FIXME: Check this makes sense.
    return true;
}

QString BareMetalRunConfiguration::disabledReason() const
{
    return m_disabledReason;
}

QWidget *BareMetalRunConfiguration::createConfigurationWidget()
{
    return new BareMetalRunConfigurationWidget(this);
}

OutputFormatter *BareMetalRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

QVariantMap BareMetalRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(QLatin1String(ArgumentsKey), m_arguments);
    const QDir dir = QDir(target()->project()->projectDirectory().toString());
    map.insert(QLatin1String(ProFileKey), dir.relativeFilePath(m_projectFilePath));
    map.insert(QLatin1String(WorkingDirectoryKey), m_workingDirectory);
    return map;
}

bool BareMetalRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    m_arguments = map.value(QLatin1String(ArgumentsKey)).toString();
    const QDir dir = QDir(target()->project()->projectDirectory().toString());
    m_projectFilePath
            = QDir::cleanPath(dir.filePath(map.value(QLatin1String(ProFileKey)).toString()));
    m_workingDirectory = map.value(QLatin1String(WorkingDirectoryKey)).toString();

    setDefaultDisplayName(defaultDisplayName());

    return true;
}

QString BareMetalRunConfiguration::defaultDisplayName()
{
    if (!m_projectFilePath.isEmpty())
        //: %1 is the name of the project run via hardware debugger
        return tr("%1 (via GDB server or hardware debugger)").arg(QFileInfo(m_projectFilePath).completeBaseName());
    //: Bare Metal run configuration default run name
    return tr("Run on GDB server or hardware debugger");
}

QString BareMetalRunConfiguration::localExecutableFilePath() const
{
    return target()->applicationTargets()
            .targetForProject(FileName::fromString(m_projectFilePath)).toString();
}

QString BareMetalRunConfiguration::arguments() const
{
    return m_arguments;
}

void BareMetalRunConfiguration::setArguments(const QString &args)
{
    m_arguments = args;
}

QString BareMetalRunConfiguration::workingDirectory() const
{
    return m_workingDirectory;
}

void BareMetalRunConfiguration::setWorkingDirectory(const QString &wd)
{
    m_workingDirectory = wd;
}

QString BareMetalRunConfiguration::projectFilePath() const
{
    return m_projectFilePath;
}

void BareMetalRunConfiguration::setDisabledReason(const QString &reason) const
{
    m_disabledReason = reason;
}

void BareMetalRunConfiguration::handleBuildSystemDataUpdated()
{
    emit targetInformationChanged();
    updateEnableState();
}

const char *BareMetalRunConfiguration::IdPrefix = "BareMetal";

} // namespace Internal
} // namespace BareMetal

