/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "projectexplorer_export.h"

#include "toolchain.h"

#include <coreplugin/id.h>

#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

#include <functional>

namespace Utils { class FileName; }

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class Abi;

// --------------------------------------------------------------------------
// ToolChainManager
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainManager : public QObject
{
    Q_OBJECT

public:
    static ToolChainManager *instance();
    ~ToolChainManager() override;

    static QList<ToolChain *> toolChains(const ToolChain::Predicate &predicate = ToolChain::Predicate());
    static ToolChain *toolChain(const ToolChain::Predicate &predicate);
    static QList<ToolChain *> findToolChains(const Abi &abi);
    static ToolChain *findToolChain(const QByteArray &id);

    static Utils::FileName defaultDebugger(const Abi &abi);

    static bool isLoaded();

    static bool registerToolChain(ToolChain *tc);
    static void deregisterToolChain(ToolChain *tc);

    static QSet<Core::Id> allLanguages();
    static bool registerLanguage(const Core::Id &language, const QString &displayName);
    static QString displayNameOfLanguageId(const Core::Id &id);
    static bool isLanguageSupported(const Core::Id &id);

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
    explicit ToolChainManager(QObject *parent = nullptr);

    // Make sure the this is only called after all
    // Tool chain Factories are registered!
    static void restoreToolChains();

    static QList<ToolChain *> readSystemFileToolChains();

    static void notifyAboutUpdate(ToolChain *);

    friend class ProjectExplorerPlugin; // for constructor
    friend class ToolChain;
};

} // namespace ProjectExplorer
