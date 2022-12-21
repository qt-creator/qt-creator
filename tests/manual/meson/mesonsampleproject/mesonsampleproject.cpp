// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "mesonsampleproject.h"
#include "ui_mesonsampleproject.h"

MesonSampleProject::MesonSampleProject(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MesonSampleProject)
{
    ui->setupUi(this);
}

MesonSampleProject::~MesonSampleProject()
{
    delete ui;
}

