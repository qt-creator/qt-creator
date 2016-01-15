/****************************************************************************
**
** Copyright (C) 2016 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

namespace TextEditor {
class BaseTextEditor;
class TextEditorLinkLabel;
}

namespace Utils {
class AnnotatedItemDelegate;
class FileName;
}

namespace CppEditor {
namespace Internal {

class CppEditor;
class CppEditorWidget;
class CppInclude;
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
    void onItemActivated(const QModelIndex &index);
    void editorsClosed(QList<Core::IEditor *> editors);

private:
    void showNoIncludeHierarchyLabel();
    void showIncludeHierarchy();

    CppEditorWidget *m_cppEditor;
    CppIncludeHierarchyTreeView *m_treeView;
    CppIncludeHierarchyModel *m_model;
    Utils::AnnotatedItemDelegate *m_delegate;
    TextEditor::TextEditorLinkLabel *m_inspectedFile;
    QLabel *m_includeHierarchyInfoLabel;
    TextEditor::BaseTextEditor *m_editor;
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

    Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPINCLUDEHIERARCHY_H
