/****************************************************************************
**
** Copyright (C) 2014 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>
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

#ifndef CPPINCLUDEHIERARCHY_H
#define CPPINCLUDEHIERARCHY_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QString>
#include <QStackedWidget>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QStandardItem;
class QModelIndex;
class QLabel;
QT_END_NAMESPACE

namespace Core { class IEditor; }

namespace Utils {
class AnnotatedItemDelegate;
class FileName;
}

namespace CppEditor {
namespace Internal {

class CPPEditor;
class CPPEditorWidget;
class CppInclude;
class CppIncludeLabel;
class CppIncludeHierarchyModel;
class CppIncludeHierarchyTreeView;

class CppIncludeHierarchyWidget : public QWidget
{
    Q_OBJECT
public:
    CppIncludeHierarchyWidget();
    virtual ~CppIncludeHierarchyWidget();

public slots:
    void perform();

private slots:
    void onItemClicked(const QModelIndex &index);
    void editorsClosed(QList<Core::IEditor *> editors);

private:
    void showNoIncludeHierarchyLabel();
    void showIncludeHierarchy();

    CPPEditorWidget *m_cppEditor;
    CppIncludeHierarchyTreeView *m_treeView;
    CppIncludeHierarchyModel *m_model;
    Utils::AnnotatedItemDelegate *m_delegate;
    CppIncludeLabel *m_inspectedFile;
    QLabel *m_includeHierarchyInfoLabel;
    CPPEditor *m_editor;
};

// @todo: Pretty much the same design as the OutlineWidgetStack. Maybe we can generalize the
// outline factory so that it works for different widgets that support the same editor.
class CppIncludeHierarchyStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    CppIncludeHierarchyStackedWidget(QWidget *parent = 0);
    virtual ~CppIncludeHierarchyStackedWidget();

private:
    CppIncludeHierarchyWidget *m_typeHiearchyWidgetInstance;
};

class CppIncludeHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    CppIncludeHierarchyFactory();
    virtual ~CppIncludeHierarchyFactory();

    QString displayName() const;
    int priority() const;
    Core::Id id() const;
    QKeySequence activationSequence() const;
    Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPINCLUDEHIERARCHY_H
