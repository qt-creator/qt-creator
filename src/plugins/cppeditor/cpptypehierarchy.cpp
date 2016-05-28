/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cpptypehierarchy.h"

#include "cppeditorconstants.h"
#include "cppeditor.h"
#include "cppelementevaluator.h"
#include "cppeditorplugin.h"

#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>
#include <utils/annotateditemdelegate.h>
#include <utils/navigationtreeview.h>
#include <utils/dropsupport.h>

#include <QApplication>
#include <QLabel>
#include <QLatin1String>
#include <QModelIndex>
#include <QStackedLayout>
#include <QVBoxLayout>

using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace Utils;

namespace {

enum ItemRole {
    AnnotationRole = Qt::UserRole + 1,
    LinkRole
};

QStandardItem *itemForClass(const CppClass &cppClass)
{
    QStandardItem *item = new QStandardItem;
    item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
    item->setData(cppClass.name, Qt::DisplayRole);
    if (cppClass.name != cppClass.qualifiedName)
        item->setData(cppClass.qualifiedName, AnnotationRole);
    item->setData(cppClass.icon, Qt::DecorationRole);
    QVariant link;
    link.setValue(CppEditorWidget::Link(cppClass.link));
    item->setData(link, LinkRole);
    return item;
}

QList<CppClass> sortClasses(const QList<CppClass> &cppClasses)
{
    QList<CppClass> sorted = cppClasses;
    Utils::sort(sorted, [](const CppClass &c1, const CppClass &c2) -> bool {
        const QString key1 = c1.name + QLatin1String("::") + c1.qualifiedName;
        const QString key2 = c2.name + QLatin1String("::") + c2.qualifiedName;
        return key1 < key2;
    });
    return sorted;
}

} // Anonymous

namespace CppEditor {
namespace Internal {

// CppTypeHierarchyWidget
CppTypeHierarchyWidget::CppTypeHierarchyWidget() :
    QWidget(0),
    m_treeView(0),
    m_model(0),
    m_delegate(0),
    m_noTypeHierarchyAvailableLabel(0)
{
    m_inspectedClass = new TextEditor::TextEditorLinkLabel(this);
    m_inspectedClass->setMargin(5);
    m_model = new CppTypeHierarchyModel(this);
    m_treeView = new NavigationTreeView(this);
    m_treeView->setActivationMode(SingleClickActivation);
    m_delegate = new AnnotatedItemDelegate(this);
    m_delegate->setDelimiter(QLatin1String(" "));
    m_delegate->setAnnotationRole(AnnotationRole);
    m_treeView->setModel(m_model);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setItemDelegate(m_delegate);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setDragEnabled(true);
    m_treeView->setDragDropMode(QAbstractItemView::DragOnly);
    m_treeView->setDefaultDropAction(Qt::MoveAction);
    connect(m_treeView, &QTreeView::activated, this, &CppTypeHierarchyWidget::onItemActivated);

    m_noTypeHierarchyAvailableLabel = new QLabel(tr("No type hierarchy available"), this);
    m_noTypeHierarchyAvailableLabel->setAlignment(Qt::AlignCenter);
    m_noTypeHierarchyAvailableLabel->setAutoFillBackground(true);
    m_noTypeHierarchyAvailableLabel->setBackgroundRole(QPalette::Base);

    m_hierarchyWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_inspectedClass);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_treeView));
    m_hierarchyWidget->setLayout(layout);

    m_stackLayout = new QStackedLayout;
    m_stackLayout->addWidget(m_hierarchyWidget);
    m_stackLayout->addWidget(m_noTypeHierarchyAvailableLabel);
    m_stackLayout->setCurrentWidget(m_noTypeHierarchyAvailableLabel);
    setLayout(m_stackLayout);

    connect(CppEditorPlugin::instance(), &CppEditorPlugin::typeHierarchyRequested, this, &CppTypeHierarchyWidget::perform);
}

CppTypeHierarchyWidget::~CppTypeHierarchyWidget()
{}

