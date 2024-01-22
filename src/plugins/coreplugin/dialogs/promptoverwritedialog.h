// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/filepath.h>

#include <QDialog>

#include <memory>

QT_BEGIN_NAMESPACE
class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QLabel;
QT_END_NAMESPACE

namespace Core {

namespace Internal {
class PromptOverwriteDialogPrivate;
} // namespace Internal

// Documentation inside.
class CORE_EXPORT PromptOverwriteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PromptOverwriteDialog(QWidget *parent = nullptr);
    ~PromptOverwriteDialog();

    void setFiles(const Utils::FilePaths &);

    void setFileEnabled(const Utils::FilePath &f, bool e);
    bool isFileEnabled(const Utils::FilePath &f) const;

    void setFileChecked(const Utils::FilePath &f, bool e);
    bool isFileChecked(const Utils::FilePath &f) const;

    Utils::FilePaths checkedFiles() const   { return files(Qt::Checked); }
    Utils::FilePaths uncheckedFiles() const { return files(Qt::Unchecked); }

private:
    QStandardItem *itemForFile(const Utils::FilePath &f) const;
    Utils::FilePaths files(Qt::CheckState cs) const;

    std::unique_ptr<Internal::PromptOverwriteDialogPrivate> d;
};

} // Core
