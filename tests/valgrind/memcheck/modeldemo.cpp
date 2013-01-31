/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
    return QCoreApplication::applicationDirPath() + QLatin1String("/../../valgrind-fake/valgrind-fake");
}

static QString dataFile(const QLatin1String &file)
{
    return QLatin1String(PARSERTESTS_DATA_DIR) + QLatin1String("/") + file;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<Valgrind::XmlProtocol::Error>();

    ThreadedParser parser;

    Valgrind::Memcheck::MemcheckRunner runner;
    runner.setValgrindExecutable(fakeValgrindExecutable());
    runner.setValgrindArguments(QStringList() << QLatin1String("-i") << dataFile(QLatin1String("memcheck-output-sample1.xml")) );
    runner.setParser(&parser);

    ModelDemo demo(&runner);
    runner.connect(&runner, SIGNAL(finished()), &demo, SLOT(finished()));
    ErrorListModel model;
    parser.connect(&parser, SIGNAL(error(Valgrind::XmlProtocol::Error)),
                   &model, SLOT(addError(Valgrind::XmlProtocol::Error)),
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

    errorview.connect(errorview.selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                      &demo, SLOT(selectionChanged(QItemSelection,QItemSelection)));


    runner.start();

    return app.exec();
}
