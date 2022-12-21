// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class AddSignalHandlerDialog; }
QT_END_NAMESPACE

class AddSignalHandlerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddSignalHandlerDialog(QWidget *parent = nullptr);
    ~AddSignalHandlerDialog() override;
    void setSignals(const QStringList &_signals);
    QString signal() const;

signals:
    void signalSelected();

private:
    void updateComboBox();
    void handleAccepted();

    Ui::AddSignalHandlerDialog *m_ui;
    QStringList m_signals;
    QString m_signal;
};
