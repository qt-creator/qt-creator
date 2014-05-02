/****************************************************************************
**
** Copyright (C) 2014 Tim Sander <tim@krieglstein.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

class BareMetalRunConfigurationPrivate
{
public:
    BareMetalRunConfigurationPrivate(const QString &projectFilePath)
        : projectFilePath(projectFilePath)
    {
    }

    BareMetalRunConfigurationPrivate(const BareMetalRunConfigurationPrivate *other)
        : projectFilePath(other->projectFilePath),
          gdbPath(other->gdbPath),
          arguments(other->arguments),
          workingDirectory(other->workingDirectory)
    {
    }

    QString projectFilePath;
    QString gdbPath;
    QString arguments;
    QString disabledReason;
    QString workingDirectory;
};

} // namespace Internal

using namespace Internal;

BareMetalRunConfiguration::BareMetalRunConfiguration(Target *parent, BareMetalRunConfiguration *source)
    : RunConfiguration(parent, source),
      d(new BareMetalRunConfigurationPrivate(source->d))
{
    init();
}

BareMetalRunConfiguration::BareMetalRunConfiguration(Target *parent,
                                                     const Core::Id id,
                                                     const QString &projectFilePath)
    : RunConfiguration(parent,id),
      d(new BareMetalRunConfigurationPrivate(projectFilePath))
{
    init();
}

BareMetalRunConfiguration::~BareMetalRunConfiguration()
{
    delete d;
}

void BareMetalRunConfiguration::init()
{
    setDefaultDisplayName(defaultDisplayName());

    connect(target(), SIGNAL(deploymentDataChanged()), SLOT(handleBuildSystemDataUpdated()));
    connect(target(), SIGNAL(applicationTargetsChanged()), SLOT(handleBuildSystemDataUpdated()));
    connect(target(), SIGNAL(kitChanged()),
            this, SLOT(handleBuildSystemDataUpdated())); // Handles device changes, etc.
}

bool BareMetalRunConfiguration::isEnabled() const
{
    d->disabledReason.clear();
    return true;
}

QString BareMetalRunConfiguration::disabledReason() const
{
    return d->disabledReason;
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
    map.insert(QLatin1String(ArgumentsKey), d->arguments);
    const QDir dir = QDir(target()->project()->projectDirectory().toString());
    map.insert(QLatin1String(ProFileKey), dir.relativeFilePath(d->projectFilePath));
    map.insert(QLatin1String(WorkingDirectoryKey), d->workingDirectory);
    return map;
}

bool BareMetalRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    d->arguments = map.value(QLatin1String(ArgumentsKey)).toString();
    const QDir dir = QDir(target()->project()->projectDirectory().toString());
    d->projectFilePath
            = QDir::cleanPath(dir.filePath(map.value(QLatin1String(ProFileKey)).toString()));
    d->workingDirectory = map.value(QLatin1String(WorkingDirectoryKey)).toString();

    setDefaultDisplayName(defaultDisplayName());

    return true;
}

QString BareMetalRunConfiguration::defaultDisplayName()
{
    if (!d->projectFilePath.isEmpty())
        //: %1 is the name of the project run via hardware debugger
        return tr("%1 (via GDB server or hardware debugger)").arg(QFileInfo(d->projectFilePath).completeBaseName());
    //: Bare Metal run configuration default run name
    return tr("Run on GDB server or hardware debugger");
}

QString BareMetalRunConfiguration::localExecutableFilePath() const
{
    return target()->applicationTargets()
            .targetForProject(Utils::FileName::fromString(d->projectFilePath)).toString();
}

QString BareMetalRunConfiguration::arguments() const
{
    return d->arguments;
}

void BareMetalRunConfiguration::setArguments(const QString &args)
{
    d->arguments = args;
}

QString BareMetalRunConfiguration::workingDirectory() const
{
    return d->workingDirectory;
}

void BareMetalRunConfiguration::setWorkingDirectory(const QString &wd)
{
    d->workingDirectory = wd;
}

QString BareMetalRunConfiguration::projectFilePath() const
{
    return d->projectFilePath;
}

void BareMetalRunConfiguration::setDisabledReason(const QString &reason) const
{
    d->disabledReason = reason;
}

void BareMetalRunConfiguration::handleBuildSystemDataUpdated()
{
    emit targetInformationChanged();
    updateEnableState();
}

const char *BareMetalRunConfiguration::IdPrefix = "BareMetalRunConfiguration";

} // namespace BareMetal

