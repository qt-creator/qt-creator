// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helloworldwindow.h"

#include "helloworldtr.h"

#include <QTextEdit>
#include <QVBoxLayout>

using namespace HelloWorld::Internal;

HelloWorldWindow::HelloWorldWindow(QWidget *parent)
   : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QTextEdit(Tr::tr("Focus me to activate my context!")));
    setWindowTitle(Tr::tr("Hello, world!"));
}
