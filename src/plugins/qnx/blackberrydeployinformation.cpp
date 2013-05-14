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

#include <projectexplorer/buildconfiguration.h>
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
const char TARGET_KEY[]         = "Qnx.BlackBerry.DeployInformation.Target";
const char SOURCE_KEY[]         = "Qnx.BlackBerry.DeployInformation.Source";
}

QString BarPackageDeployInformation::appDescriptorPath() const
{
    if (userAppDescriptorPath.isEmpty())
        return sourceDir + QLatin1String("/bar-descriptor.xml");

    return userAppDescriptorPath;
}

QString BarPackageDeployInformation::packagePath() const
{
    if (userPackagePath.isEmpty())
        return buildDir + QLatin1Char('/') + targetName + QLatin1String(".bar");

    return userPackagePath;
}

// ----------------------------------------------------------------------------

BlackBerryDeployInformation::BlackBerryDeployInformation(ProjectExplorer::Target *target)
    : QAbstractTableModel(target)
    , m_target(target)
{
    connect(project(), SIGNAL(proFilesEvaluated()), this, SLOT(updateModel()));
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
            return QDir::toNativeSeparators(di.appDescriptorPath());
        else if (index.column() == PackageColumn)
            return QDir::toNativeSeparators(di.packagePath());
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
            di.userAppDescriptorPath = value.toString();
        else if (index.column() == PackageColumn)
            di.userPackagePath = value.toString();
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
        deployInfoMap[QLatin1String(APPDESCRIPTOR_KEY)] = deployInfo.userAppDescriptorPath;
        deployInfoMap[QLatin1String(PACKAGE_KEY)] = deployInfo.userPackagePath;
        deployInfoMap[QLatin1String(PROFILE_KEY)] = deployInfo.proFilePath;
        deployInfoMap[QLatin1String(TARGET_KEY)] = deployInfo.targetName;
        deployInfoMap[QLatin1String(SOURCE_KEY)] = deployInfo.sourceDir;

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
        const QString targetName = innerMap.value(QLatin1String(TARGET_KEY)).toString();
        const QString sourceDir = innerMap.value(QLatin1String(SOURCE_KEY)).toString();

        BarPackageDeployInformation deployInformation(enabled, proFilePath, sourceDir, m_target->activeBuildConfiguration()->buildDirectory(), targetName);
        deployInformation.userAppDescriptorPath = appDescriptorPath;
        deployInformation.userPackagePath = packagePath;
        m_deployInformation << deployInformation;
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
    QList<Qt4ProjectManager::Qt4ProFileNode *> appNodes = project()->applicationProFiles();
    foreach (Qt4ProjectManager::Qt4ProFileNode *node, appNodes) {
        bool nodeFound = false;
        for (int i = 0; i < m_deployInformation.size(); ++i) {
            if (m_deployInformation[i].proFilePath == node->path()
                    && (!m_deployInformation[i].userAppDescriptorPath.isEmpty()
                        || !m_deployInformation[i].userPackagePath.isEmpty())) {
                BarPackageDeployInformation deployInformation = m_deployInformation[i];
                // In case the user resets the bar package path (or if it is empty already), we need the current build dir
                deployInformation.buildDir = m_target->activeBuildConfiguration()->buildDirectory();
                keep << deployInformation;
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

Qt4ProjectManager::Qt4Project *BlackBerryDeployInformation::project() const
{
    return static_cast<Qt4ProjectManager::Qt4Project *>(m_target->project());
}

void BlackBerryDeployInformation::initModel()
{
    if (!m_deployInformation.isEmpty())
        return;

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(m_target->kit());
    if (!version || !version->isValid()) {
        beginResetModel();
        m_deployInformation.clear();
        endResetModel();
        return;
    }

    const Qt4ProjectManager::Qt4ProFileNode *const rootNode = project()->rootQt4ProjectNode();
    if (!rootNode || rootNode->parseInProgress()) // Can be null right after project creation by wizard.
        return;

    disconnect(project(), SIGNAL(proFilesEvaluated()), this, SLOT(updateModel()));

    beginResetModel();
    m_deployInformation.clear();

    QList<Qt4ProjectManager::Qt4ProFileNode *> appNodes = project()->applicationProFiles();
    foreach (Qt4ProjectManager::Qt4ProFileNode *node, appNodes)
        m_deployInformation << deployInformationFromNode(node);

    endResetModel();
    connect(project(), SIGNAL(proFilesEvaluated()), this, SLOT(updateModel()));
}

BarPackageDeployInformation BlackBerryDeployInformation::deployInformationFromNode(Qt4ProjectManager::Qt4ProFileNode *node) const
{
    Qt4ProjectManager::TargetInformation ti = node->targetInformation();

    QFileInfo fi(node->path());
    const QString buildDir = m_target->activeBuildConfiguration()->buildDirectory();

    return BarPackageDeployInformation(true, node->path(), fi.absolutePath(), buildDir, ti.target);
}
