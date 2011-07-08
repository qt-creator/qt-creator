/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "cpptypehierarchy.h"
#include "cppeditorconstants.h"
#include "cppeditor.h"
#include "cppelementevaluator.h"
#include "cppplugin.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/navigationtreeview.h>
#include <utils/annotateditemdelegate.h>

#include <QtCore/QLatin1Char>
#include <QtCore/QLatin1String>
#include <QtCore/QModelIndex>
#include <QtCore/QVector>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStandardItemModel>
#include <QtGui/QLabel>

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
    item->setData(cppClass.name(), Qt::DisplayRole);
    if (cppClass.name() != cppClass.qualifiedName())
        item->setData(cppClass.qualifiedName(), AnnotationRole);
    item->setData(cppClass.icon(), Qt::DecorationRole);
    QVariant link;
    link.setValue(CPPEditorWidget::Link(cppClass.link()));
    item->setData(link, LinkRole);
    return item;
}

} // Anonymous

// CppTypeHierarchyWidget
CppTypeHierarchyWidget::CppTypeHierarchyWidget(Core::IEditor *editor) :
    QWidget(0),
    m_cppEditor(0),
    m_treeView(0),
    m_model(0),
    m_delegate(0)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    if (CPPEditor *cppEditor = qobject_cast<CPPEditor *>(editor)) {
        m_cppEditor = static_cast<CPPEditorWidget *>(cppEditor->widget());

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

bool CppTypeHierarchyWidget::handleEditorChange(Core::IEditor *editor)
{
    if (CPPEditor *cppEditor = qobject_cast<CPPEditor *>(editor)) {
        if (m_cppEditor) {
            m_cppEditor = static_cast<CPPEditorWidget *>(cppEditor->widget());
            return true;
        }
    } else if (!m_cppEditor) {
        return true;
    }
    return false;
}

void CppTypeHierarchyWidget::perform()
{
    if (!m_cppEditor)
        return;

    m_model->clear();

    CppElementEvaluator evaluator(m_cppEditor);
    evaluator.setLookupBaseClasses(true);
    evaluator.setLookupDerivedClasses(true);
    evaluator.execute();
    if (evaluator.identifiedCppElement()) {
        const QSharedPointer<CppElement> &cppElement = evaluator.cppElement();
        CppElement *element = cppElement.data();
        if (CppClass *cppClass = dynamic_cast<CppClass *>(element)) {
            QStandardItem *bases = new QStandardItem(tr("Bases"));
            m_model->invisibleRootItem()->appendRow(bases);
            QVector<CppClass> v;
            v.push_back(*cppClass);
            buildBaseHierarchy(&v);
            m_treeView->expand(m_model->indexFromItem(bases));
            QStandardItem *derived = new QStandardItem(tr("Derived"));
            m_model->invisibleRootItem()->appendRow(derived);
            buildDerivedHierarchy(*cppClass, derived);
        }
    }
}

void CppTypeHierarchyWidget::buildBaseHierarchy(QVector<CppClass> *s)
{
    const CppClass &current = s->back();
    const QList<CppClass> &bases = current.bases();
    if (!bases.isEmpty()) {
        foreach (const CppClass &base, bases) {
            s->push_back(base);
            buildBaseHierarchy(s);
            s->pop_back();
        }
    } else {
        QStandardItem *parent = m_model->item(0, 0);
        for (int i = s->size() - 1; i >= 0; --i) {
            QStandardItem *item = itemForClass(s->at(i));
            parent->appendRow(item);
            parent = item;
        }
    }
}

void CppTypeHierarchyWidget::buildDerivedHierarchy(const CppClass &cppClass, QStandardItem *parent)
{
    QStandardItem *item = itemForClass(cppClass);
    parent->appendRow(item);
    foreach (const CppClass &derived, cppClass.derived())
        buildDerivedHierarchy(derived, item);
    m_treeView->expand(m_model->indexFromItem(parent));
}

void CppTypeHierarchyWidget::onItemClicked(const QModelIndex &index)
{
    m_cppEditor->openLink(index.data(LinkRole).value<TextEditor::BaseTextEditorWidget::Link>());
}

// CppTypeHierarchyStackedWidget
CppTypeHierarchyStackedWidget::CppTypeHierarchyStackedWidget(QWidget *parent) :
    QStackedWidget(parent),
    m_typeHiearchyWidgetInstance(
        new CppTypeHierarchyWidget(Core::EditorManager::instance()->currentEditor()))
{
    addWidget(m_typeHiearchyWidgetInstance);

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(editorChanged(Core::IEditor*)));
}

CppTypeHierarchyStackedWidget::~CppTypeHierarchyStackedWidget()
{
    delete m_typeHiearchyWidgetInstance;
}

void CppTypeHierarchyStackedWidget::editorChanged(Core::IEditor *editor)
{
    if (!m_typeHiearchyWidgetInstance->handleEditorChange(editor)) {
        CppTypeHierarchyWidget *replacement = new CppTypeHierarchyWidget(editor);
        removeWidget(m_typeHiearchyWidgetInstance);
        m_typeHiearchyWidgetInstance->deleteLater();
        m_typeHiearchyWidgetInstance = replacement;
        addWidget(m_typeHiearchyWidgetInstance);
    }
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

QString CppTypeHierarchyFactory::id() const
{
    return QLatin1String(Constants::TYPE_HIERARCHY_ID);
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
