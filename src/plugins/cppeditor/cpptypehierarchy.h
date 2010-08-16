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

#ifndef CPPTYPEHIERARCHY_H
#define CPPTYPEHIERARCHY_H

#include "cppelementevaluator.h"

#include <coreplugin/inavigationwidgetfactory.h>

#include <QtCore/QString>
#include <QtGui/QWidget>
#include <QtGui/QStackedWidget>
#include <QtGui/QStandardItem>
#include <QtGui/QStyledItemDelegate>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QModelIndex;
class QPainter;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace Utils {
class NavigationTreeView;
}

namespace CppEditor {
namespace Internal {

class CPPEditor;

class CppTypeHierarchyItem : public QStandardItem
{
public:
    CppTypeHierarchyItem(const CppClass &cppClass);
    virtual ~CppTypeHierarchyItem();

    virtual int type() const;

    const CppClass &cppClass() const;

private:
    CppClass m_cppClass;
};

class CppTypeHierarchyDelegate : public QStyledItemDelegate
{
public:
    CppTypeHierarchyDelegate(QObject *parent = 0);
    virtual ~CppTypeHierarchyDelegate();

    virtual void paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class CppTypeHierarchyWidget : public QWidget
{
    Q_OBJECT
public:
    CppTypeHierarchyWidget(Core::IEditor *editor);
    virtual ~CppTypeHierarchyWidget();

    bool handleReplacement(Core::IEditor *editor);

public slots:
    void perform();

private slots:
    void onItemClicked(const QModelIndex &index);

private:
    void buildModel(const CppClass &cppClass, QStandardItem *item);

    CPPEditor *m_cppEditor;
    Utils::NavigationTreeView *m_treeView;
    QStandardItemModel *m_model;
    CppTypeHierarchyDelegate *m_delegate;
};

// @todo: Pretty much the same design as the OutlineWidgetStack. Maybe we can generalize the
// outline factory so that it works for different widgets that support the same editor.
class CppTypeHierarchyStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    CppTypeHierarchyStackedWidget(QWidget *parent = 0);
    virtual ~CppTypeHierarchyStackedWidget();

public slots:
    void editorChanged(Core::IEditor* editor);

private:
    CppTypeHierarchyWidget *m_typeHiearchyWidgetInstance;
};

class CppTypeHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    CppTypeHierarchyFactory();
    virtual ~CppTypeHierarchyFactory();

    virtual QString displayName() const;
    virtual QString id() const;
    virtual QKeySequence activationSequence() const;
    virtual Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPTYPEHIERARCHY_H
