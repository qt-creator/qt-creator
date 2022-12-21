// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace Utils { class FilePath; }

namespace Core {
namespace Internal {

// Present the user with a file name and a list of available
// editor kinds to choose from.
class OpenWithDialog : public QDialog
{
    Q_OBJECT

public:
    OpenWithDialog(const Utils::FilePath &filePath, QWidget *parent);

    void setEditors(const QStringList &);
    int editor() const;

    void setCurrentEditor(int index);

private:
    void currentItemChanged(QListWidgetItem *, QListWidgetItem *);
    void setOkButtonEnabled(bool);

    QListWidget *editorListWidget;
    QDialogButtonBox *buttonBox;
};

} // namespace Internal
} // namespace Core
