/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "androidsdkmodel.h"
#include "androidmanager.h"
#include "androidsdkmanager.h"

#include "utils/algorithm.h"
#include "utils/qtcassert.h"
#include "utils/utilsicons.h"

#include <QIcon>

namespace Android {
namespace Internal {

const int packageColCount = 4;

AndroidSdkModel::AndroidSdkModel(const AndroidConfig &config, AndroidSdkManager *sdkManager,
                                 QObject *parent)
    : QAbstractItemModel(parent),
      m_config(config),
      m_sdkManager(sdkManager)
{
    QTC_CHECK(m_sdkManager);
    connect(m_sdkManager, &AndroidSdkManager::packageReloadBegin, [this]() {
        clearContainers();
        beginResetModel();
    });
    connect(m_sdkManager, &AndroidSdkManager::packageReloadFinished, [this]() {
        refreshData();
        endResetModel();
    });
}

QVariant AndroidSdkModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation)
    QVariant data;
    if (role == Qt::DisplayRole) {
        switch (section) {
        case packageNameColumn:
            data = tr("Package");
            break;
        case packageRevisionColumn:
            data = tr("Revision");
            break;
        case apiLevelColumn:
            data = tr("API");
            break;
        case operationColumn:
            data = tr("Operation");
            break;
        default:
            break;
        }
    }
    return data;
}

QModelIndex AndroidSdkModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // Packages under top items.
        if (parent.row() == 0) {
            // Tools packages
            if (row < m_tools.count())
                return createIndex(row, column, const_cast<AndroidSdkPackage *>(m_tools.at(row)));
        } else if (parent.row() < m_sdkPlatforms.count() + 1) {
            // Platform packages
            const SdkPlatform *sdkPlatform = m_sdkPlatforms.at(parent.row() - 1);
            SystemImageList images = sdkPlatform->systemImages(AndroidSdkPackage::AnyValidState);
            if (row < images.count() + 1) {
                if (row == 0)
                    return createIndex(row, column, const_cast<SdkPlatform *>(sdkPlatform));
                else
                    return createIndex(row, column, images.at(row - 1));
            }
        }
    } else if (row < m_sdkPlatforms.count() + 1) {
        return createIndex(row, column); // Top level items (Tools & platform)
    }

    return QModelIndex();
}

QModelIndex AndroidSdkModel::parent(const QModelIndex &index) const
{
    void *ip = index.internalPointer();
    if (!ip)
        return QModelIndex();

    auto package = static_cast<const AndroidSdkPackage *>(ip);
    if (package->type() == AndroidSdkPackage::SystemImagePackage) {
        auto image = static_cast<const SystemImage *>(package);
        int row = m_sdkPlatforms.indexOf(const_cast<SdkPlatform *>(image->platform()));
        if (row > -1)
            return createIndex(row + 1, 0);
    } else if (package->type() == AndroidSdkPackage::SdkPlatformPackage) {
        int row = m_sdkPlatforms.indexOf(static_cast<const SdkPlatform *>(package));
        if (row > -1)
            return createIndex(row + 1, 0);
    } else {
        return createIndex(0, 0); // Tools
    }

    return QModelIndex();
}

int AndroidSdkModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_sdkPlatforms.count() + 1;

    if (!parent.internalPointer()) {
        if (parent.row() == 0) // Tools
            return m_tools.count();

        if (parent.row() <= m_sdkPlatforms.count()) {
            const SdkPlatform * platform = m_sdkPlatforms.at(parent.row() - 1);
            return platform->systemImages(AndroidSdkPackage::AnyValidState).count() + 1;
        }
    }

    return 0;
}

int AndroidSdkModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return packageColCount;
}

QVariant AndroidSdkModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();


    if (!index.parent().isValid()) {
        // Top level tools
        if (index.row() == 0) {
            return role == Qt::DisplayRole && index.column() == packageNameColumn ?
                        QVariant(tr("Tools")) : QVariant();
        }
        // Top level platforms
        const SdkPlatform *platform = m_sdkPlatforms.at(index.row() - 1);
        if (role == Qt::DisplayRole) {
            if (index.column() == packageNameColumn) {
                QString androidName = AndroidManager::androidNameForApiLevel(platform->apiLevel());
                if (androidName.startsWith("Android"))
                    return androidName;
                else
                    return platform->displayText();
            } else if (index.column() == apiLevelColumn) {
                return platform->apiLevel();
            }
        }
        return QVariant();
    }

    auto p = static_cast<const AndroidSdkPackage *>(index.internalPointer());
    QString apiLevelStr;
    if (p->type() == AndroidSdkPackage::SdkPlatformPackage)
        apiLevelStr = QString::number(static_cast<const SdkPlatform *>(p)->apiLevel());

    if (p->type() == AndroidSdkPackage::SystemImagePackage)
        apiLevelStr = QString::number(static_cast<const SystemImage *>(p)->platform()->apiLevel());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case packageNameColumn:
            return p->type() == AndroidSdkPackage::SdkPlatformPackage ?
                        tr("SDK Platform") : p->displayText();
        case packageRevisionColumn:
            return p->revision().toString();
        case apiLevelColumn:
            return apiLevelStr;
        case operationColumn:
            if (p->type() == AndroidSdkPackage::SdkToolsPackage &&
                    p->state() == AndroidSdkPackage::Installed) {
                return tr("Update Only");
            } else {
                return p->state() == AndroidSdkPackage::Installed ? tr("Uninstall") : tr("Install");
            }
        default:
            break;
        }
    }

    if (role == Qt::DecorationRole && index.column() == packageNameColumn) {
        return p->state() == AndroidSdkPackage::Installed ? Utils::Icons::OK.icon() :
                                                        Utils::Icons::EMPTY16.icon();
    }

    if (role == Qt::CheckStateRole && index.column() == operationColumn )
        return m_changeState.contains(p) ? Qt::Checked : Qt::Unchecked;

    if (role == Qt::ToolTipRole)
        return QString("%1 - (%2)").arg(p->descriptionText()).arg(p->sdkStylePath());

    if (role == PackageTypeRole)
        return p->type();

    if (role == PackageStateRole)
        return p->state();

    return QVariant();
}

