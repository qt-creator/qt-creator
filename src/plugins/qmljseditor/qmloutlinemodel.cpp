#include "qmloutlinemodel.h"
#include "qmljseditor.h"
#include "qmljsrefactoringchanges.h"

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljslookupcontext.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsrewriter.h>

#include <coreplugin/icore.h>
#include <QtCore/QDebug>
#include <QtCore/QTime>
#include <typeinfo>

using namespace QmlJS;
using namespace QmlJSEditor::Internal;

enum {
    debug = false
};

namespace QmlJSEditor {
namespace Internal {

QmlOutlineItem::QmlOutlineItem(QmlOutlineModel *model) :
    m_outlineModel(model),
    m_node(0),
    m_idNode(0)
{
    Qt::ItemFlags defaultFlags = flags();
    setFlags(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags);
    setEditable(false);
}

QVariant QmlOutlineItem::data(int role) const
{
    if (role == Qt::ToolTipRole) {
        AST::SourceLocation location = sourceLocation();
        AST::UiQualifiedId *uiQualifiedId = m_idNode;
        if (!uiQualifiedId)
            return QVariant();

        QList<AST::Node *> astPath = m_outlineModel->m_semanticInfo.astPath(location.begin());

        QmlJS::Document::Ptr document = m_outlineModel->m_semanticInfo.document;
        QmlJS::Snapshot snapshot = m_outlineModel->m_semanticInfo.snapshot;
        LookupContext::Ptr lookupContext = LookupContext::create(document, snapshot, astPath);
        const Interpreter::Value *value = lookupContext->evaluate(uiQualifiedId);

        return prettyPrint(value, lookupContext->context());
    }

    return QStandardItem::data(role);
}

int QmlOutlineItem::type() const
{
    return UserType;
}

QString QmlOutlineItem::annotation() const
{
    return data(QmlOutlineModel::AnnotationRole).value<QString>();
}

void QmlOutlineItem::setAnnotation(const QString &id)
{
    setData(QVariant::fromValue(id), QmlOutlineModel::AnnotationRole);
}

QmlJS::AST::SourceLocation QmlOutlineItem::sourceLocation() const
{
    return data(QmlOutlineModel::SourceLocationRole).value<QmlJS::AST::SourceLocation>();
}

void QmlOutlineItem::setSourceLocation(const QmlJS::AST::SourceLocation &location)
{
    setData(QVariant::fromValue(location), QmlOutlineModel::SourceLocationRole);
}

QmlJS::AST::Node *QmlOutlineItem::node() const
{
    return m_node;
}

void QmlOutlineItem::setNode(QmlJS::AST::Node *node)
{
    m_node = node;
}

QmlJS::AST::UiQualifiedId *QmlOutlineItem::idNode() const
{
    return m_idNode;
}

void QmlOutlineItem::setIdNode(QmlJS::AST::UiQualifiedId *idNode)
{
    m_idNode = idNode;
}

QmlOutlineItem &QmlOutlineItem::copyValues(const QmlOutlineItem &other)
{
    *this = other;
    m_node = other.m_node;
    m_idNode = other.m_idNode;
    emitDataChanged();
    return *this;
}

QString QmlOutlineItem::prettyPrint(const QmlJS::Interpreter::Value *value, QmlJS::Interpreter::Context *context) const
{
    if (! value)
        return QString();

    if (const Interpreter::ObjectValue *objectValue = value->asObjectValue()) {
        const QString className = objectValue->className();
        if (!className.isEmpty()) {
            return className;
        }
    }

    const QString typeId = context->engine()->typeId(value);
    if (typeId == QLatin1String("undefined")) {
        return QString();
    }

    return typeId;
}

/**
  Returns mapping of every UiObjectMember object to it's direct UiObjectMember parent object.
  */
class ObjectMemberParentVisitor : public AST::Visitor
{
public:
    QHash<AST::UiObjectMember*,AST::UiObjectMember*> operator()(Document::Ptr doc) {
        parent.clear();
        if (doc && doc->ast())
            doc->ast()->accept(this);
        return parent;
    }

private:
    QHash<AST::UiObjectMember*,AST::UiObjectMember*> parent;
    QList<AST::UiObjectMember *> stack;

