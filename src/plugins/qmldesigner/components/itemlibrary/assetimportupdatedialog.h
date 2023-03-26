// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QDialog>
#include <QStringList>
#include <QSet>

namespace QmlDesigner {
namespace Internal {

namespace Ui {
class AssetImportUpdateDialog;
}

class AssetImportUpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AssetImportUpdateDialog(const QString &importPath,
                                     const QSet<QString> &preSelectedFiles,
                                     const QSet<QString> &hiddenEntries,
                                     QWidget *parent = nullptr);
    ~AssetImportUpdateDialog();

    QStringList selectedFiles() const;

private:
    void collapseAll();
    void expandAll();

    Ui::AssetImportUpdateDialog *ui = nullptr;
};

} // namespace Internal
} // namespace QmlDesigner