void CppTypeHierarchyWidget::perform()
{
    showNoTypeHierarchyLabel();

    CppEditor *editor = qobject_cast<CppEditor *>(Core::EditorManager::currentEditor());
    if (!editor)
        return;

    CppEditorWidget *widget = qobject_cast<CppEditorWidget *>(editor->widget());
    if (!widget)
        return;

    clearTypeHierarchy();

    CppElementEvaluator evaluator(widget);
    evaluator.setLookupBaseClasses(true);
    evaluator.setLookupDerivedClasses(true);
    evaluator.execute();
    if (evaluator.identifiedCppElement()) {
        const QSharedPointer<CppElement> &cppElement = evaluator.cppElement();
        CppElement *element = cppElement.data();
        if (CppClass *cppClass = dynamic_cast<CppClass *>(element)) {
            m_inspectedClass->setText(cppClass->name);
            m_inspectedClass->setLink(cppClass->link);
            QStandardItem *bases = new QStandardItem(tr("Bases"));
            m_model->invisibleRootItem()->appendRow(bases);
            buildHierarchy(*cppClass, bases, true, &CppClass::bases);
            QStandardItem *derived = new QStandardItem(tr("Derived"));
            m_model->invisibleRootItem()->appendRow(derived);
            buildHierarchy(*cppClass, derived, true, &CppClass::derived);
            m_treeView->expandAll();

            showTypeHierarchy();
        }
    }
}

void CppTypeHierarchyWidget::buildHierarchy(const CppClass &cppClass, QStandardItem *parent,
                                            bool isRoot, const HierarchyMember member)
{
    if (!isRoot) {
        QStandardItem *item = itemForClass(cppClass);
        parent->appendRow(item);
        parent = item;
    }
    foreach (const CppClass &klass, sortClasses(cppClass.*member))
        buildHierarchy(klass, parent, false, member);
}

void CppTypeHierarchyWidget::showNoTypeHierarchyLabel()
{
    m_stackLayout->setCurrentWidget(m_noTypeHierarchyAvailableLabel);
}

void CppTypeHierarchyWidget::showTypeHierarchy()
{
    m_stackLayout->setCurrentWidget(m_hierarchyWidget);
}

void CppTypeHierarchyWidget::clearTypeHierarchy()
{
    m_inspectedClass->clear();
    m_model->clear();
}

void CppTypeHierarchyWidget::onItemActivated(const QModelIndex &index)
{
    auto link = index.data(LinkRole).value<TextEditor::TextEditorWidget::Link>();
    if (link.hasValidTarget())
        Core::EditorManager::openEditorAt(link.targetFileName,
                                          link.targetLine,
                                          link.targetColumn,
                                          Constants::CPPEDITOR_ID);
}

// CppTypeHierarchyFactory
CppTypeHierarchyFactory::CppTypeHierarchyFactory()
{
    setDisplayName(tr("Type Hierarchy"));
    setPriority(700);
    setId(Constants::TYPE_HIERARCHY_ID);
}

Core::NavigationView CppTypeHierarchyFactory::createWidget()
{
    auto w = new CppTypeHierarchyWidget;
    w->perform();
    return Core::NavigationView(w);
}

CppTypeHierarchyModel::CppTypeHierarchyModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

Qt::DropActions CppTypeHierarchyModel::supportedDragActions() const
{
    // copy & move actions to avoid idiotic behavior of drag and drop:
    // standard item model removes nodes automatically that are
    // dropped anywhere with move action, but we do not want the '+' sign in the
    // drag handle that would appear when only allowing copy action
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList CppTypeHierarchyModel::mimeTypes() const
{
    return DropSupport::mimeTypesForFilePaths();
}

QMimeData *CppTypeHierarchyModel::mimeData(const QModelIndexList &indexes) const
{
    auto data = new DropMimeData;
    data->setOverrideFileDropAction(Qt::CopyAction); // do not remove the item from the model
    foreach (const QModelIndex &index, indexes) {
        auto link = index.data(LinkRole).value<TextEditor::TextEditorWidget::Link>();
        if (link.hasValidTarget())
            data->addFile(link.targetFileName, link.targetLine, link.targetColumn);
    }
    return data;
}

} // namespace Internal
} // namespace CppEditor

