// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    static const Toolchains &toolchains();
    static Toolchains toolchains(const ToolChain::Predicate &predicate);

    static ToolChain *toolChain(const ToolChain::Predicate &predicate);
    static QList<ToolChain *> findToolChains(const Abi &abi);
    static ToolChain *findToolChain(const QByteArray &id);

    static bool isLoaded();

    static bool registerToolChain(ToolChain *tc);
    static void deregisterToolChain(ToolChain *tc);

    static QList<Utils::Id> allLanguages();
    static bool registerLanguage(const Utils::Id &language, const QString &displayName);
    static QString displayNameOfLanguageId(const Utils::Id &id);
    static bool isLanguageSupported(const Utils::Id &id);

    static void aboutToShutdown();

    static ToolchainDetectionSettings detectionSettings();
    static void setDetectionSettings(const ToolchainDetectionSettings &settings);

    static void resetBadToolchains();
    static bool isBadToolchain(const Utils::FilePath &toolchain);
    static void addBadToolchain(const Utils::FilePath &toolchain);

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

    // Make sure the this is only called after all toolchain factories are registered!
    static void restoreToolChains();

    static void notifyAboutUpdate(ToolChain *);

    friend class ProjectExplorerPlugin; // for constructor
    friend class ToolChain;
};

} // namespace ProjectExplorer
