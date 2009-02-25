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

#ifndef CONTENTWINDOW_H
#define CONTENTWINDOW_H

#include <QtCore/QUrl>
#include <QtCore/QModelIndex>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class QHelpEngine;
class QHelpContentWidget;

class ContentWindow : public QWidget
{
    Q_OBJECT

public:
    ContentWindow(QHelpEngine *helpEngine);
    ~ContentWindow();

    bool syncToContent(const QUrl &url);
    void expandToDepth(int depth);

signals:
    void linkActivated(const QUrl &link);
    void escapePressed();

private slots:
    void showContextMenu(const QPoint &pos);
    void expandTOC();
    void itemClicked(const QModelIndex &index);

private:
    void focusInEvent(QFocusEvent *e);
    void keyPressEvent(QKeyEvent *e);
    bool eventFilter(QObject *o, QEvent *e);

    QHelpEngine *m_helpEngine;
    QHelpContentWidget *m_contentWidget;
    int m_expandDepth;
};

QT_END_NAMESPACE

#endif // CONTENTWINDOW_H
