// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
class KitChooser;
} // ProjectExplorer

namespace PerfProfiler {
namespace Internal {

class PerfLoadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PerfLoadDialog(QWidget *parent = nullptr);
    ~PerfLoadDialog();

    QString traceFilePath() const;
    QString executableDirPath() const;
    ProjectExplorer::Kit *kit() const;

private:
    void on_browseTraceFileButton_pressed();
    void on_browseExecutableDirButton_pressed();

    void chooseDefaults();

    QLineEdit *m_traceFileLineEdit;
    QLineEdit *m_executableDirLineEdit;
    ProjectExplorer::KitChooser *m_kitChooser;
};

} // namespace Internal
} // namespace PerfProfiler
