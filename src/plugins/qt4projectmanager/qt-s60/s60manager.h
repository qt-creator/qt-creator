/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef S60MANAGER_H
#define S60MANAGER_H

#include "s60devices.h"

#include <projectexplorer/toolchaintype.h>

#include <QtCore/QObject>

namespace ProjectExplorer {
class ToolChain;
}

namespace Qt4ProjectManager {
class QtVersion;
namespace Internal {
class S60Devices;

class S60Manager : public QObject
{
    Q_OBJECT
public:
    S60Manager(QObject *parent = 0);
    ~S60Manager();
    static S60Manager *instance();

    ProjectExplorer::ToolChain *createWINSCWToolChain(const Qt4ProjectManager::QtVersion *version) const;
    ProjectExplorer::ToolChain *createGCCEToolChain(const Qt4ProjectManager::QtVersion *version) const;
    ProjectExplorer::ToolChain *createGCCE_GnuPocToolChain(const Qt4ProjectManager::QtVersion *version) const;
    ProjectExplorer::ToolChain *createRVCTToolChain(const Qt4ProjectManager::QtVersion *version,
                                                    ProjectExplorer::ToolChainType type) const;

    S60Devices *devices() const { return m_devices; }
    S60Devices::Device deviceForQtVersion(const Qt4ProjectManager::QtVersion *version) const;
    QString deviceIdFromDetectionSource(const QString &autoDetectionSource) const;

    static bool hasRvct2Compiler();
    static bool hasRvct4Compiler();

private slots:
    void updateQtVersions();

private:
    void addAutoReleasedObject(QObject *p);

    static S60Manager *m_instance;

    S60Devices *m_devices;
    QObjectList m_pluginObjects;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60MANAGER_H
