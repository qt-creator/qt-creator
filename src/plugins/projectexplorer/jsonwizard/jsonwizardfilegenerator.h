// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonwizardgeneratorfactory.h"

#include <QVariant>

namespace ProjectExplorer {
namespace Internal {

// Documentation inside.
class JsonWizardFileGenerator : public JsonWizardGenerator
{
public:
    bool setup(const QVariant &data, QString *errorMessage);

    Core::GeneratedFiles fileList(Utils::MacroExpander *expander,
                                  const Utils::FilePath &wizardDir, const Utils::FilePath &projectDir,
                                  QString *errorMessage) override;

    bool writeFile(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage) override;

private:
    class File {
    public:
        bool keepExisting = false;
        Utils::FilePath source;
        Utils::FilePath target;
        QVariant condition = true;
        QVariant isBinary = false;
        QVariant overwrite = false;
        QVariant openInEditor = false;
        QVariant openAsProject = false;
        QVariant isTemporary = false;

        QList<JsonWizard::OptionDefinition> options;
    };

    Core::GeneratedFile generateFile(const File &file, Utils::MacroExpander *expander,
                                     QString *errorMessage);

    QList<File> m_fileList;
    friend QDebug &operator<<(QDebug &debug, const File &file);
};

inline QDebug &operator<<(QDebug &debug, const JsonWizardFileGenerator::File &file)
{
    debug << "WizardFile{"
          << "source:" << file.source
          << "; target:" << file.target
          << "; condition:" << file.condition
          << "; options:" << file.options
          << "}";
    return debug;
}

} // namespace Internal
} // namespace ProjectExplorer
