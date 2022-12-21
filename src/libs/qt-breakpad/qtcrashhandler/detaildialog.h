// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTextBrowser;
class QDialogButtonBox;
QT_END_NAMESPACE

class DetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DetailDialog(QWidget *parent = nullptr);
    ~DetailDialog();

    void setText(const QString &text);

private:
    QTextBrowser *textBrowser = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
};
