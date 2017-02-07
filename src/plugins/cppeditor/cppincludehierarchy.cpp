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

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/find/itemviewfind.h>

#include <cpptools/baseeditordocumentprocessor.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsbridge.h>
#include <cpptools/editordocumenthandle.h>

#include <cplusplus/CppDocument.h>

#include <utils/annotateditemdelegate.h>
#include <utils/dropsupport.h>
#include <utils/fileutils.h>
#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

using namespace Core;
using namespace CPlusPlus;
using namespace CppTools;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor {
namespace Internal {

enum {
    AnnotationRole = Qt::UserRole + 1,
    LinkRole
};

static Snapshot globalSnapshot()
{
    return CppModelManager::instance()->snapshot();
}

struct FileAndLine
{
    FileAndLine() {}
    FileAndLine(const QString &f, int l) : file(f), line(l) {}

    QString file;
    int line = 0;
};

using FileAndLines = QList<FileAndLine>;

static FileAndLines findIncluders(const QString &filePath)
{
    FileAndLines result;
    const Snapshot snapshot = globalSnapshot();
    for (auto cit = snapshot.begin(), citEnd = snapshot.end(); cit != citEnd; ++cit) {
        const QString filePathFromSnapshot = cit.key().toString();
        Document::Ptr doc = cit.value();
        const QList<Document::Include> resolvedIncludes = doc->resolvedIncludes();
        for (const auto &includeFile : resolvedIncludes) {
            const QString includedFilePath = includeFile.resolvedFileName();
            if (includedFilePath == filePath)
                result.append(FileAndLine(filePathFromSnapshot, int(includeFile.line())));
        }
    }
    return result;
}

static FileAndLines findIncludes(const QString &filePath, const Snapshot &snapshot)
{
    FileAndLines result;
    if (Document::Ptr doc = snapshot.document(filePath)) {
        const QList<Document::Include> resolvedIncludes = doc->resolvedIncludes();
        for (const auto &includeFile : resolvedIncludes)
            result.append(FileAndLine(includeFile.resolvedFileName(), 0));
    }
    return result;
}

class CppIncludeHierarchyItem
    : public TypedTreeItem<CppIncludeHierarchyItem, CppIncludeHierarchyItem>
{
public:
    enum SubTree { RootItem, InIncludes, InIncludedBy };
    CppIncludeHierarchyItem() {}

    void createChild(const QString &filePath, SubTree subTree,
                     int line = 0, bool definitelyNoChildren = false)
    {
        auto item = new CppIncludeHierarchyItem;
        item->m_fileName = filePath.mid(filePath.lastIndexOf('/') + 1);
        item->m_filePath = filePath;
        item->m_line = line;
        item->m_subTree = subTree;
        appendChild(item);
        for (auto ancestor = this; ancestor; ancestor = ancestor->parent()) {
            if (ancestor->filePath() == filePath) {
                item->m_isCyclic = true;
                break;
            }
        }
        if (filePath == model()->editorFilePath() || definitelyNoChildren)
            item->setChildrenChecked();
    }

    QString filePath() const
    {
        return isPhony() ? model()->editorFilePath() : m_filePath;
    }

private:
    bool isPhony() const { return !parent() || !parent()->parent(); }
    void setChildrenChecked() { m_checkedForChildren = true; }

    CppIncludeHierarchyModel *model() const
    {
        return static_cast<CppIncludeHierarchyModel *>(TreeItem::model());
    }

    QVariant data(int column, int role) const override;

    Qt::ItemFlags flags(int) const override
    {
        TextEditorWidget::Link link(m_filePath, m_line);
        if (link.hasValidTarget())
            return Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    bool canFetchMore() const override;
    void fetchMore() override;

    QString m_fileName;
    QString m_filePath;
    int m_line = 0;
    SubTree m_subTree = RootItem;
    bool m_isCyclic = false;
    bool m_checkedForChildren = false;
};

QVariant CppIncludeHierarchyItem::data(int column, int role) const
{
    Q_UNUSED(column);
    if (role == Qt::DisplayRole) {
        if (isPhony() && childCount() == 0)
            return QString(m_fileName + ' ' + CppIncludeHierarchyModel::tr("(none)"));
        if (m_isCyclic)
            return QString(m_fileName + ' ' + CppIncludeHierarchyModel::tr("(cyclic)"));
        return m_fileName;
    }

    if (isPhony())
        return QVariant();

    switch (role) {
        case Qt::ToolTipRole:
            return m_filePath;
        case Qt::DecorationRole:
            return FileIconProvider::icon(QFileInfo(m_filePath));
        case LinkRole:
            return QVariant::fromValue(TextEditorWidget::Link(m_filePath, m_line));
    }

    return QVariant();
}

bool CppIncludeHierarchyItem::canFetchMore() const
{
    if (m_isCyclic || m_checkedForChildren || childCount() > 0)
        return false;

    return !model()->m_searching || !model()->m_seen.contains(m_filePath);
}

void CppIncludeHierarchyItem::fetchMore()
{
    QTC_ASSERT(canFetchMore(), setChildrenChecked(); return);
    QTC_ASSERT(model(), return);
    QTC_ASSERT(m_subTree != RootItem, return); // Root should always be populated.

    model()->m_seen.insert(m_filePath);

    const QString editorFilePath = model()->editorFilePath();

    setChildrenChecked();
    if (m_subTree == InIncludes) {
        auto processor = CppToolsBridge::baseEditorDocumentProcessor(editorFilePath);
        QTC_ASSERT(processor, return);
        const Snapshot snapshot = processor->snapshot();
        const FileAndLines includes = findIncludes(filePath(), snapshot);
        for (const FileAndLine &include : includes) {
            const FileAndLines subIncludes = findIncludes(include.file, snapshot);
            bool definitelyNoChildren = subIncludes.isEmpty();
            createChild(include.file, InIncludes, include.line, definitelyNoChildren);
        }
    } else if (m_subTree == InIncludedBy) {
        const FileAndLines includers = findIncluders(filePath());
        for (const FileAndLine &includer : includers) {
            const FileAndLines subIncluders = findIncluders(includer.file);
            bool definitelyNoChildren = subIncluders.isEmpty();
            createChild(includer.file, InIncludedBy, includer.line, definitelyNoChildren);
        }
    }
}

void CppIncludeHierarchyModel::buildHierarchy(const QString &document)
{
    m_editorFilePath = document;
    rootItem()->removeChildren();
    rootItem()->createChild(tr("Includes"), CppIncludeHierarchyItem::InIncludes);
    rootItem()->createChild(tr("Included by"), CppIncludeHierarchyItem::InIncludedBy);
}

void CppIncludeHierarchyModel::setSearching(bool on)
{
    m_searching = on;
    m_seen.clear();
}


// CppIncludeHierarchyModel

CppIncludeHierarchyModel::CppIncludeHierarchyModel()
{
    setRootItem(new CppIncludeHierarchyItem); // FIXME: Remove in 4.2
}

Qt::DropActions CppIncludeHierarchyModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

QStringList CppIncludeHierarchyModel::mimeTypes() const
{
    return DropSupport::mimeTypesForFilePaths();
}

QMimeData *CppIncludeHierarchyModel::mimeData(const QModelIndexList &indexes) const
{
    auto data = new DropMimeData;
    foreach (const QModelIndex &index, indexes) {
        auto link = index.data(LinkRole).value<TextEditorWidget::Link>();
        if (link.hasValidTarget())
            data->addFile(link.targetFileName, link.targetLine, link.targetColumn);
    }
    return data;
}


// CppIncludeHierarchyTreeView

class CppIncludeHierarchyTreeView : public NavigationTreeView
{
public:
    CppIncludeHierarchyTreeView()
    {
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragOnly);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key())
            QAbstractItemView::keyPressEvent(event);
        else
            NavigationTreeView::keyPressEvent(event);
    }
};


// IncludeFinder

class IncludeFinder : public ItemViewFind
{
public:
    IncludeFinder(QAbstractItemView *view, CppIncludeHierarchyModel *model)
        : ItemViewFind(view, Qt::DisplayRole, FetchMoreWhileSearching)
        , m_model(model)
    {}

private:
    Result findIncremental(const QString &txt, FindFlags findFlags)
    {
        m_model->setSearching(true);
        Result result = ItemViewFind::findIncremental(txt, findFlags);
        m_model->setSearching(false);
        return result;
    }

    CppIncludeHierarchyModel *m_model; // Not owned.
};


// CppIncludeHierarchyWidget

class CppIncludeHierarchyWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(CppEditor::CppIncludeHierarchy)

public:
    CppIncludeHierarchyWidget();
    ~CppIncludeHierarchyWidget() { delete m_treeView; }

    void perform();

private:
    void onItemActivated(const QModelIndex &index);
    void editorsClosed(QList<IEditor *> editors);
    void showNoIncludeHierarchyLabel();
    void showIncludeHierarchy();

    CppIncludeHierarchyTreeView *m_treeView = nullptr;
    CppIncludeHierarchyModel m_model;
    AnnotatedItemDelegate m_delegate;
    TextEditorLinkLabel *m_inspectedFile = nullptr;
    QLabel *m_includeHierarchyInfoLabel = nullptr;
    BaseTextEditor *m_editor = nullptr;
};

