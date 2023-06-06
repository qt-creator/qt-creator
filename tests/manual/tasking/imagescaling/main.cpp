// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "imagescaling.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc,argv);
    app.setOrganizationName("QtProject");
    app.setApplicationName(QObject::tr("Image Downloading and Scaling"));

    Images imageView;
    imageView.setWindowTitle(QObject::tr("Image Downloading and Scaling"));
    imageView.show();

    return app.exec();
}
