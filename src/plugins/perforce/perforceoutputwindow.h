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

#ifndef PERFORCEOUTPUTWINDOW_H
#define PERFORCEOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>

#include <QtGui/QAction>
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>

namespace Perforce {
namespace Internal {

class PerforcePlugin;

class PerforceOutputWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    PerforceOutputWindow(PerforcePlugin *p4Plugin);
    ~PerforceOutputWindow();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets(void) const { return QList<QWidget *>(); }

    QString name() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);

    bool canFocus();
    bool hasFocus();
    void setFocus();

public slots:
     void append(const QString &txt, bool doPopup = false);

private slots:;
    void diff();
    void openFiles();

private:
    void contextMenuEvent(QContextMenuEvent *event);

    static QString getFileName(const QListWidgetItem *item);

    PerforcePlugin *m_p4Plugin;
    QListWidget *m_outputListWidget;
    QAction *m_diffAction;
};

} // namespace Perforce
} // namespace Internal

#endif // PERFORCEOUTPUTWINDOW_H
