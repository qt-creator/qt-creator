/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/

#include "mainwindow.h"
#include "qrceditor.h"

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
    setWindowTitle(tr("Test resource editor"));
    QMenu* fMenu = menuBar()->addMenu(tr("File"));

    QAction* oa = fMenu->addAction(tr("Open..."));
    connect(oa, SIGNAL(triggered()), this, SLOT(slotOpen()));

    QAction* sa = fMenu->addAction(tr("Save"));
    connect(sa, SIGNAL(triggered()), this, SLOT(slotSave()));

    QAction* xa = fMenu->addAction(tr("Exit!"));
    connect(xa, SIGNAL(triggered()), this, SLOT(close()));


    QWidget *cw = new QWidget();
    setCentralWidget(cw);
    QVBoxLayout *lt = new QVBoxLayout(cw);
    lt->addWidget(m_qrcEditor);
    setMinimumSize(QSize(500, 500));
}

void MainWindow::slotOpen()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Choose resource file"),
                                                          QString(),
                                                          tr("Resource files (*.qrc)"));
    if (fileName.isEmpty())
        return;

    if (m_qrcEditor->load(fileName))
        statusBar()->showMessage(tr("%1 opened").arg(fileName));
    else
        statusBar()->showMessage(tr("Unable to open %1!").arg(fileName));
}

void MainWindow::slotSave()
{
    const QString oldFileName = m_qrcEditor->fileName();
    QString fileName = oldFileName;

    if (fileName.isEmpty()) {
        fileName = QFileDialog::getSaveFileName(this, tr("Save resource file"),
                                                QString(),
                                                tr("Resource files (*.qrc)"));
        if (fileName.isEmpty())
            return;
    }

    m_qrcEditor->setFileName(fileName);
    if (m_qrcEditor->save())
        statusBar()->showMessage(tr("%1 written").arg(fileName));
    else {
        statusBar()->showMessage(tr("Unable to write %1!").arg(fileName));
        m_qrcEditor->setFileName(oldFileName);
    }
}
