// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "viewer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("QtProject");
    app.setApplicationName("Data Exchange");

    Viewer viewer;
    viewer.show();

    return app.exec();
}