QHash<int, QByteArray> AndroidSdkModel::roleNames() const
{
    QHash <int, QByteArray> roles;
    roles[PackageTypeRole] = "PackageRole";
    roles[PackageStateRole] = "PackageState";
    return roles;
}

Qt::ItemFlags AndroidSdkModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags(index);
    if (index.column() == operationColumn)
        f |= Qt::ItemIsUserCheckable;

    void *ip = index.internalPointer();
    if (ip && index.column() == operationColumn) {
        auto package = static_cast<const AndroidSdkPackage *>(ip);
        if (package->state() == AndroidSdkPackage::Installed &&
                package->type() == AndroidSdkPackage::SdkToolsPackage) {
            f &= ~Qt::ItemIsEnabled;
        }
    }
    return f;
}

bool AndroidSdkModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    void *ip = index.internalPointer();
    if (ip && role == Qt::CheckStateRole) {
        auto package = static_cast<const AndroidSdkPackage *>(ip);
        if (value.toInt() == Qt::Checked) {
            m_changeState << package;
            emit dataChanged(index, index, {Qt::CheckStateRole});
        } else if (m_changeState.remove(package)) {
            emit dataChanged(index, index, {Qt::CheckStateRole});
        }
        return true;
    }
    return false;
}

void AndroidSdkModel::selectMissingEssentials()
{
    resetSelection();
    bool selectPlatformTool = !m_config.adbToolPath().exists();
    bool selectBuildTools = m_config.buildToolsVersion().isNull();
    auto addTool = [this](QList<const AndroidSdkPackage *>::const_iterator itr) {
        m_changeState << *itr;
        auto i = index(std::distance(m_tools.cbegin(), itr), 0, index(0, 0));
        emit dataChanged(i, i, {Qt::CheckStateRole});
    };
    for (auto tool = m_tools.cbegin(); tool != m_tools.cend(); ++tool) {
        if (selectPlatformTool && (*tool)->type() == AndroidSdkPackage::PlatformToolsPackage) {
            // Select Platform tools
            addTool(tool);
            selectPlatformTool = false;
        }
        if (selectBuildTools && (*tool)->type() == AndroidSdkPackage::BuildToolsPackage) {
            // Select build tools
            addTool(tool);
            selectBuildTools = false;
        }
        if (!selectPlatformTool && !selectBuildTools)
            break;
    }

    // Select SDK platform
    if (m_sdkManager->installedSdkPlatforms().isEmpty() && !m_sdkPlatforms.isEmpty()) {
        auto i = index(0, 0, index(1,0));
        m_changeState << m_sdkPlatforms.at(0);
        emit dataChanged(i , i, {Qt::CheckStateRole});
    }
}


QList<const AndroidSdkPackage *> AndroidSdkModel::userSelection() const
{
    return m_changeState.toList();
}

void AndroidSdkModel::resetSelection()
{
    beginResetModel();
    m_changeState.clear();
    endResetModel();
}

void AndroidSdkModel::clearContainers()
{
    m_sdkPlatforms.clear();
    m_tools.clear();
    m_changeState.clear();
}

void AndroidSdkModel::refreshData()
{
    clearContainers();
    for (AndroidSdkPackage *p : m_sdkManager->allSdkPackages()) {
        if (p->type() == AndroidSdkPackage::SdkPlatformPackage)
            m_sdkPlatforms << static_cast<SdkPlatform *>(p);
        else
            m_tools << p;
    }
    Utils::sort(m_sdkPlatforms, [](const SdkPlatform *p1, const SdkPlatform *p2) {
       return p1->apiLevel() > p2->apiLevel();
    });

    Utils::sort(m_tools, [](const AndroidSdkPackage *p1, const AndroidSdkPackage *p2) {
       if (p1->state() == p2->state()) {
           if (p1->type() ==  p2->type())
               return p1->revision() > p2->revision();
           else
               return p1->type() > p2->type();
       } else {
           return p1->state() < p2->state();
       }
    });
}

} // namespace Internal
} // namespace Android
