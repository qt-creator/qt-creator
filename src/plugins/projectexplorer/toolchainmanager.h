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

#ifndef TOOLCHAINMANAGER_H
#define TOOLCHAINMANAGER_H

#include "projectexplorer_export.h"

#include <utils/fileutils.h>

#include <QList>
#include <QObject>
#include <QString>

namespace ProjectExplorer {
class ProjectExplorerPlugin;
class ToolChain;
class ToolChainFactory;
class Abi;

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

    Utils::FileName defaultDebugger(const Abi &abi) const;

public slots:
    bool registerToolChain(ProjectExplorer::ToolChain *tc);
    void deregisterToolChain(ProjectExplorer::ToolChain *tc);

    void saveToolChains();

signals:
    void toolChainAdded(ProjectExplorer::ToolChain *);
    // Tool chain is still valid when this call happens!
    void toolChainRemoved(ProjectExplorer::ToolChain *);
    // Tool chain was updated.
    void toolChainUpdated(ProjectExplorer::ToolChain *);
    // Something changed:
    void toolChainsChanged();

private:
    explicit ToolChainManager(QObject *parent = 0);

    // Make sure the this is only called after all
    // Tool chain Factories are registered!
    void restoreToolChains();
    QList<ToolChain *> restoreToolChains(const QString &fileName);

    void notifyAboutUpdate(ProjectExplorer::ToolChain *);

    Internal::ToolChainManagerPrivate *const d;

    static ToolChainManager *m_instance;

    friend class Internal::ToolChainManagerPrivate; // for the restoreToolChains methods
    friend class ProjectExplorerPlugin; // for constructor
    friend class ToolChain;
};

} // namespace ProjectExplorer

#endif // TOOLCHAINMANAGER_H
