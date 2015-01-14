/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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

#include "mainwindow.h"
#include <resourceeditor/qrceditor/qrceditor.h>

#include <QAction>
#include <QDebug>
#include <QFileDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow() :
    m_qrcEditor(new  SharedTools::QrcEditor())
{
    m_qrcEditor->setResourceDragEnabled(true);
    setWindowTitle(QLatin1String("Test resource editor"));
    QMenu* fMenu = menuBar()->addMenu(QLatin1String("File"));

    QAction* oa = fMenu->addAction(QLatin1String("Open..."));
    connect(oa, SIGNAL(triggered()), this, SLOT(slotOpen()));

    QAction* sa = fMenu->addAction(QLatin1String("Save"));
    connect(sa, SIGNAL(triggered()), this, SLOT(slotSave()));

    QAction* xa = fMenu->addAction(QLatin1String("Exit!"));
    connect(xa, SIGNAL(triggered()), this, SLOT(close()));


    QWidget *cw = new QWidget();
    setCentralWidget(cw);
    QVBoxLayout *lt = new QVBoxLayout(cw);
    lt->addWidget(m_qrcEditor);
    setMinimumSize(QSize(500, 500));
}

void MainWindow::slotOpen()
{
    const QString fileName = QFileDialog::getOpenFileName(this, QLatin1String("Choose resource file"),
                                                          QString(),
                                                          QLatin1String("Resource files (*.qrc)"));
    if (fileName.isEmpty())
        return;

    if (m_qrcEditor->load(fileName))
        statusBar()->showMessage(QString::fromLatin1("%1 opened").arg(fileName));
    else
        statusBar()->showMessage(QString::fromLatin1("Unable to open %1!").arg(fileName));
}

void MainWindow::slotSave()
{
    const QString oldFileName = m_qrcEditor->fileName();
    QString fileName = oldFileName;

    if (fileName.isEmpty()) {
        fileName = QFileDialog::getSaveFileName(this, QLatin1String("Save resource file"),
                                                QString(),
                                                QLatin1String("Resource files (*.qrc)"));
        if (fileName.isEmpty())
            return;
    }

    m_qrcEditor->setFileName(fileName);
    if (m_qrcEditor->save())
        statusBar()->showMessage(QString::fromLatin1("%1 written").arg(fileName));
    else {
        statusBar()->showMessage(QString::fromLatin1("Unable to write %1!").arg(fileName));
        m_qrcEditor->setFileName(oldFileName);
    }
}
