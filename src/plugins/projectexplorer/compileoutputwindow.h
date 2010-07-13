/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef COMPILEOUTPUTWINDOW_H
#define COMPILEOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>

#include <QtCore/QHash>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextCharFormat;
QT_END_NAMESPACE

namespace ProjectExplorer {

class BuildManager;
class Task;

namespace Internal {

class ShowOutputTaskHandler;

class CompileOutputWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    CompileOutputWindow(BuildManager *bm);
    ~CompileOutputWindow();

    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets() const { return QList<QWidget *>(); }
    QString displayName() const { return tr("Compile Output"); }
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    void appendText(const QString &text, const QTextCharFormat &textCharFormat);
    bool canFocus();
    bool hasFocus();
    void setFocus();

    bool canNext();
    bool canPrevious();
    void goToNext();
    void goToPrev();
    bool canNavigate();

    void registerPositionOf(const Task &task);
    bool knowsPositionOf(const Task &task);
    void showPositionOf(const Task &task);

private:
    QPlainTextEdit *m_textEdit;
    QHash<unsigned int, int> m_taskPositions;
    ShowOutputTaskHandler * m_handler;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // COMPILEOUTPUTWINDOW_H
