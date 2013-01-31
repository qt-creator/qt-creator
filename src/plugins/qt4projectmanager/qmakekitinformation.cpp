/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmakekitinformation.h"

#include "qmakekitinformation.h"
#include "qmakekitconfigwidget.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

namespace Qt4ProjectManager {
namespace Internal {
const char MKSPEC_INFORMATION[] = "QtPM4.mkSpecInformation";
} // namespace Internal

QmakeKitInformation::QmakeKitInformation()
{
    setObjectName(QLatin1String("QmakeKitInformation"));
}

Core::Id QmakeKitInformation::dataId() const
{
    static Core::Id id = Core::Id(Internal::MKSPEC_INFORMATION);
    return id;
}

unsigned int QmakeKitInformation::priority() const
{
    return 24000;
}

QVariant QmakeKitInformation::defaultValue(ProjectExplorer::Kit *k) const
{
    Q_UNUSED(k);
    return QString();
}

QList<ProjectExplorer::Task> QmakeKitInformation::validate(const ProjectExplorer::Kit *k) const
{
    QList<ProjectExplorer::Task> result;
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);

    Utils::FileName mkspec = QmakeKitInformation::mkspec(k);
    if (!version && !mkspec.isEmpty())
        result << ProjectExplorer::Task(ProjectExplorer::Task::Warning,
                                        tr("No Qt version set, so mkspec is ignored."),
                                        Utils::FileName(), -1,
                                        Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    if (version && !version->hasMkspec(mkspec))
        result << ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                        tr("Mkspec not found for Qt version."),
                                        Utils::FileName(), -1,
                                        Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    return result;
}

void QmakeKitInformation::setup(ProjectExplorer::Kit *k)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version)
        return;

    Utils::FileName spec = QmakeKitInformation::mkspec(k);
    if (spec.isEmpty())
        spec = version->mkspec();

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);

    if (!tc || !tc->suggestedMkspecList().contains(spec)) {
        QList<ProjectExplorer::ToolChain *> tcList = ProjectExplorer::ToolChainManager::instance()->toolChains();
        foreach (ProjectExplorer::ToolChain *current, tcList) {
            if (version->qtAbis().contains(current->targetAbi())
                    && current->suggestedMkspecList().contains(spec)) {
                ProjectExplorer::ToolChainKitInformation::setToolChain(k, current);
                break;
            }
        }
    }
}

ProjectExplorer::KitConfigWidget *
QmakeKitInformation::createConfigWidget(ProjectExplorer::Kit *k) const
{
    return new Internal::QmakeKitConfigWidget(k);
}

ProjectExplorer::KitInformation::ItemList QmakeKitInformation::toUserOutput(ProjectExplorer::Kit *k) const
{
    return ItemList() << qMakePair(tr("mkspec"), mkspec(k).toUserOutput());
}

Utils::FileName QmakeKitInformation::mkspec(const ProjectExplorer::Kit *k)
{
    if (!k)
        return Utils::FileName();
    return Utils::FileName::fromString(k->value(Core::Id(Internal::MKSPEC_INFORMATION)).toString());
}

Utils::FileName QmakeKitInformation::effectiveMkspec(const ProjectExplorer::Kit *k)
{
    if (!k)
        return Utils::FileName();
    Utils::FileName spec = mkspec(k);
    if (spec.isEmpty())
        return defaultMkspec(k);
    return spec;
}

void QmakeKitInformation::setMkspec(ProjectExplorer::Kit *k, const Utils::FileName &fn)
{
    if (fn == defaultMkspec(k))
        k->setValue(Core::Id(Internal::MKSPEC_INFORMATION), QString());
    else
        k->setValue(Core::Id(Internal::MKSPEC_INFORMATION), fn.toString());
}

Utils::FileName QmakeKitInformation::defaultMkspec(const ProjectExplorer::Kit *k)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version) // No version, so no qmake
        return Utils::FileName();

    return version->mkspecFor(ProjectExplorer::ToolChainKitInformation::toolChain(k));
}

} // namespace Qt4ProjectManager
