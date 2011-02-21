/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
#include <QtGui/QVBoxLayout>
#include <QtGui/QStandardItemModel>
#include <QtGui/QLabel>

using namespace CppEditor;
using namespace Internal;
using namespace Utils;

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

        m_model = new QStandardItemModel;
        m_treeView = new Utils::NavigationTreeView;
        m_delegate = new AnnotatedItemDelegate;
        m_delegate->setDelimiter(QLatin1String(" "));
        m_delegate->setAnnotationRole(AnnotationRole);
        m_treeView->setModel(m_model);
        m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_treeView->setItemDelegate(m_delegate);
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
{
    delete m_model;
    delete m_delegate;
}

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
    evaluator.execute();
    if (evaluator.identifiedCppElement()) {
        const QSharedPointer<CppElement> &cppElement = evaluator.cppElement();
        CppElement *element = cppElement.data();
        if (CppClass *cppClass = dynamic_cast<CppClass *>(element))
            buildModel(*cppClass, m_model->invisibleRootItem());
    }
}

void CppTypeHierarchyWidget::buildModel(const CppClass &cppClass, QStandardItem *parent)
{
    QStandardItem *item = new QStandardItem;
    parent->appendRow(item);

    m_model->setData(m_model->indexFromItem(item), cppClass.name(), Qt::DisplayRole);
    if (cppClass.name() != cppClass.qualifiedName())
        m_model->setData(m_model->indexFromItem(item), cppClass.qualifiedName(), AnnotationRole);
    m_model->setData(m_model->indexFromItem(item), cppClass.icon(), Qt::DecorationRole);
    QVariant link;
    link.setValue(CPPEditorWidget::Link(cppClass.link()));
    m_model->setData(m_model->indexFromItem(item), link, LinkRole);

    foreach (const CppClass &cppBase, cppClass.bases())
        buildModel(cppBase, item);

    m_treeView->expand(m_model->indexFromItem(item));
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
