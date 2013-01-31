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

#include "qmloutlinemodel.h"
#include "qmljseditor.h"

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsrewriter.h>
#include <qmljstools/qmljsrefactoringchanges.h>

#include <utils/qtcassert.h>

#include <coreplugin/icore.h>
#include <QDebug>
#include <QTime>
#include <QMimeData>
#include <typeinfo>

using namespace QmlJS;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;

enum {
    debug = false
};

namespace QmlJSEditor {
namespace Internal {

QmlOutlineItem::QmlOutlineItem(QmlOutlineModel *model) :
    m_outlineModel(model)
{
}

QVariant QmlOutlineItem::data(int role) const
{
    if (role == Qt::ToolTipRole) {
        AST::SourceLocation location = m_outlineModel->sourceLocation(index());
        AST::UiQualifiedId *uiQualifiedId = m_outlineModel->idNode(index());
        if (!uiQualifiedId || !location.isValid() || !m_outlineModel->m_semanticInfo.isValid())
            return QVariant();

        QList<AST::Node *> astPath = m_outlineModel->m_semanticInfo.rangePath(location.begin());
        ScopeChain scopeChain = m_outlineModel->m_semanticInfo.scopeChain(astPath);
        const Value *value = scopeChain.evaluate(uiQualifiedId);

        return prettyPrint(value, scopeChain.context());
    }

    if (role == Qt::DecorationRole)
        return m_outlineModel->icon(index());

    return QStandardItem::data(role);
}

int QmlOutlineItem::type() const
{
    return UserType;
}

void QmlOutlineItem::setItemData(const QMap<int, QVariant> &roles)
{
    QMap<int,QVariant>::const_iterator iter(roles.constBegin());
    while (iter != roles.constEnd()) {
        setData(iter.value(), iter.key());
        iter++;
    }
}

QString QmlOutlineItem::prettyPrint(const Value *value, const ContextPtr &context) const
{
    if (! value)
        return QString();

    if (const ObjectValue *objectValue = value->asObjectValue()) {
        const QString className = objectValue->className();
        if (!className.isEmpty())
            return className;
    }

    const QString typeId = context->valueOwner()->typeId(value);
    if (typeId == QLatin1String("undefined"))
        return QString();

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
            if (!stack.isEmpty())
                parent.insert(objMember, stack.last());
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

    bool visit(AST::UiObjectBinding *objBinding)
    {
        QModelIndex index = m_model->enterObjectBinding(objBinding);
        m_nodeToIndex.insert(objBinding, index);
        return true;
    }

    void endVisit(AST::UiObjectBinding * /*objBinding*/)
    {
        m_model->leaveObjectBinding();
    }

    bool visit(AST::UiArrayBinding *arrayBinding)
    {
        QModelIndex index = m_model->enterArrayBinding(arrayBinding);
        m_nodeToIndex.insert(arrayBinding, index);

        return true;
    }

    void endVisit(AST::UiArrayBinding * /*arrayBinding*/)
    {
        m_model->leaveArrayBinding();
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

    bool visit(AST::FunctionDeclaration *functionDeclaration)
    {
        QModelIndex index = m_model->enterFunctionDeclaration(functionDeclaration);
        m_nodeToIndex.insert(functionDeclaration, index);

        return true;
    }

    void endVisit(AST::FunctionDeclaration * /*functionDeclaration*/)
    {
        m_model->leaveFunctionDeclaration();
    }

    bool visit(AST::BinaryExpression *binExp)
    {
        AST::IdentifierExpression *lhsIdent = AST::cast<AST::IdentifierExpression *>(binExp->left);
        AST::ObjectLiteral *rhsObjLit = AST::cast<AST::ObjectLiteral *>(binExp->right);

        if (lhsIdent && rhsObjLit && (lhsIdent->name == QLatin1String("testcase"))
            && (binExp->op == QSOperator::Assign)) {
            QModelIndex index = m_model->enterTestCase(rhsObjLit);
            m_nodeToIndex.insert(rhsObjLit, index);

            if (AST::PropertyNameAndValueList *properties = rhsObjLit->properties)
                visitProperties(properties);

            m_model->leaveTestCase();
        }
        return true;
    }

    void visitProperties(AST::PropertyNameAndValueList *properties)
    {
        while (properties) {
            QModelIndex index = m_model->enterTestCaseProperties(properties);
            m_nodeToIndex.insert(properties, index);

            if (AST::ObjectLiteral *objLiteral = AST::cast<AST::ObjectLiteral *>(properties->value))
                visitProperties(objLiteral->properties);

            m_model->leaveTestCaseProperties();
            properties = properties->next;
        }
    }

    QmlOutlineModel *m_model;

    QHash<AST::Node*, QModelIndex> m_nodeToIndex;
    int indent;
};

QmlOutlineModel::QmlOutlineModel(QmlJSTextEditorWidget *editor) :
    QStandardItemModel(editor),
    m_textEditor(editor)
{
    m_icons = Icons::instance();
    const QString resourcePath = Core::ICore::resourcePath();
    Icons::instance()->setIconFilesPath(resourcePath + QLatin1String("/qmlicons"));

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

Qt::ItemFlags QmlOutlineModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return QStandardItemModel::flags(index);

    Qt::ItemFlags flags = Qt::ItemIsSelectable|Qt::ItemIsEnabled;

    // only allow drag&drop if we're in sync
    if (m_semanticInfo.isValid()
            && !m_textEditor->isSemanticInfoOutdated()) {
        if (index.parent().isValid())
            flags |= Qt::ItemIsDragEnabled;
        if (index.data(ItemTypeRole) != NonElementBindingType)
            flags |= Qt::ItemIsDropEnabled;
    }
    return flags;
}


Document::Ptr QmlOutlineModel::document() const
{
    return m_semanticInfo.document;
}

void QmlOutlineModel::update(const SemanticInfo &semanticInfo)
{
    m_semanticInfo = semanticInfo;
    if (! m_semanticInfo.isValid())
        return;

    m_treePos.clear();
    m_treePos.append(0);
    m_currentItem = invisibleRootItem();

    // resetModel() actually gives better average performance
    // then the incremental updates.
    beginResetModel();

    m_typeToIcon.clear();
    m_itemToNode.clear();
    m_itemToIdNode.clear();
    m_itemToIcon.clear();


    QmlOutlineModelSync syncModel(this);
    syncModel(m_semanticInfo.document);

    endResetModel();

    emit updated();
}

QModelIndex QmlOutlineModel::enterObjectDefinition(AST::UiObjectDefinition *objDef)
{
    const QString typeName = asString(objDef->qualifiedTypeNameId);

    QMap<int, QVariant> data;
    AST::UiQualifiedId *idNode = 0;
    QIcon icon;

    data.insert(Qt::DisplayRole, typeName);

    if (typeName.at(0).isUpper()) {
        data.insert(ItemTypeRole, ElementType);
        data.insert(AnnotationRole, getAnnotation(objDef->initializer));
        idNode = objDef->qualifiedTypeNameId;
        if (!m_typeToIcon.contains(typeName))
            m_typeToIcon.insert(typeName, getIcon(objDef->qualifiedTypeNameId));
        icon = m_typeToIcon.value(typeName);
    } else {
        // it's a grouped propery like 'anchors'
        data.insert(ItemTypeRole, NonElementBindingType);
        icon = m_icons->scriptBindingIcon();
    }

    QmlOutlineItem *item = enterNode(data, objDef, idNode, icon);

    return item->index();
}

void QmlOutlineModel::leaveObjectDefiniton()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterObjectBinding(AST::UiObjectBinding *objBinding)
{
    QMap<int, QVariant> bindingData;

    bindingData.insert(Qt::DisplayRole, asString(objBinding->qualifiedId));
    bindingData.insert(ItemTypeRole, ElementBindingType);

    QmlOutlineItem *bindingItem = enterNode(bindingData, objBinding, objBinding->qualifiedId, m_icons->scriptBindingIcon());

    const QString typeName = asString(objBinding->qualifiedTypeNameId);
    if (!m_typeToIcon.contains(typeName))
        m_typeToIcon.insert(typeName, getIcon(objBinding->qualifiedTypeNameId));

    QMap<int, QVariant> objectData;
    objectData.insert(Qt::DisplayRole, typeName);
    objectData.insert(AnnotationRole, getAnnotation(objBinding->initializer));
    objectData.insert(ItemTypeRole, ElementType);

    enterNode(objectData, objBinding, objBinding->qualifiedTypeNameId, m_typeToIcon.value(typeName));

    return bindingItem->index();
}

void QmlOutlineModel::leaveObjectBinding()
{
    leaveNode();
    leaveNode();
}

QModelIndex QmlOutlineModel::enterArrayBinding(AST::UiArrayBinding *arrayBinding)
{
    QMap<int, QVariant> bindingData;

    bindingData.insert(Qt::DisplayRole, asString(arrayBinding->qualifiedId));
    bindingData.insert(ItemTypeRole, ElementBindingType);

    QmlOutlineItem *item = enterNode(bindingData, arrayBinding, arrayBinding->qualifiedId, m_icons->scriptBindingIcon());

    return item->index();
}

void QmlOutlineModel::leaveArrayBinding()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterScriptBinding(AST::UiScriptBinding *scriptBinding)
{
    QMap<int, QVariant> objectData;

    objectData.insert(Qt::DisplayRole, asString(scriptBinding->qualifiedId));
    objectData.insert(AnnotationRole, getAnnotation(scriptBinding->statement));
    objectData.insert(ItemTypeRole, NonElementBindingType);

    QmlOutlineItem *item = enterNode(objectData, scriptBinding, scriptBinding->qualifiedId, m_icons->scriptBindingIcon());

    return item->index();
}

void QmlOutlineModel::leaveScriptBinding()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterPublicMember(AST::UiPublicMember *publicMember)
{
    QMap<int, QVariant> objectData;

    if (!publicMember->name.isEmpty())
        objectData.insert(Qt::DisplayRole, publicMember->name.toString());
    objectData.insert(AnnotationRole, getAnnotation(publicMember->statement));
    objectData.insert(ItemTypeRole, NonElementBindingType);

    QmlOutlineItem *item = enterNode(objectData, publicMember, 0, m_icons->publicMemberIcon());

    return item->index();
}

void QmlOutlineModel::leavePublicMember()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterFunctionDeclaration(AST::FunctionDeclaration *functionDeclaration)
{
    QMap<int, QVariant> objectData;

    if (!functionDeclaration->name.isEmpty())
        objectData.insert(Qt::DisplayRole, functionDeclaration->name.toString());
    objectData.insert(ItemTypeRole, ElementBindingType);

    QmlOutlineItem *item = enterNode(objectData, functionDeclaration, 0, m_icons->functionDeclarationIcon());

    return item->index();
}

void QmlOutlineModel::leaveFunctionDeclaration()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterTestCase(AST::ObjectLiteral *objectLiteral)
{
    QMap<int, QVariant> objectData;

    objectData.insert(Qt::DisplayRole, QLatin1String("testcase"));
    objectData.insert(ItemTypeRole, ElementBindingType);

    QmlOutlineItem *item = enterNode(objectData, objectLiteral, 0, m_icons->objectDefinitionIcon());

    return item->index();
}

void QmlOutlineModel::leaveTestCase()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterTestCaseProperties(AST::PropertyNameAndValueList *propertyNameAndValueList)
{
    QMap<int, QVariant> objectData;
    if (AST::IdentifierPropertyName *propertyName = AST::cast<AST::IdentifierPropertyName *>(propertyNameAndValueList->name)) {
        objectData.insert(Qt::DisplayRole, propertyName->id.toString());
        objectData.insert(ItemTypeRole, ElementBindingType);
        QmlOutlineItem *item;
        if (propertyNameAndValueList->value->kind == AST::Node::Kind_FunctionExpression)
            item = enterNode(objectData, propertyNameAndValueList, 0, m_icons->functionDeclarationIcon());
        else if (propertyNameAndValueList->value->kind == AST::Node::Kind_ObjectLiteral)
            item = enterNode(objectData, propertyNameAndValueList, 0, m_icons->objectDefinitionIcon());
        else
            item = enterNode(objectData, propertyNameAndValueList, 0, m_icons->scriptBindingIcon());

        return item->index();
    } else {
        return QModelIndex();
    }
}

void QmlOutlineModel::leaveTestCaseProperties()
{
    leaveNode();
}

AST::Node *QmlOutlineModel::nodeForIndex(const QModelIndex &index) const
{
    QTC_ASSERT(index.isValid() && (index.model() == this), return 0);
    if (index.isValid()) {
        QmlOutlineItem *item = static_cast<QmlOutlineItem*>(itemFromIndex(index));
        QTC_ASSERT(item, return 0);
        QTC_ASSERT(m_itemToNode.contains(item), return 0);
        return m_itemToNode.value(item);
    }
    return 0;
}

AST::SourceLocation QmlOutlineModel::sourceLocation(const QModelIndex &index) const
{
    AST::SourceLocation location;
    QTC_ASSERT(index.isValid() && (index.model() == this), return location);
    AST::Node *node = nodeForIndex(index);
    if (node) {
        if (AST::UiObjectMember *member = node->uiObjectMemberCast())
            location = getLocation(member);
        else if (AST::ExpressionNode *expression = node->expressionCast())
            location = getLocation(expression);
        else if (AST::PropertyNameAndValueList *propertyNameAndValueList = AST::cast<AST::PropertyNameAndValueList *>(node))
            location = getLocation(propertyNameAndValueList);
    }
    return location;
}

AST::UiQualifiedId *QmlOutlineModel::idNode(const QModelIndex &index) const
{
    QTC_ASSERT(index.isValid() && (index.model() == this), return 0);
    QmlOutlineItem *item = static_cast<QmlOutlineItem*>(itemFromIndex(index));
    return m_itemToIdNode.value(item);
}

QIcon QmlOutlineModel::icon(const QModelIndex &index) const
{
    QTC_ASSERT(index.isValid() && (index.model() == this), return QIcon());
    QmlOutlineItem *item = static_cast<QmlOutlineItem*>(itemFromIndex(index));
    return m_itemToIcon.value(item);
}

QmlOutlineItem *QmlOutlineModel::enterNode(QMap<int, QVariant> data, AST::Node *node, AST::UiQualifiedId *idNode, const QIcon &icon)
{
    int siblingIndex = m_treePos.last();
    QmlOutlineItem *newItem = 0;
    if (siblingIndex == 0) {
        // first child
        if (!m_currentItem->hasChildren()) {
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << m_currentItem->text();

            newItem = new QmlOutlineItem(this);
        } else {
            m_currentItem = m_currentItem->child(0);
        }
    } else {
        // sibling
        if (m_currentItem->rowCount() <= siblingIndex) {
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << m_currentItem->text();

            newItem = new QmlOutlineItem(this);
        } else {
            m_currentItem = m_currentItem->child(siblingIndex);
        }
    }

    QmlOutlineItem *item = newItem ? newItem : static_cast<QmlOutlineItem*>(m_currentItem);
    m_itemToNode.insert(item, node);
    m_itemToIdNode.insert(item, idNode);
    m_itemToIcon.insert(item, icon);

    if (newItem) {
        m_currentItem->appendRow(newItem);
        m_currentItem = newItem;
    }

    setItemData(m_currentItem->index(), data);

    m_treePos.append(0);

    return item;
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

    AST::UiObjectMember *targetObjectMember = m_itemToNode.value(targetItem)->uiObjectMemberCast();
    if (!targetObjectMember)
        return;

    QList<Utils::ChangeSet::Range> changedRanges;

    for (int i = 0; i < itemsToMove.size(); ++i) {
        QmlOutlineItem *outlineItem = itemsToMove.at(i);
        AST::UiObjectMember *sourceObjectMember = m_itemToNode.value(outlineItem)->uiObjectMemberCast();
        if (!sourceObjectMember)
            return;

        bool insertionOrderSpecified = true;
        AST::UiObjectMember *memberToInsertAfter = 0;
        {
            if (row == -1) {
                insertionOrderSpecified = false;
            } else if (row > 0) {
                QmlOutlineItem *outlineItem = static_cast<QmlOutlineItem*>(targetItem->child(row - 1));
                memberToInsertAfter = m_itemToNode.value(outlineItem)->uiObjectMemberCast();
            }
        }

        Utils::ChangeSet::Range range;
        if (sourceObjectMember)
            moveObjectMember(sourceObjectMember, targetObjectMember, insertionOrderSpecified,
                             memberToInsertAfter, &changeSet, &range);
        changedRanges << range;
    }

    QmlJSRefactoringChanges refactoring(ModelManagerInterface::instance(), m_semanticInfo.snapshot);
    TextEditor::RefactoringFilePtr file = refactoring.file(m_semanticInfo.document->fileName());
    file->setChangeSet(changeSet);
    foreach (const Utils::ChangeSet::Range &range, changedRanges) {
        file->appendIndentRange(range);
    }
    file->apply();
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

    QHash<AST::UiObjectMember*, AST::UiObjectMember*> parentMembers;
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

    Rewriter rewriter(documentText, changeSet, QStringList());

    if (AST::UiObjectDefinition *objDefinition = AST::cast<AST::UiObjectDefinition*>(newParent)) {
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

            if (insertionOrderSpecified)
                *addedRange = rewriter.addBinding(objDefinition->initializer, propertyName, propertyValue, bindingType, listInsertAfter);
            else
                *addedRange = rewriter.addBinding(objDefinition->initializer, propertyName, propertyValue, bindingType);
        } else {
            QString strToMove;
            {
                const int offset = toMove->firstSourceLocation().begin();
                const int length = toMove->lastSourceLocation().end() - offset;
                strToMove = documentText.mid(offset, length);
            }

            if (insertionOrderSpecified)
                *addedRange = rewriter.addObject(objDefinition->initializer, strToMove, listInsertAfter);
            else
                *addedRange = rewriter.addObject(objDefinition->initializer, strToMove);
        }
    } else if (AST::UiArrayBinding *arrayBinding = AST::cast<AST::UiArrayBinding*>(newParent)) {
        AST::UiArrayMemberList *listInsertAfter = 0;
        if (insertionOrderSpecified) {
            if (insertAfter) {
                listInsertAfter = arrayBinding->members;
                while (listInsertAfter && (listInsertAfter->member != insertAfter))
                    listInsertAfter = listInsertAfter->next;
            }
        }
        QString strToMove;
        {
            const int offset = toMove->firstSourceLocation().begin();
            const int length = toMove->lastSourceLocation().end() - offset;
            strToMove = documentText.mid(offset, length);
        }

        if (insertionOrderSpecified)
            *addedRange = rewriter.addObject(arrayBinding, strToMove, listInsertAfter);
        else
            *addedRange = rewriter.addObject(arrayBinding, strToMove);
    } else if (AST::cast<AST::UiObjectBinding*>(newParent)) {
        qDebug() << "TODO: Reparent to UiObjectBinding";
        return;
        // target is a property
    } else {
        return;
    }

    rewriter.removeObjectMember(toMove, oldParent);
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
        if (!id->name.isEmpty())
            text += id->name;
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

AST::SourceLocation QmlOutlineModel::getLocation(AST::ExpressionNode *exprNode) {
    AST::SourceLocation location;
    location.offset = exprNode->firstSourceLocation().offset;
    location.length = exprNode->lastSourceLocation().offset
            - exprNode->firstSourceLocation().offset
            + exprNode->lastSourceLocation().length;
    return location;
}

AST::SourceLocation QmlOutlineModel::getLocation(AST::PropertyNameAndValueList *propertyNode) {
    AST::SourceLocation location;
    location.offset = propertyNode->name->propertyNameToken.offset;
    location.length = propertyNode->value->lastSourceLocation().end() - location.offset;

    return location;
}

QIcon QmlOutlineModel::getIcon(AST::UiQualifiedId *qualifiedId) {
    QIcon icon;
    if (qualifiedId) {
        QString name = asString(qualifiedId);
        if (name.contains(QLatin1Char('.')))
            name = name.split(QLatin1Char('.')).last();

        // TODO: get rid of namespace prefixes.
        icon = m_icons->icon(QLatin1String("Qt"), name);
        if (icon.isNull())
            icon = m_icons->icon(QLatin1String("QtWebkit"), name);
    }
    return icon;
}

QString QmlOutlineModel::getAnnotation(AST::UiObjectInitializer *objectInitializer) {
    const QHash<QString,QString> bindings = getScriptBindings(objectInitializer);

    if (bindings.contains(QLatin1String("id")))
        return bindings.value(QLatin1String("id"));

    if (bindings.contains(QLatin1String("name")))
        return bindings.value(QLatin1String("name"));

    if (bindings.contains(QLatin1String("target")))
        return bindings.value(QLatin1String("target"));

    return QString();
}

QString QmlOutlineModel::getAnnotation(AST::Statement *statement)
{
    if (AST::ExpressionStatement *expr = AST::cast<AST::ExpressionStatement*>(statement))
        return getAnnotation(expr->expression);
    return QString();
}

QString QmlOutlineModel::getAnnotation(AST::ExpressionNode *expression)
{
    if (!expression)
        return QString();
    QString source = m_semanticInfo.document->source();
    return source.mid(expression->firstSourceLocation().begin(),
                      expression->lastSourceLocation().end() - expression->firstSourceLocation().begin());
}

QHash<QString,QString> QmlOutlineModel::getScriptBindings(AST::UiObjectInitializer *objectInitializer) {
    QHash <QString,QString> scriptBindings;
    for (AST::UiObjectMemberList *it = objectInitializer->members; it; it = it->next) {
        if (AST::UiScriptBinding *binding = AST::cast<AST::UiScriptBinding*>(it->member)) {
            const QString bindingName = asString(binding->qualifiedId);
            scriptBindings.insert(bindingName, getAnnotation(binding->statement));
        }
    }
    return scriptBindings;
}

} // namespace Internal
} // namespace QmlJSEditor