    bool preVisit(AST::Node *node)
    {
        if (AST::UiObjectMember *objMember = node->uiObjectMemberCast())
            stack.append(objMember);
        return true;
    }

    void postVisit(AST::Node *node)
    {
        if (AST::UiObjectMember *objMember = node->uiObjectMemberCast()) {
            stack.removeLast();
            if (!stack.isEmpty()) {
                parent.insert(objMember, stack.last());
            }
        }
    }
};


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

    typedef QPair<QString,QString> ElementType;
    bool visit(AST::UiObjectDefinition *objDef)
    {
        QModelIndex index = m_model->enterObjectDefinition(objDef);
        m_nodeToIndex.insert(objDef, index);
        return true;
    }

    void endVisit(AST::UiObjectDefinition * /*objDef*/)
    {
        m_model->leaveObjectDefiniton();
    }

    bool visit(AST::UiScriptBinding *scriptBinding)
    {
        QModelIndex index = m_model->enterScriptBinding(scriptBinding);
        m_nodeToIndex.insert(scriptBinding, index);

        return true;
    }

    void endVisit(AST::UiScriptBinding * /*scriptBinding*/)
    {
        m_model->leaveScriptBinding();
    }

    bool visit(AST::UiPublicMember *publicMember)
    {
        QModelIndex index = m_model->enterPublicMember(publicMember);
        m_nodeToIndex.insert(publicMember, index);

        return true;
    }

    void endVisit(AST::UiPublicMember * /*publicMember*/)
    {
        m_model->leavePublicMember();
    }

    QmlOutlineModel *m_model;

    QHash<AST::Node*, QModelIndex> m_nodeToIndex;
    int indent;
};

QmlOutlineModel::QmlOutlineModel(QObject *parent) :
    QStandardItemModel(parent)
{
    m_icons = Icons::instance();
    const QString resourcePath = Core::ICore::instance()->resourcePath();
    QmlJS::Icons::instance()->setIconFilesPath(resourcePath + "/qmlicons");

    // TODO: Maybe add a Copy Action?
    setSupportedDragActions(Qt::MoveAction);
    setItemPrototype(new QmlOutlineItem(this));
}

QStringList QmlOutlineModel::mimeTypes() const
{
    QStringList types;
    types << QLatin1String("application/x-qtcreator-qmloutlinemodel");
    return types;
}


QMimeData *QmlOutlineModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.count() <= 0)
        return 0;
    QStringList types = mimeTypes();
    QMimeData *data = new QMimeData();
    QString format = types.at(0);
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream << indexes.size();

    for (int i = 0; i < indexes.size(); ++i) {
        QModelIndex index = indexes.at(i);

        QList<int> rowPath;
        for (QModelIndex i = index; i.isValid(); i = i.parent()) {
            rowPath.prepend(i.row());
        }

        stream << rowPath;
    }
    data->setData(format, encoded);
    return data;
}

bool QmlOutlineModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int /*column*/, const QModelIndex &parent)
{
    if (debug)
        qDebug() << __FUNCTION__ << row << parent;

    // check if the action is supported
    if (!data || !(action == Qt::CopyAction || action == Qt::MoveAction))
        return false;

    // We cannot reparent outside of the root item
    if (!parent.isValid())
        return false;

    // check if the format is supported
    QStringList types = mimeTypes();
    if (types.isEmpty())
        return false;
    QString format = types.at(0);
    if (!data->hasFormat(format))
        return false;

    // decode and insert
    QByteArray encoded = data->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    int indexSize;
    stream >> indexSize;
    QList<QmlOutlineItem*> itemsToMove;
    for (int i = 0; i < indexSize; ++i) {
        QList<int> rowPath;
        stream >> rowPath;

        QModelIndex index;
        foreach (int row, rowPath) {
            index = this->index(row, 0, index);
            if (!index.isValid())
                continue;
        }

        itemsToMove << static_cast<QmlOutlineItem*>(itemFromIndex(index));
    }

    QmlOutlineItem *targetItem = static_cast<QmlOutlineItem*>(itemFromIndex(parent));
    reparentNodes(targetItem, row, itemsToMove);

    // Prevent view from calling removeRow() on it's own
    return false;
}

