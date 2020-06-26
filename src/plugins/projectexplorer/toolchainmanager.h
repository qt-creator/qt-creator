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

#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

#include <functional>

namespace Utils { class FilePath; }

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class Abi;

class ToolchainDetectionSettings
{
public:
    bool detectX64AsX32 = false;
};

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

    static bool isLoaded();

    static bool registerToolChain(ToolChain *tc);
    static void deregisterToolChain(ToolChain *tc);

    static QSet<Utils::Id> allLanguages();
    static bool registerLanguage(const Utils::Id &language, const QString &displayName);
    static QString displayNameOfLanguageId(const Utils::Id &id);
    static bool isLanguageSupported(const Utils::Id &id);

    static void aboutToShutdown();

    static ToolchainDetectionSettings detectionSettings();
    static void setDetectionSettings(const ToolchainDetectionSettings &settings);

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
