// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/result.h>

#include <QMap>
#include <QStringList>

namespace Core { class GeneratedFile; }

namespace Utils { class FilePath; }

namespace ProjectExplorer::Internal {

class GeneratorScriptArgument;

// Parse the script arguments apart and expand the binary.
QStringList fixGeneratorScript(const QString &configFile, QString attributeIn);

// Step 1) Do a dry run of the generation script to get a list of files on stdout
Utils::Result<QList<Core::GeneratedFile>>
    dryRunCustomWizardGeneratorScript(const Utils::FilePath &targetPath,
                                      const QStringList &script,
                                      const QList<GeneratorScriptArgument> &arguments,
                                      const QMap<QString, QString> &fieldMap);

// Step 2) Generate files
Utils::Result<> runCustomWizardGeneratorScript(const Utils::FilePath &targetPath,
                                               const QStringList &script,
                                               const QList<GeneratorScriptArgument> &arguments,
                                               const QMap<QString, QString> &fieldMap);

} // namespace ProjectExplorer::Internal
