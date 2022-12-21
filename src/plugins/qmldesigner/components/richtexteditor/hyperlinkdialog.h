// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace QmlDesigner {

namespace Ui {
class HyperlinkDialog;
}

class HyperlinkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HyperlinkDialog(QWidget *parent = nullptr);
    ~HyperlinkDialog();

    QString getLink() const;
    void setLink(const QString &link);

    QString getAnchor() const;
    void setAnchor(const QString &anchor);

private:
    Ui::HyperlinkDialog *ui;
};

}
