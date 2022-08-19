// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace QmlDesigner {

namespace Ui {
class OpenUiQmlFileDialog;
}

class OpenUiQmlFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenUiQmlFileDialog(QWidget *parent = nullptr);
    ~OpenUiQmlFileDialog() override;
    bool uiFileOpened() const;
    void setUiQmlFiles(const QString &projectPath, const QStringList &stringList);
    QString uiQmlFile() const;

private:
    Ui::OpenUiQmlFileDialog *ui;
    bool m_uiFileOpened = false;
    QString m_uiQmlFile;
};


} // namespace QmlDesigner
