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

#ifndef GITOUTPUTWINDOW_H
#define GITOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>

#include <QtGui/QAction>
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>

namespace Git {
namespace Internal {

class GitOutputWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    GitOutputWindow();
    ~GitOutputWindow();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets() const { return QList<QWidget *>(); }

    QString name() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);

    bool canFocus();
    bool hasFocus();
    void setFocus();

public slots:
    void setText(const QString &text);
    void append(const QString &text);
    void setData(const QByteArray &data);
    void appendData(const QByteArray &data);

private:
    QListWidget *m_outputListWidget;
};

} // namespace Internal
} // namespace Git

#endif // GITOUTPUTWINDOW_H
