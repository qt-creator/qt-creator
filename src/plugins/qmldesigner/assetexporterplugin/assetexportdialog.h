// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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


namespace Utils {
class OutputFormatter;
}

namespace ProjectExplorer {
class Task;
}

namespace QmlDesigner {
namespace Ui { class AssetExportDialog; }
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
    QCheckBox *m_perComponentExportCheck = nullptr;
    QListView *m_filesView = nullptr;
    QPlainTextEdit *m_exportLogs = nullptr;
    Utils::OutputFormatter *m_outputFormatter = nullptr;
};

}
