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

#ifndef QDB_MAINWINDOW_H
#define QDB_MAINWINDOW_H

#include <QtCore/QHash>

#include <QtGui/QApplication>
#include <QtGui/QMainWindow>

namespace GdbDebugger {
namespace Internal {
class DebuggerManager; 
} // namespace Internal
} // namespace GdbDebugger

class TextViewer;

QT_BEGIN_NAMESPACE
class QModelIndex;
class QPlainTextEdit;
class QSettings;
class QTabWidget;
class QTreeView;
class QStandardItemModel;
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();

    void loadFile(const QString &fileName);
    void loadFiles(const QStringList &fileNames);

private slots:
    void startDebuggerRequest();
    void startDebuggingRequest();
    void jumpToExec();
    void runToExec();
    void showStatusMessage(const QString &msg, int timeout);
    void resetLocation();
    void gotoLocation(const QString &fileName, int line, bool setMarker);
    void fileOpen();
    void quit();
    void changeCurrentFile(const QModelIndex &idx);
    void handleDataDumpersUnavailable();
    void queryCurrentTextEditor(QString *fileName, int *lineNumber,
        QObject **widget);

private:
    QSettings &settings();

    QAction *m_fileOpenAction;
    QAction *m_quitAction;
    //QAction *m_resetAction; // FIXME: Should not be needed in a stable release

    QTreeView *m_filesWindow;
    QStandardItemModel *m_filesModel;
    GdbDebugger::Internal::DebuggerManager *m_manager;

    QTabWidget *m_textViewers;
    QHash<QString, TextViewer *> m_textViewerFromName;
    QHash<TextViewer *, int> m_textBlockFromName;
    TextViewer *findOrCreateTextViewer(const QString &fileName);
    TextViewer *currentTextViewer();

    QString m_executable;
};

#endif // QDB_MAINWINDOW_H
