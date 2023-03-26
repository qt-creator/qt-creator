// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangfileinfo.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace ClangTools {
namespace Internal {

class SelectableFilesModel;

class SelectableFilesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectableFilesDialog(ProjectExplorer::Project *project,
                                   const FileInfoProviders &fileInfoProviders,
                                   int initialProviderIndex);
    ~SelectableFilesDialog() override;

    FileInfos fileInfos() const;
    int currentProviderIndex() const;

private:
    void onFileFilterChanged(int index);
    void accept() override;

    QTreeView *m_fileView = nullptr;
    std::unique_ptr<SelectableFilesModel> m_filesModel;

    FileInfoProviders m_fileInfoProviders;
    int m_previousProviderIndex = -1;

    ProjectExplorer::Project *m_project;
    QComboBox *m_fileFilterComboBox;
};

} // namespace Internal
} // namespace ClangTools
