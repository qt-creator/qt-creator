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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
