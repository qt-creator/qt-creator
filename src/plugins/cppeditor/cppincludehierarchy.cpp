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

#include "cppincludehierarchy.h"

#include "cppeditor.h"
#include "cppeditorconstants.h"
#include "cppeditorplugin.h"
#include "cppelementevaluator.h"
#include "cppincludehierarchymodel.h"
#include "cppincludehierarchytreeview.h"

#include <texteditor/textdocument.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/itemviewfind.h>
#include <cplusplus/CppDocument.h>

#include <utils/annotateditemdelegate.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QLabel>
#include <QLatin1String>
#include <QModelIndex>
#include <QStandardItem>
#include <QVBoxLayout>

using namespace TextEditor;
using namespace Utils;

namespace CppEditor {
namespace Internal {

// CppIncludeHierarchyWidget
CppIncludeHierarchyWidget::CppIncludeHierarchyWidget() :
    QWidget(0),
    m_treeView(0),
    m_model(0),
    m_delegate(0),
    m_includeHierarchyInfoLabel(0),
    m_editor(0)
{
    m_inspectedFile = new TextEditorLinkLabel(this);
    m_inspectedFile->setMargin(5);
    m_model = new CppIncludeHierarchyModel(this);
    m_treeView = new CppIncludeHierarchyTreeView(this);
    m_delegate = new AnnotatedItemDelegate(this);
    m_delegate->setDelimiter(QLatin1String(" "));
    m_delegate->setAnnotationRole(AnnotationRole);
    m_treeView->setModel(m_model);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setItemDelegate(m_delegate);
    connect(m_treeView, SIGNAL(activated(QModelIndex)), this, SLOT(onItemActivated(QModelIndex)));

    m_includeHierarchyInfoLabel = new QLabel(tr("No include hierarchy available"), this);
    m_includeHierarchyInfoLabel->setAlignment(Qt::AlignCenter);
    m_includeHierarchyInfoLabel->setAutoFillBackground(true);
    m_includeHierarchyInfoLabel->setBackgroundRole(QPalette::Base);
    m_includeHierarchyInfoLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_inspectedFile);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(
                          m_treeView,
                          Core::ItemViewFind::DarkColored,
                          Core::ItemViewFind::FetchMoreWhileSearching));
    layout->addWidget(m_includeHierarchyInfoLabel);
    setLayout(layout);

    connect(CppEditorPlugin::instance(), SIGNAL(includeHierarchyRequested()), SLOT(perform()));
    connect(Core::EditorManager::instance(), SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(editorsClosed(QList<Core::IEditor*>)));

}

CppIncludeHierarchyWidget::~CppIncludeHierarchyWidget()
{
}

void CppIncludeHierarchyWidget::perform()
{
    showNoIncludeHierarchyLabel();

    m_editor = qobject_cast<CppEditor *>(Core::EditorManager::currentEditor());
    if (!m_editor)
        return;

    CppEditorWidget *widget = qobject_cast<CppEditorWidget *>(m_editor->widget());
    if (!widget)
        return;

    m_model->clear();
    m_model->buildHierarchy(m_editor, widget->textDocument()->filePath().toString());
    if (m_model->isEmpty())
        return;

    m_inspectedFile->setText(widget->textDocument()->displayName());
    m_inspectedFile->setLink(TextEditorWidget::Link(widget->textDocument()->filePath().toString()));

    //expand "Includes"
    m_treeView->expand(m_model->index(0, 0));
    //expand "Included by"
    m_treeView->expand(m_model->index(1, 0));

    showIncludeHierarchy();
}

void CppIncludeHierarchyWidget::onItemActivated(const QModelIndex &index)
{
    const TextEditorWidget::Link link = index.data(LinkRole).value<TextEditorWidget::Link>();
    if (link.hasValidTarget())
        Core::EditorManager::openEditorAt(link.targetFileName,
                                          link.targetLine,
                                          link.targetColumn,
                                          Constants::CPPEDITOR_ID);
}

void CppIncludeHierarchyWidget::editorsClosed(QList<Core::IEditor *> editors)
{
    foreach (Core::IEditor *editor, editors) {
        if (m_editor == editor)
            perform();
    }
}

void CppIncludeHierarchyWidget::showNoIncludeHierarchyLabel()
{
    m_inspectedFile->hide();
    m_treeView->hide();
    m_includeHierarchyInfoLabel->show();
}

void CppIncludeHierarchyWidget::showIncludeHierarchy()
{
    m_inspectedFile->show();
    m_treeView->show();
    m_includeHierarchyInfoLabel->hide();
}

// CppIncludeHierarchyStackedWidget
CppIncludeHierarchyStackedWidget::CppIncludeHierarchyStackedWidget(QWidget *parent) :
    QStackedWidget(parent),
    m_typeHiearchyWidgetInstance(new CppIncludeHierarchyWidget)
{
    addWidget(m_typeHiearchyWidgetInstance);
}

CppIncludeHierarchyStackedWidget::~CppIncludeHierarchyStackedWidget()
{
    delete m_typeHiearchyWidgetInstance;
}

// CppIncludeHierarchyFactory
CppIncludeHierarchyFactory::CppIncludeHierarchyFactory()
{
    setDisplayName(tr("Include Hierarchy"));
    setPriority(800);
    setId(Constants::INCLUDE_HIERARCHY_ID);
}

Core::NavigationView CppIncludeHierarchyFactory::createWidget()
{
    CppIncludeHierarchyStackedWidget *w = new CppIncludeHierarchyStackedWidget;
    static_cast<CppIncludeHierarchyWidget *>(w->currentWidget())->perform();
    Core::NavigationView navigationView;
    navigationView.widget = w;
    return navigationView;
}

} // namespace Internal
} // namespace CppEditor
