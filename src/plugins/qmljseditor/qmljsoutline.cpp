#include "qmljsoutline.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QtCore/QDebug>
#include <QtGui/QVBoxLayout>

#include <typeinfo>

using namespace QmlJS;

enum {
    debug = false
};

namespace QmlJSEditor {
namespace Internal {

QmlOutlineModel::QmlOutlineModel(QObject *parent) :
    QStandardItemModel(parent)
{
}

QmlJSOutlineTreeView::QmlJSOutlineTreeView(QWidget *parent) :
    QTreeView(parent)
{
    // see also CppOutlineTreeView
    setFocusPolicy(Qt::NoFocus);
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setUniformRowHeights(true);
    setHeaderHidden(true);
    setTextElideMode(Qt::ElideNone);
    setIndentation(20);
    setExpandsOnDoubleClick(false);
}

void QmlOutlineModel::startSync()
{
    m_treePos.clear();
    m_treePos.append(0);
    m_currentItem = invisibleRootItem();
}

QModelIndex QmlOutlineModel::enterElement(const QString &type, const AST::SourceLocation &sourceLocation)
{
    QStandardItem *item = enterNode(sourceLocation);
    item->setText(type);
    item->setIcon(m_icons.objectDefinitionIcon());
    return item->index();
}

void QmlOutlineModel::leaveElement()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterProperty(const QString &name, const AST::SourceLocation &sourceLocation)
{
    QStandardItem *item = enterNode(sourceLocation);
    item->setText(name);
    item->setIcon(m_icons.scriptBindingIcon());
    return item->index();
}

void QmlOutlineModel::leaveProperty()
{
    leaveNode();
}

QStandardItem *QmlOutlineModel::enterNode(const QmlJS::AST::SourceLocation &location)
{
    int siblingIndex = m_treePos.last();
    if (siblingIndex == 0) {
        // first child
        if (!m_currentItem->hasChildren()) {
            QStandardItem *parentItem = m_currentItem;
            m_currentItem = new QStandardItem;
            m_currentItem->setEditable(false);
            parentItem->appendRow(m_currentItem);
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << parentItem->text();
        } else {
            m_currentItem = m_currentItem->child(0);
        }
    } else {
        // sibling
        if (m_currentItem->rowCount() <= siblingIndex) {
            // attach
            QStandardItem *oldItem = m_currentItem;
            m_currentItem = new QStandardItem;
            m_currentItem->setEditable(false);
            oldItem->appendRow(m_currentItem);
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << oldItem->text();
        } else {
            m_currentItem = m_currentItem->child(siblingIndex);
        }
    }

    m_treePos.append(0);
    m_currentItem->setData(QVariant::fromValue(location), SourceLocationRole);

    return m_currentItem;
}

void QmlOutlineModel::leaveNode()
{
    int lastIndex = m_treePos.takeLast();


    if (lastIndex > 0) {
        // element has children
        if (lastIndex < m_currentItem->rowCount()) {
            if (debug)
                qDebug() << "QmlOutlineModel - removeRows from " << m_currentItem->text() << lastIndex << m_currentItem->rowCount() - lastIndex;
            m_currentItem->removeRows(lastIndex, m_currentItem->rowCount() - lastIndex);
        }
        m_currentItem = parentItem();
    } else {
        if (m_currentItem->hasChildren()) {
            if (debug)
                qDebug() << "QmlOutlineModel - removeRows from " << m_currentItem->text() << 0 << m_currentItem->rowCount();
            m_currentItem->removeRows(0, m_currentItem->rowCount());
        }
        m_currentItem = parentItem();
    }


    m_treePos.last()++;
}

QStandardItem *QmlOutlineModel::parentItem()
{
    QStandardItem *parent = m_currentItem->parent();
    if (!parent)
        parent = invisibleRootItem();
    return parent;
}

class QmlOutlineModelSync : protected AST::Visitor
{
public:
    QmlOutlineModelSync(QmlOutlineModel *model) :
        m_model(model),
        indent(0)
    {
    }

    void operator()(Document::Ptr doc)
    {
        m_nodeToIndex.clear();

        if (debug)
            qDebug() << "QmlOutlineModel ------";
        m_model->startSync();
        if (doc && doc->ast())
            doc->ast()->accept(this);
    }

private:
    bool preVisit(AST::Node *node)
    {
        if (!node)
            return false;
        if (debug)
            qDebug() << "QmlOutlineModel -" << QByteArray(indent++, '-').constData() << node << typeid(*node).name();
        return true;
    }

    void postVisit(AST::Node *)
    {
        indent--;
    }

