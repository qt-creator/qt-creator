// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace  QmlDesigner {

namespace Ui {
class PuppetBuildProgressDialog;
}


class PuppetBuildProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PuppetBuildProgressDialog();
    ~PuppetBuildProgressDialog() override;

    void setProgress(int progress);
    void newBuildOutput(const QByteArray &standardOutput);
    bool useFallbackPuppet() const;
    void setErrorOutputFile(const QString &filePath);
    void setErrorMessage(const QString &message);

private:
    void setUseFallbackPuppet();

private:
    Ui::PuppetBuildProgressDialog *ui;
    int m_lineCount;
    bool m_useFallbackPuppet;
};

}
