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
// ToolchainManager
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolchainManager : public QObject
{
    Q_OBJECT

public:
    static ToolchainManager *instance();
    ~ToolchainManager() override;

    static const Toolchains &toolchains();
    static Toolchains toolchains(const Toolchain::Predicate &predicate);

    static Toolchain *toolChain(const Toolchain::Predicate &predicate);
    static QList<Toolchain *> findToolChains(const Abi &abi);
    static Toolchain *findToolChain(const QByteArray &id);

    static bool isLoaded();

    static bool registerToolChain(Toolchain *tc);
    static void deregisterToolChain(Toolchain *tc);

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
    void toolChainAdded(ProjectExplorer::Toolchain *);
    // Tool chain is still valid when this call happens!
    void toolChainRemoved(ProjectExplorer::Toolchain *);
    // Tool chain was updated.
    void toolChainUpdated(ProjectExplorer::Toolchain *);
    // Something changed.
    void toolChainsChanged();
    //
    void toolChainsLoaded();

private:
    explicit ToolchainManager(QObject *parent = nullptr);

    // Make sure the this is only called after all toolchain factories are registered!
    static void restoreToolChains();

    static void notifyAboutUpdate(Toolchain *);

    friend class ProjectExplorerPlugin; // for constructor
    friend class Toolchain;
};

} // namespace ProjectExplorer