    QString asString(AST::UiQualifiedId *id)
    {
        QString text;
        for (; id; id = id->next) {
            if (id->name)
                text += id->name->asString();
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    bool visit(AST::UiObjectDefinition *objDef)
    {
        AST::SourceLocation location;
        location.offset = objDef->firstSourceLocation().offset;
        location.length = objDef->lastSourceLocation().offset
                - objDef->firstSourceLocation().offset
                + objDef->lastSourceLocation().length;

        QModelIndex index = m_model->enterElement(asString(objDef->qualifiedTypeNameId), location);
        m_nodeToIndex.insert(objDef, index);
        return true;
    }

    void endVisit(AST::UiObjectDefinition * /*objDefinition*/)
    {
        m_model->leaveElement();
    }

    bool visit(AST::UiScriptBinding *scriptBinding)
    {
        AST::SourceLocation location;
        location.offset = scriptBinding->firstSourceLocation().offset;
        location.length = scriptBinding->lastSourceLocation().offset
                - scriptBinding->firstSourceLocation().offset
                + scriptBinding->lastSourceLocation().length;

        QModelIndex index = m_model->enterProperty(asString(scriptBinding->qualifiedId), location);
        m_nodeToIndex.insert(scriptBinding, index);

        return true;
    }

    void endVisit(AST::UiScriptBinding * /*scriptBinding*/)
    {
        m_model->leaveProperty();
    }

    QmlOutlineModel *m_model;
    QHash<AST::Node*, QModelIndex> m_nodeToIndex;
    int indent;
};


QmlJSOutlineWidget::QmlJSOutlineWidget(QWidget *parent) :
    TextEditor::IOutlineWidget(parent),
    m_treeView(new QmlJSOutlineTreeView()),
    m_model(new QmlOutlineModel),
    m_enableCursorSync(true),
    m_blockCursorSync(false)
{
    QVBoxLayout *layout = new QVBoxLayout;

    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_treeView);

    setLayout(layout);

    m_treeView->setModel(m_model);

    connect(m_treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateSelectionInText(QItemSelection)));
}

void QmlJSOutlineWidget::setEditor(QmlJSTextEditor *editor)
{
    m_editor = editor;

    connect(m_editor.data(), SIGNAL(semanticInfoUpdated(QmlJSEditor::Internal::SemanticInfo)),
            this, SLOT(updateOutline(QmlJSEditor::Internal::SemanticInfo)));
    connect(m_editor.data(), SIGNAL(cursorPositionChanged()),
            this, SLOT(updateSelectionInTree()));

    updateOutline(m_editor.data()->semanticInfo());
}

void QmlJSOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    if (m_enableCursorSync)
        updateSelectionInTree();
}

void QmlJSOutlineWidget::updateOutline(const QmlJSEditor::Internal::SemanticInfo &semanticInfo)
{
    Document::Ptr doc = semanticInfo.document;

    if (!doc) {
        return;
    }

    if (!m_editor
            || m_editor.data()->file()->fileName() != doc->fileName()
            || m_editor.data()->documentRevision() != doc->editorRevision()) {
        return;
    }

    if (doc->ast()
            && m_model) {

        // got a correctly parsed (or recovered) file.

        if (QmlOutlineModel *qmlModel = qobject_cast<QmlOutlineModel*>(m_model)) {
            QmlOutlineModelSync syncModel(qmlModel);
            syncModel(doc);
        }
    } else {
        // TODO: Maybe disable view?
    }

    m_treeView->expandAll();
}

QModelIndex QmlJSOutlineWidget::indexForPosition(const QModelIndex &rootIndex, int cursorPosition)
{
    if (!rootIndex.isValid())
        return QModelIndex();

    AST::SourceLocation location = rootIndex.data(QmlOutlineModel::SourceLocationRole).value<AST::SourceLocation>();

    if (!offsetInsideLocation(cursorPosition, location)) {
        return QModelIndex();
    }

    const int rowCount = rootIndex.model()->rowCount(rootIndex);
    for (int i = 0; i < rowCount; ++i) {
        QModelIndex childIndex = rootIndex.child(i, 0);
        QModelIndex resultIndex = indexForPosition(childIndex, cursorPosition);
        if (resultIndex.isValid())
            return resultIndex;
    }

    return rootIndex;
}

void QmlJSOutlineWidget::updateSelectionInTree()
{
    if (!syncCursor())
        return;

    int absoluteCursorPos = m_editor.data()->textCursor().position();
    QModelIndex index = indexForPosition(m_model->index(0, 0), absoluteCursorPos);

    m_blockCursorSync = true;
    m_treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    m_treeView->scrollTo(index);
    m_blockCursorSync = false;
}

void QmlJSOutlineWidget::updateSelectionInText(const QItemSelection &selection)
{
    if (!syncCursor())
        return;


    if (!selection.indexes().isEmpty()) {
        QModelIndex index = selection.indexes().first();
        AST::SourceLocation location = index.data(QmlOutlineModel::SourceLocationRole).value<AST::SourceLocation>();

        QTextCursor textCursor = m_editor.data()->textCursor();
        m_blockCursorSync = true;
        textCursor.setPosition(location.offset);
        m_editor.data()->setTextCursor(textCursor);
        m_blockCursorSync = false;
    }
}

bool QmlJSOutlineWidget::offsetInsideLocation(quint32 offset, const QmlJS::AST::SourceLocation &location)
{
    return ((offset >= location.offset)
            && (offset <= location.offset + location.length));
}

bool QmlJSOutlineWidget::syncCursor()
{
    return m_enableCursorSync && !m_blockCursorSync;
}

bool QmlJSOutlineWidgetFactory::supportsEditor(Core::IEditor *editor) const
{
    if (qobject_cast<QmlJSEditorEditable*>(editor))
        return true;
    return false;
}

TextEditor::IOutlineWidget *QmlJSOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    QmlJSOutlineWidget *widget = new QmlJSOutlineWidget;

    QmlJSEditorEditable *qmlJSEditable = qobject_cast<QmlJSEditorEditable*>(editor);
    QmlJSTextEditor *qmlJSEditor = qobject_cast<QmlJSTextEditor*>(qmlJSEditable->widget());
    Q_ASSERT(qmlJSEditor);

    widget->setEditor(qmlJSEditor);

    return widget;
}

} // namespace Internal
} // namespace QmlJSEditor
