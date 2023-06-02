// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef IMAGESCALING_H
#define IMAGESCALING_H

#include <QNetworkAccessManager>
#include <QtWidgets>
#include <tasking/tasktree.h>

class DownloadDialog;
class Images : public QWidget
{
Q_OBJECT
public:
    Images(QWidget *parent = nullptr);

private:
    void process();
    void initLayout(qsizetype count);

    QPushButton *cancelButton;
    QVBoxLayout *mainLayout;
    QList<QLabel *> labels;
    QGridLayout *imagesLayout;
    QStatusBar *statusBar;
    DownloadDialog *downloadDialog;

    QNetworkAccessManager qnam;
    std::unique_ptr<Tasking::TaskTree> taskTree;
};

#endif // IMAGESCALING_H
