/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPTYPEHIERARCHY_H
#define CPPTYPEHIERARCHY_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QString>
#include <QWidget>
#include <QStackedWidget>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
class QLabel;
class QModelIndex;
class QStackedLayout;
class QStandardItem;
template <class> class QVector;
template <class> class QList;
QT_END_NAMESPACE

namespace Core { class IEditor; }

namespace TextEditor { class TextEditorLinkLabel; }

namespace Utils {
class NavigationTreeView;
class AnnotatedItemDelegate;
}

namespace CppEditor {
namespace Internal {

class CppEditorWidget;
class CppClass;
class CppClassLabel;

class CppTypeHierarchyModel : public QStandardItemModel
{
    Q_OBJECT

public:
    CppTypeHierarchyModel(QObject *parent);

    Qt::DropActions supportedDragActions() const;
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
};

class CppTypeHierarchyWidget : public QWidget
{
    Q_OBJECT
public:
    CppTypeHierarchyWidget();
    virtual ~CppTypeHierarchyWidget();

public slots:
    void perform();

private:
    typedef QList<CppClass> CppClass::*HierarchyMember;
    void buildHierarchy(const CppClass &cppClass, QStandardItem *parent,
                        bool isRoot, HierarchyMember member);
    void showNoTypeHierarchyLabel();
    void showTypeHierarchy();
    void clearTypeHierarchy();
    void onItemActivated(const QModelIndex &index);

    CppEditorWidget *m_cppEditor;
    Utils::NavigationTreeView *m_treeView;
    QWidget *m_hierarchyWidget;
    QStackedLayout *m_stackLayout;
    QStandardItemModel *m_model;
    Utils::AnnotatedItemDelegate *m_delegate;
    TextEditor::TextEditorLinkLabel *m_inspectedClass;
    QLabel *m_noTypeHierarchyAvailableLabel;
};

class CppTypeHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    CppTypeHierarchyFactory();

    virtual Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPTYPEHIERARCHY_H
