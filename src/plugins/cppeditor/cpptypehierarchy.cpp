/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cpptypehierarchy.h"

#include "cppeditorconstants.h"
#include "cppeditor.h"
#include "cppelementevaluator.h"
#include "cppeditorplugin.h"

#include <coreplugin/find/treeviewfind.h>
#include <utils/algorithm.h>
#include <utils/navigationtreeview.h>
#include <utils/annotateditemdelegate.h>

#include <QLabel>
#include <QLatin1String>
#include <QModelIndex>
#include <QStackedLayout>
#include <QStandardItemModel>
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
    item->setData(cppClass.name, Qt::DisplayRole);
    if (cppClass.name != cppClass.qualifiedName)
        item->setData(cppClass.qualifiedName, AnnotationRole);
    item->setData(cppClass.icon, Qt::DecorationRole);
    QVariant link;
    link.setValue(CPPEditorWidget::Link(cppClass.link));
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

class CppClassLabel : public QLabel
{
public:
    CppClassLabel(QWidget *parent)
        : QLabel(parent)
    {}

    void setup(CppClass *cppClass)
    {
        setText(cppClass->name);
        m_link = cppClass->link;
    }

    void clear()
    {
        QLabel::clear();
        m_link = CPPEditorWidget::Link();
    }

private:
    void mousePressEvent(QMouseEvent *)
    {
        if (!m_link.hasValidTarget())
            return;

        Core::EditorManager::openEditorAt(m_link.targetFileName,
                                          m_link.targetLine,
                                          m_link.targetColumn,
                                          Constants::CPPEDITOR_ID);
    }

    CPPEditorWidget::Link m_link;
};

// CppTypeHierarchyWidget
CppTypeHierarchyWidget::CppTypeHierarchyWidget() :
    QWidget(0),
    m_treeView(0),
    m_model(0),
    m_delegate(0),
    m_noTypeHierarchyAvailableLabel(0)
{
    m_inspectedClass = new CppClassLabel(this);
    m_inspectedClass->setMargin(5);
    m_model = new QStandardItemModel(this);
    m_treeView = new NavigationTreeView(this);
    m_delegate = new AnnotatedItemDelegate(this);
    m_delegate->setDelimiter(QLatin1String(" "));
    m_delegate->setAnnotationRole(AnnotationRole);
    m_treeView->setModel(m_model);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setItemDelegate(m_delegate);
    m_treeView->setRootIsDecorated(false);
    connect(m_treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(onItemClicked(QModelIndex)));

    m_noTypeHierarchyAvailableLabel = new QLabel(tr("No type hierarchy available"), this);
    m_noTypeHierarchyAvailableLabel->setAlignment(Qt::AlignCenter);
    m_noTypeHierarchyAvailableLabel->setAutoFillBackground(true);
    m_noTypeHierarchyAvailableLabel->setBackgroundRole(QPalette::Base);

    m_hierarchyWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_inspectedClass);
    layout->addWidget(Core::TreeViewFind::createSearchableWrapper(m_treeView));
    m_hierarchyWidget->setLayout(layout);

    m_stackLayout = new QStackedLayout;
    m_stackLayout->addWidget(m_hierarchyWidget);
    m_stackLayout->addWidget(m_noTypeHierarchyAvailableLabel);
    m_stackLayout->setCurrentWidget(m_noTypeHierarchyAvailableLabel);
    setLayout(m_stackLayout);

    connect(CppEditorPlugin::instance(), SIGNAL(typeHierarchyRequested()), SLOT(perform()));
}

CppTypeHierarchyWidget::~CppTypeHierarchyWidget()
{}

void CppTypeHierarchyWidget::perform()
{
    showNoTypeHierarchyLabel();

    CPPEditor *editor = qobject_cast<CPPEditor *>(Core::EditorManager::currentEditor());
    if (!editor)
        return;

    CPPEditorWidget *widget = qobject_cast<CPPEditorWidget *>(editor->widget());
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
            m_inspectedClass->setup(cppClass);
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

void CppTypeHierarchyWidget::onItemClicked(const QModelIndex &index)
{
    const TextEditor::BaseTextEditorWidget::Link link
            = index.data(LinkRole).value<TextEditor::BaseTextEditorWidget::Link>();
    if (link.hasValidTarget())
        Core::EditorManager::openEditorAt(link.targetFileName,
                                          link.targetLine,
                                          link.targetColumn,
                                          Constants::CPPEDITOR_ID);
}

// CppTypeHierarchyStackedWidget
CppTypeHierarchyStackedWidget::CppTypeHierarchyStackedWidget(QWidget *parent) :
    QStackedWidget(parent),
    m_typeHiearchyWidgetInstance(new CppTypeHierarchyWidget)
{
    addWidget(m_typeHiearchyWidgetInstance);
}

CppTypeHierarchyStackedWidget::~CppTypeHierarchyStackedWidget()
{
    delete m_typeHiearchyWidgetInstance;
}

// CppTypeHierarchyFactory
CppTypeHierarchyFactory::CppTypeHierarchyFactory()
{}

CppTypeHierarchyFactory::~CppTypeHierarchyFactory()
{}

QString CppTypeHierarchyFactory::displayName() const
{
    return tr("Type Hierarchy");
}

int CppTypeHierarchyFactory::priority() const
{
    return Constants::TYPE_HIERARCHY_PRIORITY;
}

Core::Id CppTypeHierarchyFactory::id() const
{
    return Core::Id(Constants::TYPE_HIERARCHY_ID);
}

QKeySequence CppTypeHierarchyFactory::activationSequence() const
{
    return QKeySequence();
}

Core::NavigationView CppTypeHierarchyFactory::createWidget()
{
    CppTypeHierarchyStackedWidget *w = new CppTypeHierarchyStackedWidget;
    static_cast<CppTypeHierarchyWidget *>(w->currentWidget())->perform();
    Core::NavigationView navigationView;
    navigationView.widget = w;
    return navigationView;
}

} // namespace Internal
} // namespace CppEditor

