/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef OPENEDITORSVIEW_H
#define OPENEDITORSVIEW_H

#include "ui_openeditorsview.h"

#include <coreplugin/inavigationwidgetfactory.h>

#include <QStyledItemDelegate>

namespace Core {
class IEditor;

namespace Internal {

class OpenEditorsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit OpenEditorsDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;

    mutable QModelIndex pressedIndex;
};

class OpenEditorsWidget : public QWidget
{
    Q_OBJECT

public:
    OpenEditorsWidget();
    ~OpenEditorsWidget();

    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void handleClicked(const QModelIndex &);
    void handlePressed(const QModelIndex &);
    void updateCurrentItem(Core::IEditor*);
    void contextMenuRequested(QPoint pos);

private:
    void activateEditor(const QModelIndex &index);
    void closeEditor(const QModelIndex &index);

    Ui::OpenEditorsView m_ui;
    QWidget *m_widget;
    OpenEditorsDelegate *m_delegate;
};

class OpenEditorsViewFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    OpenEditorsViewFactory();
    ~OpenEditorsViewFactory();
    QString displayName() const;
    int priority() const;
    Core::Id id() const;
    QKeySequence activationSequence() const;
    Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace Core


#endif // OPENEDITORSVIEW_H
