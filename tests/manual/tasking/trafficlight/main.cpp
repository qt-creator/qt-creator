// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "glueinterface.h"
#include "recipe.h"
#include "trafficlight.h"

#include <tasking/tasktree.h>

#include <QtWidgets/qapplication.h>

using namespace Tasking;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    GlueInterface iface;
    TaskTree taskTree(recipe(&iface));
    TrafficLight widget(&iface);

    widget.show();
    taskTree.start();

    return app.exec();
}
