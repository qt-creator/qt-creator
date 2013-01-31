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

#include "cpptypehierarchy.h"
#include "cppeditorconstants.h"
#include "cppeditor.h"
#include "cppelementevaluator.h"
#include "cppplugin.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/navigationtreeview.h>
#include <utils/annotateditemdelegate.h>

#include <QLatin1Char>
#include <QLatin1String>
#include <QModelIndex>
#include <QVector>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QLabel>

using namespace CppEditor;
using namespace Internal;
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

bool compareCppClassNames(const CppClass &c1, const CppClass &c2)
{
    const QString key1 = c1.name + QLatin1String("::") + c1.qualifiedName;
    const QString key2 = c2.name + QLatin1String("::") + c2.qualifiedName;
    return key1 < key2;
}

QList<CppClass> sortClasses(const QList<CppClass> &cppClasses)
{
    QList<CppClass> sorted = cppClasses;
    qSort(sorted.begin(), sorted.end(), compareCppClassNames);
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

private:
    void mousePressEvent(QMouseEvent *)
    {
        if (m_link.fileName.isEmpty())
            return;

        TextEditor::BaseTextEditorWidget::openEditorAt(m_link.fileName,
                                                       m_link.line,
                                                       m_link.column,
                                                       Constants::CPPEDITOR_ID);
    }

    CPPEditorWidget::Link m_link;
};

} // namespace Internal
} // namespace CppEditor

// CppTypeHierarchyWidget
CppTypeHierarchyWidget::CppTypeHierarchyWidget(Core::IEditor *editor) :
    QWidget(0),
    m_treeView(0),
    m_model(0),
    m_delegate(0)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    if (qobject_cast<CPPEditor *>(editor)) {
        m_inspectedClass = new CppClassLabel(this);
        m_inspectedClass->setMargin(5);
        layout->addWidget(m_inspectedClass);
        m_model = new QStandardItemModel(this);
        m_treeView = new NavigationTreeView(this);
        m_delegate = new AnnotatedItemDelegate(this);
        m_delegate->setDelimiter(QLatin1String(" "));
        m_delegate->setAnnotationRole(AnnotationRole);
        m_treeView->setModel(m_model);
        m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_treeView->setItemDelegate(m_delegate);
        m_treeView->setRootIsDecorated(false);
        layout->addWidget(m_treeView);

        connect(m_treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(onItemClicked(QModelIndex)));
        connect(CppPlugin::instance(), SIGNAL(typeHierarchyRequested()), this, SLOT(perform()));
    } else {
        QLabel *label = new QLabel(tr("No type hierarchy available"), this);
        label->setAlignment(Qt::AlignCenter);
        label->setAutoFillBackground(true);
        label->setBackgroundRole(QPalette::Base);
        layout->addWidget(label);
    }
    setLayout(layout);
}

CppTypeHierarchyWidget::~CppTypeHierarchyWidget()
{}

void CppTypeHierarchyWidget::perform()
{
    CPPEditor *editor = qobject_cast<CPPEditor *>(Core::EditorManager::instance()->currentEditor());
    if (!editor)
        return;
    CPPEditorWidget *widget = qobject_cast<CPPEditorWidget *>(editor->widget());
    if (!widget)
        return;

    m_model->clear();

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
        }
    }
}

void CppTypeHierarchyWidget::buildHierarchy(const CppClass &cppClass, QStandardItem *parent, bool isRoot, const HierarchyMember member)
{
    if (!isRoot) {
        QStandardItem *item = itemForClass(cppClass);
        parent->appendRow(item);
        parent = item;
    }
    foreach (const CppClass &klass, sortClasses(cppClass.*member))
        buildHierarchy(klass, parent, false, member);
}

void CppTypeHierarchyWidget::onItemClicked(const QModelIndex &index)
{
    const TextEditor::BaseTextEditorWidget::Link link
            = index.data(LinkRole).value<TextEditor::BaseTextEditorWidget::Link>();
    if (!link.fileName.isEmpty())
        TextEditor::BaseTextEditorWidget::openEditorAt(link.fileName,
                                                       link.line,
                                                       link.column,
                                                       Constants::CPPEDITOR_ID);
}

// CppTypeHierarchyStackedWidget
CppTypeHierarchyStackedWidget::CppTypeHierarchyStackedWidget(QWidget *parent) :
    QStackedWidget(parent),
    m_typeHiearchyWidgetInstance(new CppTypeHierarchyWidget(Core::EditorManager::currentEditor()))
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