CppIncludeHierarchyWidget::CppIncludeHierarchyWidget()
{
    m_delegate.setDelimiter(" ");
    m_delegate.setAnnotationRole(AnnotationRole);

    m_inspectedFile = new TextEditorLinkLabel(this);
    m_inspectedFile->setMargin(5);

    m_treeView = new CppIncludeHierarchyTreeView;
    m_treeView->setModel(&m_model);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setItemDelegate(&m_delegate);
    connect(m_treeView, &QAbstractItemView::activated, this, &CppIncludeHierarchyWidget::onItemActivated);

    m_includeHierarchyInfoLabel = new QLabel(tr("No include hierarchy available"), this);
    m_includeHierarchyInfoLabel->setAlignment(Qt::AlignCenter);
    m_includeHierarchyInfoLabel->setAutoFillBackground(true);
    m_includeHierarchyInfoLabel->setBackgroundRole(QPalette::Base);
    m_includeHierarchyInfoLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_inspectedFile);
    layout->addWidget(ItemViewFind::createSearchableWrapper(new IncludeFinder(m_treeView, &m_model)));
    layout->addWidget(m_includeHierarchyInfoLabel);

    connect(CppEditorPlugin::instance(), &CppEditorPlugin::includeHierarchyRequested,
            this, &CppIncludeHierarchyWidget::perform);
    connect(EditorManager::instance(), &EditorManager::editorsClosed,
            this, &CppIncludeHierarchyWidget::editorsClosed);
}

