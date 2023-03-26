// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "mesonsampleproject.h"

#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator translator;
    translator.load("mesonsampleproject_" + QLocale::system().name());
    a.installTranslator(&translator);
    MesonSampleProject w;
    w.show();
    return a.exec();
}
