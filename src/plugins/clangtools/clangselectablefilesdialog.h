/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangfileinfo.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QPushButton;
QT_END_NAMESPACE

namespace CppTools { class ProjectInfo; }

namespace ClangTools {
namespace Internal {

namespace Ui { class SelectableFilesDialog; }
class SelectableFilesModel;

class SelectableFilesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectableFilesDialog(const CppTools::ProjectInfo &projectInfo,
                                   const FileInfoProviders &fileInfoProviders,
                                   int initialProviderIndex);
    ~SelectableFilesDialog() override;

    FileInfos fileInfos() const;
    int currentProviderIndex() const;

private:
    void onFileFilterChanged(int index);
    void accept() override;

    std::unique_ptr<Ui::SelectableFilesDialog> m_ui;
    QTreeView *m_fileView = nullptr;
    QDialogButtonBox *m_buttons = nullptr;
    std::unique_ptr<SelectableFilesModel> m_filesModel;

    FileInfoProviders m_fileInfoProviders;
    int m_previousProviderIndex = -1;

    ProjectExplorer::Project *m_project;
    QPushButton *m_analyzeButton = nullptr;
};

} // namespace Internal
} // namespace ClangTools
