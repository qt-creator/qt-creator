/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
**************************************************************************/

#include "blackberrydeployinformation.h"

#include "blackberrydeployconfiguration.h"

#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char COUNT_KEY[]      = "Qnx.BlackBerry.DeployInformationCount";
const char DEPLOYINFO_KEY[] = "Qnx.BlackBerry.DeployInformation.%1";

const char ENABLED_KEY[]        = "Qnx.BlackBerry.DeployInformation.Enabled";
const char APPDESCRIPTOR_KEY[]  = "Qnx.BlackBerry.DeployInformation.AppDescriptor";
const char PACKAGE_KEY[]        = "Qnx.BlackBerry.DeployInformation.Package";
const char PROFILE_KEY[]        = "Qnx.BlackBerry.DeployInformation.ProFile";
}

BlackBerryDeployInformation::BlackBerryDeployInformation(Qt4ProjectManager::Qt4Project *project)
    : QAbstractTableModel(project)
    , m_project(project)
{
    connect(m_project, SIGNAL(proFilesEvaluated()), this, SLOT(updateModel()));
}

int BlackBerryDeployInformation::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_deployInformation.count();
}

int BlackBerryDeployInformation::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return ColumnCount;
}

QVariant BlackBerryDeployInformation::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= m_deployInformation.count() || index.column() >= ColumnCount)
        return QVariant();

    BarPackageDeployInformation di = m_deployInformation[index.row()];
    if (role == Qt::CheckStateRole) {
        if (index.column() == EnabledColumn)
            return di.enabled ? Qt::Checked : Qt::Unchecked;
    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (index.column() == AppDescriptorColumn)
            return di.appDescriptorPath;
        else if (index.column() == PackageColumn)
            return di.packagePath;
    }

    return QVariant();
}

QVariant BlackBerryDeployInformation::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case EnabledColumn:
        return tr("Enabled");
    case AppDescriptorColumn:
        return tr("Application descriptor file");
    case PackageColumn:
        return tr("Package");
    default:
        return QVariant();
    }
}