QmlJS::Document::Ptr QmlOutlineModel::document() const
{
    return m_semanticInfo.document;
}

void QmlOutlineModel::update(const SemanticInfo &semanticInfo)
{
    m_semanticInfo = semanticInfo;

    m_treePos.clear();
    m_treePos.append(0);
    m_currentItem = invisibleRootItem();

    // Set up lookup context once to do the element type lookup
    //
    // We're simplifying here by using the root context everywhere
    // (empty node list). However, creating the LookupContext is quite expensive (about 3ms),
    // and there is AFAIK no way to introduce new type names in a sub-context.
    m_context = LookupContext::create(semanticInfo.document, semanticInfo.snapshot, QList<AST::Node*>());
    m_typeToIcon.clear();

    QmlOutlineModelSync syncModel(this);
    syncModel(m_semanticInfo.document);

    m_context.clear();

    emit updated();
}

QModelIndex QmlOutlineModel::enterObjectDefinition(AST::UiObjectDefinition *objDef)
{
    QmlOutlineItem prototype(this);

    const QString typeName = asString(objDef->qualifiedTypeNameId);

    if (typeName.at(0).isUpper()) {
        prototype.setText(typeName);
        prototype.setAnnotation(getId(objDef));
        if (!m_typeToIcon.contains(typeName)) {
            m_typeToIcon.insert(typeName, getIcon(objDef));
        }
        prototype.setIcon(m_typeToIcon.value(typeName));
        prototype.setData(ElementType, ItemTypeRole);
        prototype.setIdNode(objDef->qualifiedTypeNameId);
    } else {
        // it's a grouped propery like 'anchors'
        prototype.setText(typeName);
        prototype.setIcon(m_icons->scriptBindingIcon());
        prototype.setData(PropertyType, ItemTypeRole);
    }
    prototype.setSourceLocation(getLocation(objDef));
    prototype.setNode(objDef);

    return enterNode(prototype);
}

void QmlOutlineModel::leaveObjectDefiniton()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterScriptBinding(AST::UiScriptBinding *scriptBinding)
{
    QmlOutlineItem prototype(this);

    prototype.setText(asString(scriptBinding->qualifiedId));
    prototype.setIcon(m_icons->scriptBindingIcon());
    prototype.setData(PropertyType, ItemTypeRole);
    prototype.setSourceLocation(getLocation(scriptBinding));
    prototype.setNode(scriptBinding);
    prototype.setIdNode(scriptBinding->qualifiedId);

    return enterNode(prototype);
}

void QmlOutlineModel::leaveScriptBinding()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterPublicMember(AST::UiPublicMember *publicMember)
{
    QmlOutlineItem prototype(this);

    if (publicMember->name)
        prototype.setText(publicMember->name->asString());
    prototype.setIcon(m_icons->publicMemberIcon());
    prototype.setData(PropertyType, ItemTypeRole);
    prototype.setSourceLocation(getLocation(publicMember));
    prototype.setNode(publicMember);

    return enterNode(prototype);
}

void QmlOutlineModel::leavePublicMember()
{
    leaveNode();
}

QmlJS::AST::Node *QmlOutlineModel::nodeForIndex(const QModelIndex &index)
{
    if (index.isValid()) {
        QmlOutlineItem *item = static_cast<QmlOutlineItem*>(itemFromIndex(index));
        return item->node();
    }
    return 0;
}

