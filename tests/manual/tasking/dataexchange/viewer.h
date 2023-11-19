// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef VIEWER_H
#define VIEWER_H

#include "recipe.h"

#include <tasking/tasktree.h>

#include <QNetworkAccessManager>
#include <QtWidgets>

class Viewer : public QWidget
{
    Q_OBJECT

public:
    Viewer(QWidget *parent = nullptr);

private:
    QLineEdit *m_lineEdit = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QListWidget *m_listWidget = nullptr;
    QStatusBar *m_statusBar = nullptr;

    QNetworkAccessManager m_nam;
    const Tasking::Storage<ExternalData> m_storage;
    const Tasking::Group m_recipe;
    std::unique_ptr<Tasking::TaskTree> m_taskTree;
};

#endif // VIEWER_H
