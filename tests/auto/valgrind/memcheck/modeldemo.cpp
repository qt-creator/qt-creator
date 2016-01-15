/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <valgrind/xmlprotocol/frame.h>
#include <valgrind/xmlprotocol/parser.h>
#include <valgrind/xmlprotocol/stack.h>
#include <valgrind/xmlprotocol/status.h>
#include <valgrind/xmlprotocol/threadedparser.h>

#include "modeldemo.h"

#include <QApplication>
#include <QTreeView>

using namespace Valgrind;
using namespace Valgrind::XmlProtocol;

static QString fakeValgrindExecutable()
{
    return QLatin1String(VALGRIND_FAKE_PATH);
}

static QString dataFile(const QLatin1String &file)
{
    return QLatin1String(PARSERTESTS_DATA_DIR) + QLatin1String("/") + file;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<Error>();

    ThreadedParser parser;

    Memcheck::MemcheckRunner runner;
    runner.setValgrindExecutable(fakeValgrindExecutable());
    runner.setValgrindArguments(QStringList() << QLatin1String("-i") << dataFile(QLatin1String("memcheck-output-sample1.xml")) );
    runner.setParser(&parser);

    ModelDemo demo(&runner);
    QObject::connect(&runner, &ValgrindRunner::finished,
                     &demo, &ModelDemo::finished);
    ErrorListModel model;
    QObject::connect(&parser, &ThreadedParser::error,
                     &model, &ErrorListModel::addError,
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
