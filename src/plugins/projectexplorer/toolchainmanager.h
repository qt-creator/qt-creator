/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TOOLCHAINMANAGER_H
#define TOOLCHAINMANAGER_H

#include "projectexplorer_export.h"

#include <QList>
#include <QObject>
#include <QString>

namespace Utils { class FileName; }

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class ToolChain;
class Abi;

// --------------------------------------------------------------------------
// ToolChainManager
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainManager : public QObject
{
    Q_OBJECT

public:
    static ToolChainManager *instance();
    ~ToolChainManager();

    static QList<ToolChain *> toolChains();
    static QList<ToolChain *> findToolChains(const Abi &abi);
    static ToolChain *findToolChain(const QByteArray &id);

    static Utils::FileName defaultDebugger(const Abi &abi);

    static bool isLoaded();

    static bool registerToolChain(ToolChain *tc);
    static void deregisterToolChain(ToolChain *tc);

public slots:
    void saveToolChains();

signals:
    void toolChainAdded(ProjectExplorer::ToolChain *);
    // Tool chain is still valid when this call happens!
    void toolChainRemoved(ProjectExplorer::ToolChain *);
    // Tool chain was updated.
    void toolChainUpdated(ProjectExplorer::ToolChain *);
    // Something changed.
    void toolChainsChanged();
    //
    void toolChainsLoaded();

private:
    explicit ToolChainManager(QObject *parent = 0);

    // Make sure the this is only called after all
    // Tool chain Factories are registered!
    static void restoreToolChains();
    static void notifyAboutUpdate(ToolChain *);

    friend class ProjectExplorerPlugin; // for constructor
    friend class ToolChain;
};

} // namespace ProjectExplorer

#endif // TOOLCHAINMANAGER_H