void CppIncludeHierarchyWidget::perform()
{
    showNoIncludeHierarchyLabel();

    m_editor = qobject_cast<CppEditor *>(EditorManager::currentEditor());
    if (!m_editor)
        return;

    QString document = m_editor->textDocument()->filePath().toString();
    m_model.buildHierarchy(document);

    m_inspectedFile->setText(m_editor->textDocument()->displayName());
    m_inspectedFile->setLink(TextEditorWidget::Link(document));

    // expand "Includes" adn "Included by"
    m_treeView->expand(m_model.index(0, 0));
    m_treeView->expand(m_model.index(1, 0));

    showIncludeHierarchy();
}

void CppIncludeHierarchyWidget::onItemActivated(const QModelIndex &index)
{
    const auto link = index.data(LinkRole).value<TextEditorWidget::Link>();
    if (link.hasValidTarget())
        EditorManager::openEditorAt(link.targetFileName,
                                    link.targetLine,
                                    link.targetColumn,
                                    Constants::CPPEDITOR_ID);
}

void CppIncludeHierarchyWidget::editorsClosed(QList<IEditor *> editors)
{
    foreach (IEditor *editor, editors) {
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


// CppIncludeHierarchyFactory

CppIncludeHierarchyFactory::CppIncludeHierarchyFactory()
{
    setDisplayName(tr("Include Hierarchy"));
    setPriority(800);
    setId(Constants::INCLUDE_HIERARCHY_ID);
}

NavigationView CppIncludeHierarchyFactory::createWidget()
{
    auto hierarchyWidget = new CppIncludeHierarchyWidget;
    hierarchyWidget->perform();

    auto stack = new QStackedWidget;
    stack->addWidget(hierarchyWidget);

    NavigationView navigationView;
    navigationView.widget = stack;
    return navigationView;
}

} // namespace Internal
} // namespace CppEditor