QModelIndex QmlOutlineModel::enterNode(const QmlOutlineItem &prototype)
{
    int siblingIndex = m_treePos.last();
    if (siblingIndex == 0) {
        // first child
        if (!m_currentItem->hasChildren()) {
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << m_currentItem->text();

            QmlOutlineItem *newItem = new QmlOutlineItem(this);
            newItem->copyValues(prototype);
            newItem->setEditable(false);
            m_currentItem->appendRow(newItem);

            m_currentItem = newItem;
        } else {
            m_currentItem = m_currentItem->child(0);

            QmlOutlineItem *existingItem = static_cast<QmlOutlineItem*>(m_currentItem);
            existingItem->copyValues(prototype);
        }
    } else {
        // sibling
        if (m_currentItem->rowCount() <= siblingIndex) {
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << m_currentItem->text();

            QmlOutlineItem *newItem = new QmlOutlineItem(this);
            newItem->copyValues(prototype);
            newItem->setEditable(false);
            m_currentItem->appendRow(newItem);
            m_currentItem = newItem;
        } else {
            m_currentItem = m_currentItem->child(siblingIndex);

            QmlOutlineItem *existingItem = static_cast<QmlOutlineItem*>(m_currentItem);
            existingItem->copyValues(prototype);
        }
    }

    m_treePos.append(0);

    return m_currentItem->index();
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

void QmlOutlineModel::reparentNodes(QmlOutlineItem *targetItem, int row, QList<QmlOutlineItem*> itemsToMove)
{
    Utils::ChangeSet changeSet;

    AST::UiObjectMember *targetObjectMember = targetItem->node()->uiObjectMemberCast();
    if (!targetObjectMember)
        return;

    QList<Utils::ChangeSet::Range> changedRanges;

    for (int i = 0; i < itemsToMove.size(); ++i) {
        QmlOutlineItem *outlineItem = itemsToMove.at(i);
        AST::UiObjectMember *sourceObjectMember = outlineItem->node()->uiObjectMemberCast();
        if (!sourceObjectMember)
            return;

        bool insertionOrderSpecified = true;
        AST::UiObjectMember *memberToInsertAfter = 0;
        {
            if (row == -1) {
                insertionOrderSpecified = false;
            } else if (row > 0) {
                QmlOutlineItem *outlineItem = static_cast<QmlOutlineItem*>(targetItem->child(row - 1));
                memberToInsertAfter = outlineItem->node()->uiObjectMemberCast();
            }
        }

        Utils::ChangeSet::Range range;
        if (sourceObjectMember)
            moveObjectMember(sourceObjectMember, targetObjectMember, insertionOrderSpecified,
                             memberToInsertAfter, &changeSet, &range);
        changedRanges << range;
    }

    QmlJSRefactoringChanges refactoring(QmlJS::ModelManagerInterface::instance(), m_semanticInfo.snapshot);
    refactoring.changeFile(m_semanticInfo.document->fileName(), changeSet);
    foreach (const Utils::ChangeSet::Range &range, changedRanges) {
        refactoring.reindent(m_semanticInfo.document->fileName(), range);
    }
    refactoring.apply();
}

void QmlOutlineModel::moveObjectMember(AST::UiObjectMember *toMove,
                                       AST::UiObjectMember *newParent,
                                       bool insertionOrderSpecified,
                                       AST::UiObjectMember *insertAfter,
                                       Utils::ChangeSet *changeSet,
                                       Utils::ChangeSet::Range *addedRange)
{
    Q_ASSERT(toMove);
    Q_ASSERT(newParent);
    Q_ASSERT(changeSet);

    QHash<QmlJS::AST::UiObjectMember*, QmlJS::AST::UiObjectMember*> parentMembers;
    {
        ObjectMemberParentVisitor visitor;
        parentMembers = visitor(m_semanticInfo.document);
    }

    AST::UiObjectMember *oldParent = parentMembers.value(toMove);
    Q_ASSERT(oldParent);

    // make sure that target parent is actually a direct ancestor of target sibling
    if (insertAfter)
        newParent = parentMembers.value(insertAfter);

    const QString documentText = m_semanticInfo.document->source();

    if (AST::UiObjectDefinition *objDefinition = AST::cast<AST::UiObjectDefinition*>(newParent)) {
        // target is an element

        Rewriter rewriter(documentText, changeSet, QStringList());
        rewriter.removeObjectMember(toMove, oldParent);

        AST::UiObjectMemberList *listInsertAfter = 0;
        if (insertionOrderSpecified) {
            if (insertAfter) {
                listInsertAfter = objDefinition->initializer->members;
                while (listInsertAfter && (listInsertAfter->member != insertAfter))
                    listInsertAfter = listInsertAfter->next;
            }
        }

        if (AST::UiScriptBinding *moveScriptBinding = AST::cast<AST::UiScriptBinding*>(toMove)) {
            const QString propertyName = asString(moveScriptBinding->qualifiedId);
            QString propertyValue;
            {
                const int offset = moveScriptBinding->statement->firstSourceLocation().begin();
                const int length = moveScriptBinding->statement->lastSourceLocation().end() - offset;
                propertyValue = documentText.mid(offset, length);
            }
            Rewriter::BindingType bindingType = Rewriter::ScriptBinding;

            if (insertionOrderSpecified) {
                *addedRange = rewriter.addBinding(objDefinition->initializer, propertyName, propertyValue, bindingType, listInsertAfter);
            } else {
                *addedRange = rewriter.addBinding(objDefinition->initializer, propertyName, propertyValue, bindingType);
            }
        } else {
            QString strToMove;
            {
                const int offset = toMove->firstSourceLocation().begin();
                const int length = toMove->lastSourceLocation().end() - offset;
                strToMove = documentText.mid(offset, length);
            }

            if (insertionOrderSpecified) {
                *addedRange = rewriter.addObject(objDefinition->initializer, strToMove, listInsertAfter);
            } else {
                *addedRange = rewriter.addObject(objDefinition->initializer, strToMove);
            }
        }
    } else {
        // target is a property
    }
}

QStandardItem *QmlOutlineModel::parentItem()
{
    QStandardItem *parent = m_currentItem->parent();
    if (!parent)
        parent = invisibleRootItem();
    return parent;
}


QString QmlOutlineModel::asString(AST::UiQualifiedId *id)
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

AST::SourceLocation QmlOutlineModel::getLocation(AST::UiObjectMember *objMember) {
    AST::SourceLocation location;
    location.offset = objMember->firstSourceLocation().offset;
    location.length = objMember->lastSourceLocation().offset
            - objMember->firstSourceLocation().offset
            + objMember->lastSourceLocation().length;
    return location;
}

QIcon QmlOutlineModel::getIcon(AST::UiObjectDefinition *objDef) {
    const QmlJS::Interpreter::Value *value = m_context->evaluate(objDef->qualifiedTypeNameId);

    if (const Interpreter::ObjectValue *objectValue = value->asObjectValue()) {
        do {
            QString module;
            QString typeName;
            if (const Interpreter::QmlObjectValue *qmlObjectValue =
                    dynamic_cast<const Interpreter::QmlObjectValue*>(objectValue)) {
                module = qmlObjectValue->packageName();
            }
            typeName = objectValue->className();

            QIcon icon = m_icons->icon(module, typeName);
            if (! icon.isNull())
                return icon;

            objectValue = objectValue->prototype(m_context->context());
        } while (objectValue);
    }
    return QIcon();
}

QString QmlOutlineModel::getId(AST::UiObjectDefinition *objDef) {
    QString id;
    for (AST::UiObjectMemberList *it = objDef->initializer->members; it; it = it->next) {
        if (AST::UiScriptBinding *binding = AST::cast<AST::UiScriptBinding*>(it->member)) {
            if (binding->qualifiedId->name->asString() == "id") {
                AST::ExpressionStatement *expr = AST::cast<AST::ExpressionStatement*>(binding->statement);
                if (!expr)
                    continue;
                AST::IdentifierExpression *idExpr = AST::cast<AST::IdentifierExpression*>(expr->expression);
                if (!idExpr)
                    continue;
                id = idExpr->name->asString();
                break;
            }
        }
    }
    return id;
}

} // namespace Internal
} // namespace QmlJSEditor
