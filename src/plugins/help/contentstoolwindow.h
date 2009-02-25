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

#ifndef CONTENTSTOOLWINDOW_H
#define CONTENTSTOOLWINDOW_H

#include <coreplugin/iview.h>

#include <QtGui/QTreeWidget>

namespace Help {
namespace Internal {

class HelpEngine;
class ContentsToolWindow;

class ContentsToolWidget : public QTreeWidget
{
    Q_OBJECT
public:
    ContentsToolWidget();

signals:
    void buildRequested();
    void escapePressed();

private:
    friend class ContentsToolWindow;
    void showEvent(QShowEvent *e);
    void focusInEvent(QFocusEvent *e);
    void keyPressEvent(QKeyEvent *e);

    bool wasInitialized;
};

class ContentsToolWindow : public Core::IView
{
    Q_OBJECT

public:
    ContentsToolWindow(const QList<int> &context, HelpEngine *help);
    ~ContentsToolWindow();

    const QList<int> &context() const;
    QWidget *widget();

    QList<QWidget*> dockToolBarWidgets() const { return QList<QWidget*>(); }

    const char *uniqueViewName() const { return "Help.ContentsToolWindow"; }
    const char *globalMenuGroup() const { return "Help.Group"; }
    inline QKeySequence defaultShortcut() const { return QKeySequence(); }
    Qt::DockWidgetArea defaultArea() const { return Qt::RightDockWidgetArea; }
    IView::ViewPosition defaultPosition() const { return IView::First; }

signals:
    void showLinkRequested(const QString &link, bool newWindow);
    void escapePressed();

private slots:
    void contentsDone();
    void indexRequested();

private:
    HelpEngine *helpEngine;

    QList<int> m_context;
    ContentsToolWidget *m_widget;
};

} // namespace Internal
} // namespace Help

#endif // CONTENTSTOOLWINDOW_H
