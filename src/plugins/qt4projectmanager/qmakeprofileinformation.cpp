/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmakeprofileinformation.h"

#include "qmakeprofileinformation.h"
#include "qmakeprofileconfigwidget.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtprofileinformation.h>

namespace Qt4ProjectManager {
namespace Internal {
const char MKSPEC_INFORMATION[] = "QtPM4.mkSpecInformation";
} // namespace Internal

QmakeProfileInformation::QmakeProfileInformation()
{
    setObjectName(QLatin1String("QmakeProfileInformation"));
}

Core::Id QmakeProfileInformation::dataId() const
{
    static Core::Id id = Core::Id(Internal::MKSPEC_INFORMATION);
    return id;
}

unsigned int QmakeProfileInformation::priority() const
{
    return 24000;
}

QVariant QmakeProfileInformation::defaultValue(ProjectExplorer::Profile *p) const
{
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(p);
    if (!version) // No version, so no qmake
        return QString();

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(p);

    const QList<Utils::FileName> tcSpecList = tc ? tc->suggestedMkspecList() : QList<Utils::FileName>();
    foreach (const Utils::FileName &tcSpec, tcSpecList) {
        if (version->hasMkspec(tcSpec))
            return tcSpec.toString();
    }

    return version ? version->mkspec().toString() : QString();
}

QList<ProjectExplorer::Task> QmakeProfileInformation::validate(ProjectExplorer::Profile *p) const
{
    QList<ProjectExplorer::Task> result;
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(p);

    Utils::FileName mkspec = QmakeProfileInformation::mkspec(p);
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

ProjectExplorer::ProfileConfigWidget *
QmakeProfileInformation::createConfigWidget(ProjectExplorer::Profile *p) const
{
    return new Internal::QmakeProfileConfigWidget(p);
}

ProjectExplorer::ProfileInformation::ItemList QmakeProfileInformation::toUserOutput(ProjectExplorer::Profile *p) const
{
    return ItemList() << qMakePair(tr("mkspec"), mkspec(p).toUserOutput());
}

Utils::FileName QmakeProfileInformation::mkspec(const ProjectExplorer::Profile *p)
{
    if (!p)
        return Utils::FileName();
    return Utils::FileName::fromString(p->value(Core::Id(Internal::MKSPEC_INFORMATION)).toString());
}

void QmakeProfileInformation::setMkspec(ProjectExplorer::Profile *p, const Utils::FileName &fn)
{
    p->setValue(Core::Id(Internal::MKSPEC_INFORMATION), fn.toString());
}

} // namespace Qt4ProjectManager
