// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/commandline.h>
#include <utils/temporarydirectory.h>

#include <valgrind/valgrindprocess.h>
#include <valgrind/xmlprotocol/frame.h>
#include <valgrind/xmlprotocol/parser.h>
#include <valgrind/xmlprotocol/stack.h>
#include <valgrind/xmlprotocol/status.h>

#include "modeldemo.h"

#include <QApplication>
#include <QTreeView>

using namespace Valgrind;
using namespace Valgrind::XmlProtocol;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Utils::TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/QtCreator-XXXXXX");

    qRegisterMetaType<Error>();

    ValgrindProcess runner;
    runner.setValgrindCommand({VALGRIND_FAKE_PATH,
                              {"-i", PARSERTESTS_DATA_DIR "/memcheck-output-sample1.xml"}});
    ModelDemo demo;
    QObject::connect(&runner, &ValgrindProcess::processErrorReceived, &app, [](const QString &err) {
        qDebug() << err;
    });
    QObject::connect(&runner, &ValgrindProcess::done, &app, [](Tasking::DoneResult result) {
        qApp->exit(result == Tasking::DoneResult::Success ? 0 : 1);
    });
    ErrorListModel model;
    QObject::connect(&runner, &ValgrindProcess::error, &model, &ErrorListModel::addError,
                     Qt::QueuedConnection);

    QTreeView errorview;
    errorview.setSelectionMode(QAbstractItemView::SingleSelection);
    errorview.setSelectionBehavior(QAbstractItemView::SelectRows);
    errorview.setModel(&model);
    errorview.show();

    StackModel stackModel;
    demo.stackModel = &stackModel;

    QTreeView stackView;
    stackView.setModel(&stackModel);
    stackView.show();

    QObject::connect(errorview.selectionModel(), &QItemSelectionModel::selectionChanged,
                     &demo, &ModelDemo::selectionChanged);


    runner.start();

    return app.exec();
}
