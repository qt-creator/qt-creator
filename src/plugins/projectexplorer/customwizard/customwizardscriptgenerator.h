// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMap>
#include <QStringList>

namespace Core { class GeneratedFile; }

namespace ProjectExplorer {
namespace Internal {

class GeneratorScriptArgument;

// Parse the script arguments apart and expand the binary.
QStringList fixGeneratorScript(const QString &configFile, QString attributeIn);

// Step 1) Do a dry run of the generation script to get a list of files on stdout
QList<Core::GeneratedFile>
    dryRunCustomWizardGeneratorScript(const QString &targetPath,
                                      const QStringList &script,
                                      const QList<GeneratorScriptArgument> &arguments,
                                      const QMap<QString, QString> &fieldMap,
                                      QString *errorMessage);

// Step 2) Generate files
bool runCustomWizardGeneratorScript(const QString &targetPath,
                                    const QStringList &script,
                                    const QList<GeneratorScriptArgument> &arguments,
                                    const QMap<QString, QString> &fieldMap,
                                    QString *errorMessage);

} // namespace Internal
} // namespace ProjectExplorer
