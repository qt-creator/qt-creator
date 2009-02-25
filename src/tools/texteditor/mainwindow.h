/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef TEXTEDITOR_MAINWINDOW_H
#define TEXTEDITOR_MAINWINDOW_H

#include "itexteditor.h"

#include <QtCore/QHash>

#include <QtGui/QApplication>
#include <QtGui/QMainWindow>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QSettings;
QT_END_NAMESPACE

namespace TextEditor { class BaseTextEditor; }
typedef TextEditor::BaseTextEditor Editor;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();

    void loadFile(const QString &fileName);
    void loadFiles(const QStringList &fileNames);

private slots:
    void fileOpen();

private:
    QTabWidget *m_textViewers;
    QHash<QString, Editor *> m_textViewerFromName;
    Editor *findOrCreateTextViewer(const QString &fileName);
    Editor *currentTextViewer();
    
    QSettings &settings();
    QAction * m_fileOpenAction;
    QAction * m_quitAction;
};

#endif // TEXTEDITOR_MAINWINDOW_H
