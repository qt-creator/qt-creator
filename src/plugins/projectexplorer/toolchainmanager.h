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

#ifndef TOOLCHAINMANAGER_H
#define TOOLCHAINMANAGER_H

#include "projectexplorer_export.h"

#include "abi.h"
#include "toolchain.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>

namespace ProjectExplorer {
class ProjectExplorerPlugin;
class ToolChain;
class ToolChainFactory;

namespace Internal {
class ToolChainManagerPrivate;
}

// --------------------------------------------------------------------------
// ToolChainManager
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainManager : public QObject
{
    Q_OBJECT

public:
    static ToolChainManager *instance();
    ~ToolChainManager();

    QList<ToolChain *> toolChains() const;
    QList<ToolChain *> findToolChains(const Abi &abi) const;
    ToolChain *findToolChain(const QString &id) const;

public slots:
    void registerToolChain(ProjectExplorer::ToolChain *tc);
    void deregisterToolChain(ProjectExplorer::ToolChain *tc);

    void saveToolChains();

signals:
    void toolChainAdded(ProjectExplorer::ToolChain *);
    // ToolChain is still valid when this call happens!
    void toolChainRemoved(ProjectExplorer::ToolChain *);

private:
    explicit ToolChainManager(QObject *parent = 0);

    // Make sure the this is only called after all
    // ToolChain Factories are registered!
    void restoreToolChains();

    Internal::ToolChainManagerPrivate *const m_d;

    static ToolChainManager *m_instance;

    friend class ProjectExplorerPlugin;
};

} // namespace ProjectExplorer

#endif // TOOLCHAINMANAGER_H
