// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace QmlDesigner {

namespace Ui {
class PuppetDialog;
}


class PuppetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PuppetDialog(QWidget *parent = nullptr);
    ~PuppetDialog() override;

    void setDescription(const QString &description);
    void setCopyAndPasteCode(const QString &text);

    static void warning(QWidget *parent,
                   const QString &title,
                   const QString &description,
                   const QString &copyAndPasteCode);

private:
    Ui::PuppetDialog *ui;
};

} //QmlDesigner
