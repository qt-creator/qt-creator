// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DOWNLOADDIALOG_H
#define DOWNLOADDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class DownloadDialog;
}
QT_END_NAMESPACE

class DownloadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadDialog(QWidget *parent = nullptr);
    ~DownloadDialog();

    QList<QUrl> getUrls() const;

private:
    Ui::DownloadDialog *ui;
};

#endif // DOWNLOADDIALOG_H
