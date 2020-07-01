/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include "assetexporter.h"

#include <QDialog>
#include <QStringListModel>

#include "utils/fileutils.h"

#include <memory>

QT_BEGIN_NAMESPACE
class QPushButton;
class QCheckBox;
class QListView;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Ui {
class AssetExportDialog;
}

namespace Utils {
class OutputFormatter;
}

namespace ProjectExplorer {
class Task;
}

namespace QmlDesigner {
class FilePathModel;

class AssetExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AssetExportDialog(const Utils::FilePath &exportPath, AssetExporter &assetExporter,
                               FilePathModel& model, QWidget *parent = nullptr);
    ~AssetExportDialog();

private:
    void onExport();
    void onExportStateChanged(AssetExporter::ParsingState newState);
    void updateExportProgress(double value);
    void switchView(bool showExportView);
    void onTaskAdded(const ProjectExplorer::Task &task);

private:
    AssetExporter &m_assetExporter;
    FilePathModel &m_filePathModel;
    std::unique_ptr<Ui::AssetExportDialog> m_ui;
    QPushButton *m_exportBtn = nullptr;
    QCheckBox *m_exportAssetsCheck = nullptr;
    QListView *m_filesView = nullptr;
    QPlainTextEdit *m_exportLogs = nullptr;
    Utils::OutputFormatter *m_outputFormatter = nullptr;
};

}
