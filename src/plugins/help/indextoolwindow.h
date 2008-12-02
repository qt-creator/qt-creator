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
#ifndef INDEXTOOLWINDOW_H
#define INDEXTOOLWINDOW_H

#include <QtCore/QModelIndex>
#include <QtGui/QWidget>

#include <coreplugin/iview.h>

class QListView;
class QLineEdit;

namespace Help {
namespace Internal {

class HelpEngine;
class IndexListModel;
class IndexToolWindow;

class IndexToolWidget : public QWidget
{
    Q_OBJECT
public:
    IndexToolWidget();

signals:
    void buildRequested();
    void escapePressed();

private:
    friend class IndexToolWindow;

    bool eventFilter(QObject * o, QEvent * e);
    void showEvent(QShowEvent *e);
    void focusInEvent(QFocusEvent *e);

    bool wasInitialized;
    QLineEdit *findLineEdit;
    QListView *indicesView;
};


class IndexToolWindow : public Core::IView
{
    Q_OBJECT

public:
    IndexToolWindow(const QList<int> &context, HelpEngine *help);
    ~IndexToolWindow();

    const QList<int> &context() const;
    QWidget *widget();

    QList<QWidget*> dockToolBarWidgets() const { return QList<QWidget*>(); }

    const char *uniqueViewName() const { return "Help.IndexToolWindow"; }
    const char *globalMenuGroup() const { return "Help.Group"; }
    inline QKeySequence defaultShortcut() const { return QKeySequence(); }
    Qt::DockWidgetArea defaultArea() const { return Qt::RightDockWidgetArea; }
    IView::ViewPosition defaultPosition() const { return IView::Second; }

signals:
    void showLinkRequested(const QString &link, bool newWindow);
    void escapePressed();

private slots:
    void indexDone();
    void searchInIndex(const QString &str);
    void indexRequested();

private:
    HelpEngine *helpEngine;
    IndexListModel *model;

    QList<int> m_context;
    IndexToolWidget *m_widget;
};

} // namespace Internal
} // namespace Help

#endif // INDEXTOOLWINDOW_H
