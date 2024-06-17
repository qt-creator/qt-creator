// Copyright (C) 2019 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppchecksettings.h"

#include <QDialog>

namespace Utils {
class FilePath;
using FilePaths = QList<FilePath>;
} // Utils

namespace ProjectExplorer {
class Project;
class SelectableFilesFromDirModel;
} // ProjectExplorer

namespace Cppcheck::Internal {

class ManualRunDialog : public QDialog
{
public:
    explicit ManualRunDialog(const ProjectExplorer::Project *project);

    Utils::FilePaths filePaths() const;
    QSize sizeHint() const override;

    const CppcheckSettings &manualRunSettings() const { return m_manualRunSettings; }
private:
    ProjectExplorer::SelectableFilesFromDirModel *m_model;
    CppcheckSettings m_manualRunSettings;
};

} // Cppcheck::Internal
