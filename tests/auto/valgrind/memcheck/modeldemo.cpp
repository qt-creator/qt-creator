/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
