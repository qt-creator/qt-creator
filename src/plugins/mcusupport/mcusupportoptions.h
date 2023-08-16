// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcukitmanager.h"
#include "mcusupport_global.h"
#include "settingshandler.h"

#include <utils/environmentfwd.h>
#include <utils/macroexpander.h>

#include <QObject>
#include <QVersionNumber>

namespace Utils {
class FilePath;
class PathChooser;
class InfoLabel;
} // namespace Utils

namespace ProjectExplorer {
class Kit;
class ToolChain;
} // namespace ProjectExplorer

namespace McuSupport {
namespace Internal {

class McuAbstractPackage;

using MacroExpanderPtr = std::shared_ptr<Utils::MacroExpander>;
using Macros = QHash<QString, Utils::MacroExpander::StringFunction>;

class McuSdkRepository final
{
public:
    Targets mcuTargets;
    Packages packages;

    static void updateQtDirMacro(const Utils::FilePath &qulDir);
    void expandVariablesAndWildcards();
    MacroExpanderPtr getMacroExpander(const McuTarget &target);

    static Macros *globalMacros();
};

class McuSupportOptions final : public QObject
{
    Q_OBJECT

public:
    explicit McuSupportOptions(const SettingsHandler::Ptr &, QObject *parent = nullptr);

    McuPackagePtr qtForMCUsSdkPackage{nullptr};
    McuSdkRepository sdkRepository;

    void setQulDir(const Utils::FilePath &dir);
    [[nodiscard]] Utils::FilePath qulDirFromSettings() const;
    [[nodiscard]] Utils::FilePath qulDocsDir() const;
    static McuKitManager::UpgradeOption askForKitUpgrades();
    static void displayKitCreationMessages(const MessagesList &messages,
                                           const SettingsHandler::Ptr &settingsHandler,
                                           McuPackagePtr qtMCUsPackage);

    void registerQchFiles() const;
    void registerExamples() const;

    static const QVersionNumber &minimalQulVersion();
    static bool isLegacyVersion(const QVersionNumber &version);

    void checkUpgradeableKits();
    void populatePackagesAndTargets();

    static bool kitsNeedQtVersion();

    [[nodiscard]] bool automaticKitCreationEnabled() const;
    void setAutomaticKitCreationEnabled(const bool enabled);

private:
    SettingsHandler::Ptr settingsHandler;

    bool m_automaticKitCreation = true;
signals:
    void packagesChanged();
};

} // namespace Internal
} // namespace McuSupport
