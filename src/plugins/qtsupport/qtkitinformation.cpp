/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "qtkitinformation.h"

#include "qtkitconfigwidget.h"
#include "qtversionmanager.h"

#include <utils/environment.h>

namespace QtSupport {
namespace Internal {
const char QT_INFORMATION[] = "QtSupport.QtInformation";
} // namespace Internal

QtKitInformation::QtKitInformation()
{
    setObjectName(QLatin1String("QtKitInformation"));
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SIGNAL(validationNeeded()));
}

Core::Id QtKitInformation::dataId() const
{
    static Core::Id id = Core::Id(Internal::QT_INFORMATION);
    return id;
}

unsigned int QtKitInformation::priority() const
{
    return 26000;
}

QVariant QtKitInformation::defaultValue(ProjectExplorer::Kit *p) const
{
    Q_UNUSED(p);
    QtVersionManager *mgr = QtVersionManager::instance();

    // find "Qt in PATH":
    Utils::Environment env = Utils::Environment::systemEnvironment();
    Utils::FileName qmake = Utils::FileName::fromString(env.searchInPath(QLatin1String("qmake")));

    if (qmake.isEmpty())
        return -1;

    QList<BaseQtVersion *> versionList = mgr->versions();
    foreach (BaseQtVersion *v, versionList) {
        if (qmake == v->qmakeCommand())
            return v->uniqueId();
    }

    return -1;
}

QList<ProjectExplorer::Task> QtKitInformation::validate(ProjectExplorer::Kit *p) const
{
    int id = qtVersionId(p);
    if (id == -1)
        return QList<ProjectExplorer::Task>();
    BaseQtVersion *version = QtVersionManager::instance()->version(id);
    if (!version) {
        setQtVersionId(p, -1);
        return QList<ProjectExplorer::Task>();
    }
    return version->validateKit(p);
}

ProjectExplorer::KitConfigWidget *QtKitInformation::createConfigWidget(ProjectExplorer::Kit *p) const
{
    return new Internal::QtKitConfigWidget(p);
}

QString QtKitInformation::displayNamePostfix(const ProjectExplorer::Kit *p) const
{
    BaseQtVersion *version = qtVersion(p);
    return version ? version->displayName() : QString();
}

ProjectExplorer::KitInformation::ItemList
QtKitInformation::toUserOutput(ProjectExplorer::Kit *p) const
{
    BaseQtVersion *version = qtVersion(p);
    return ItemList() << qMakePair(tr("Qt version"), version ? version->displayName() : tr("None"));
}

void QtKitInformation::addToEnvironment(const ProjectExplorer::Kit *p, Utils::Environment &env) const
{
    BaseQtVersion *version = qtVersion(p);
    if (version)
        version->addToEnvironment(p, env);
}

int QtKitInformation::qtVersionId(const ProjectExplorer::Kit *p)
{
    if (!p)
        return -1;
    bool ok = false;
    int id = p->value(Core::Id(Internal::QT_INFORMATION), -1).toInt(&ok);
    if (!ok)
        id = -1;
    return id;
}

void QtKitInformation::setQtVersionId(ProjectExplorer::Kit *p, const int id)
{
    p->setValue(Core::Id(Internal::QT_INFORMATION), id);
}

BaseQtVersion *QtKitInformation::qtVersion(const ProjectExplorer::Kit *p)
{
    return QtVersionManager::instance()->version(qtVersionId(p));
}

void QtKitInformation::setQtVersion(ProjectExplorer::Kit *p, const BaseQtVersion *v)
{
    if (!v)
        setQtVersionId(p, -1);
    else
        setQtVersionId(p, v->uniqueId());
}

QtPlatformKitMatcher::QtPlatformKitMatcher(const QString &platform) :
    m_platform(platform)
{ }

bool QtPlatformKitMatcher::matches(const ProjectExplorer::Kit *p) const
{
    BaseQtVersion *version = QtKitInformation::qtVersion(p);
    return version && version->platformName() == m_platform;
}

bool QtVersionKitMatcher::matches(const ProjectExplorer::Kit *p) const
{
    BaseQtVersion *version = QtKitInformation::qtVersion(p);
    if (!version)
        return false;
    QtVersionNumber current = version->qtVersion();
    if (m_min.majorVersion > -1 && current < m_min)
        return false;
    if (m_max.majorVersion > -1 && current > m_max)
        return false;
    return version->availableFeatures().contains(m_features);
}

} // namespace QtSupport
