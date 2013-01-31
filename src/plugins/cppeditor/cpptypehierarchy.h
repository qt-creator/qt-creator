/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPTYPEHIERARCHY_H
#define CPPTYPEHIERARCHY_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QString>
#include <QWidget>
#include <QStackedWidget>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QStandardItem;
class QModelIndex;
class QLabel;
template <class> class QVector;
template <class> class QList;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace Utils {
class NavigationTreeView;
class AnnotatedItemDelegate;
}

namespace CppEditor {
namespace Internal {

class CPPEditorWidget;
class CppClass;
class CppClassLabel;

class CppTypeHierarchyWidget : public QWidget
{
    Q_OBJECT
public:
    CppTypeHierarchyWidget(Core::IEditor *editor);
    virtual ~CppTypeHierarchyWidget();

public slots:
    void perform();

private slots:
    void onItemClicked(const QModelIndex &index);

private:
    typedef QList<CppClass> CppClass::*HierarchyMember;
    void buildHierarchy(const CppClass &cppClass, QStandardItem *parent, bool isRoot, HierarchyMember member);

    CPPEditorWidget *m_cppEditor;
    Utils::NavigationTreeView *m_treeView;
    QStandardItemModel *m_model;
    Utils::AnnotatedItemDelegate *m_delegate;
    CppClassLabel *m_inspectedClass;
};

// @todo: Pretty much the same design as the OutlineWidgetStack. Maybe we can generalize the
// outline factory so that it works for different widgets that support the same editor.
class CppTypeHierarchyStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    CppTypeHierarchyStackedWidget(QWidget *parent = 0);
    virtual ~CppTypeHierarchyStackedWidget();

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
    virtual int priority() const;
    virtual Core::Id id() const;
    virtual QKeySequence activationSequence() const;
    virtual Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPTYPEHIERARCHY_H
