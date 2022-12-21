// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef MESONSAMPLEPROJECT_H
#define MESONSAMPLEPROJECT_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MesonSampleProject; }
QT_END_NAMESPACE

class MesonSampleProject : public QMainWindow
{
    Q_OBJECT

public:
    MesonSampleProject(QWidget *parent = nullptr);
    ~MesonSampleProject();

private:
    Ui::MesonSampleProject *ui;
};
#endif // MESONSAMPLEPROJECT_H
