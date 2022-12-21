// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "detaildialog.h"

#include <QByteArray>
#include <QNetworkReply>
#include <QPointer>
#include <QWidget>

namespace Ui { class MainWidget; }

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

    void setProgressbarMaximum(int maximum);
    void updateProgressBar(qint64 progressCount, qint64 fullCount);

signals:
    void restartCrashedApplication();
    void sendDump();
    void restartCrashedApplicationAndSendDump();
    void emailAdressChanged(const QString &email);
    void commentChanged(const QString &comment);

protected:
    void changeEvent(QEvent *e);

private:
    void restartApplication();
    void quitApplication();
    void showError(QNetworkReply::NetworkError error);
    void showDetails();
    void commentIsProvided();

private:
    Ui::MainWidget *ui;

    QPointer<DetailDialog> m_detailDialog;
    bool m_commentIsProvided = false;
};