bool BlackBerryDeployInformation::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (index.row() >= m_deployInformation.count() || index.column() >= ColumnCount)
        return false;

    BarPackageDeployInformation &di = m_deployInformation[index.row()];
    if (role == Qt::CheckStateRole && index.column() == EnabledColumn) {
        di.enabled = static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked;
    } else if (role == Qt::EditRole) {
        if (index.column() == AppDescriptorColumn)
            di.appDescriptorPath = value.toString();
        else if (index.column() == PackageColumn)
            di.packagePath = value.toString();
    }

    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags BlackBerryDeployInformation::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    switch (index.column()) {
    case EnabledColumn:
        flags |= Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
        break;
    case AppDescriptorColumn:
    case PackageColumn:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

QList<BarPackageDeployInformation> BlackBerryDeployInformation::enabledPackages() const
{
    QList<BarPackageDeployInformation> result;

    foreach (const BarPackageDeployInformation& info, m_deployInformation) {
        if (info.enabled)
            result << info;
    }

    return result;
}

QVariantMap BlackBerryDeployInformation::toMap() const
{
    QVariantMap outerMap;
    outerMap[QLatin1String(COUNT_KEY)] = m_deployInformation.size();

    for (int i = 0; i < m_deployInformation.size(); ++i) {
        const BarPackageDeployInformation &deployInfo = m_deployInformation[i];

        QVariantMap deployInfoMap;
        deployInfoMap[QLatin1String(ENABLED_KEY)] = deployInfo.enabled;
        deployInfoMap[QLatin1String(APPDESCRIPTOR_KEY)] = deployInfo.appDescriptorPath;
        deployInfoMap[QLatin1String(PACKAGE_KEY)] = deployInfo.packagePath;
        deployInfoMap[QLatin1String(PROFILE_KEY)] = deployInfo.proFilePath;

        outerMap[QString::fromLatin1(DEPLOYINFO_KEY).arg(i)] = deployInfoMap;
    }

    return outerMap;
}

void BlackBerryDeployInformation::fromMap(const QVariantMap &map)
{
    beginResetModel();
    m_deployInformation.clear();

    int count = map.value(QLatin1String(COUNT_KEY)).toInt();
    for (int i = 0; i < count; ++i) {
        QVariantMap innerMap = map.value(QString::fromLatin1(DEPLOYINFO_KEY).arg(i)).toMap();

        const bool enabled = innerMap.value(QLatin1String(ENABLED_KEY)).toBool();
        const QString appDescriptorPath = innerMap.value(QLatin1String(APPDESCRIPTOR_KEY)).toString();
        const QString packagePath = innerMap.value(QLatin1String(PACKAGE_KEY)).toString();
        const QString proFilePath = innerMap.value(QLatin1String(PROFILE_KEY)).toString();

        m_deployInformation << BarPackageDeployInformation(enabled, appDescriptorPath, packagePath, proFilePath);
    }

    endResetModel();
}

void BlackBerryDeployInformation::updateModel()
{
    if (m_deployInformation.isEmpty()) {
        initModel();
        return;
    }

    beginResetModel();
    QList<BarPackageDeployInformation> keep;
    QList<Qt4ProjectManager::Qt4ProFileNode *> appNodes = m_project->applicationProFiles();
    foreach (Qt4ProjectManager::Qt4ProFileNode *node, appNodes) {
        bool nodeFound = false;
        for (int i = 0; i < m_deployInformation.size(); ++i) {
            if (m_deployInformation[i].proFilePath == node->path()) {
                keep << m_deployInformation[i];
                nodeFound = true;
                break;
            }
        }

        if (!nodeFound)
            keep << deployInformationFromNode(node);
    }
    m_deployInformation = keep;
    endResetModel();
}

void BlackBerryDeployInformation::initModel()
{
    if (!m_deployInformation.isEmpty())
        return;

    ProjectExplorer::Target *target = m_project->activeTarget();
    if (!target
            || !target->activeDeployConfiguration()
            || !qobject_cast<BlackBerryDeployConfiguration *>(target->activeDeployConfiguration()))
        return;

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!version || !version->isValid()) {
        beginResetModel();
        m_deployInformation.clear();
        endResetModel();
        return;
    }

    const Qt4ProjectManager::Qt4ProFileNode *const rootNode = m_project->rootQt4ProjectNode();
    if (!rootNode || rootNode->parseInProgress()) // Can be null right after project creation by wizard.
        return;

    disconnect(m_project, SIGNAL(proFilesEvaluated()), this, SLOT(updateModel()));

    beginResetModel();
    m_deployInformation.clear();

    QList<Qt4ProjectManager::Qt4ProFileNode *> appNodes = m_project->applicationProFiles();
    foreach (Qt4ProjectManager::Qt4ProFileNode *node, appNodes)
        m_deployInformation << deployInformationFromNode(node);

    endResetModel();
    connect(m_project, SIGNAL(proFilesEvaluated()), this, SLOT(updateModel()));
}

BarPackageDeployInformation BlackBerryDeployInformation::deployInformationFromNode(Qt4ProjectManager::Qt4ProFileNode *node) const
{
    Qt4ProjectManager::TargetInformation ti = node->targetInformation();

    QFileInfo fi(node->path());
    const QString appDescriptorPath = QDir::toNativeSeparators(fi.absolutePath() + QLatin1String("/bar-descriptor.xml"));
    QString barPackagePath;
    if (!ti.buildDir.isEmpty())
        barPackagePath = QDir::toNativeSeparators(ti.buildDir + QLatin1Char('/') + ti.target + QLatin1String(".bar"));

    return BarPackageDeployInformation(true, appDescriptorPath, barPackagePath, node->path());
}
