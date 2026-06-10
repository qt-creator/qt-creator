// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"
#include "filepath.h"

#include <QDialog>
#include <QFileDialog>

#include <memory>

namespace Utils {

class FileDialogPrivate;

class QTCREATOR_UTILS_EXPORT FileDialog : public QDialog
{
    Q_OBJECT
public:
    enum class ViewMode { List, Icons };

    explicit FileDialog(QWidget *parent = nullptr);
    ~FileDialog() override;

    void setDirectory(const FilePath &directory);
    FilePath directory() const;

    void setNameFilters(const QStringList &filters);
    void setViewMode(ViewMode mode);
    void setMode(QFileDialog::FileMode mode);
    void setAcceptMode(QFileDialog::AcceptMode mode);

    FilePath selectedFile() const;
    FilePaths selectedFiles() const;
    QString selectedNameFilter() const;

    static FilePath getOpenFilePath(const QString &caption,
                                    const FilePath &dir = {},
                                    const QStringList &nameFilters = {},
                                    QWidget *parent = nullptr);

    static FilePaths getOpenFilePaths(const QString &caption,
                                      const FilePath &dir = {},
                                      const QStringList &nameFilters = {},
                                      QWidget *parent = nullptr);

    static FilePath getOpenDirectory(const QString &caption,
                                     const FilePath &dir = {},
                                     QWidget *parent = nullptr);

    static FilePath getSaveFilePath(const QString &caption,
                                    const FilePath &dir = {},
                                    const QStringList &nameFilters = {},
                                    QWidget *parent = nullptr);
signals:
    void directoryChanged(const FilePath &directory);

private:
    std::unique_ptr<FileDialogPrivate> d;
};

} // namespace Utils
