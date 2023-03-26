// Copyright (C) 2019 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

class OptionsWidget;
class CppcheckOptions;

class ManualRunDialog : public QDialog
{
public:
    ManualRunDialog(const CppcheckOptions &options,
                    const ProjectExplorer::Project *project);

    CppcheckOptions options() const;
    Utils::FilePaths filePaths() const;
    QSize sizeHint() const override;

private:
    OptionsWidget *m_options;
    ProjectExplorer::SelectableFilesFromDirModel *m_model;
};

} // Cppcheck::Internal
