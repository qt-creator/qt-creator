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
