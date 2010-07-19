#include "qmloutlinemodel.h"
#include "qmljseditor.h"
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljslookupcontext.h>

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
    m_outlineModel(model)
{
}

QVariant QmlOutlineItem::data(int role) const
{
    if (role == Qt::ToolTipRole) {
        AST::SourceLocation location = data(QmlOutlineModel::SourceLocationRole).value<AST::SourceLocation>();
        AST::UiQualifiedId *uiQualifiedId = data(QmlOutlineModel::IdPointerRole).value<AST::UiQualifiedId*>();
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

QmlOutlineItem &QmlOutlineItem::copyValues(const QmlOutlineItem &other)
{
    *this = other;
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
    const QString id = getId(objDef);
    if (!id.isEmpty()) {
        prototype.setText(id);
    } else {
        prototype.setText(typeName);
    }
    if (!m_typeToIcon.contains(typeName)) {
        m_typeToIcon.insert(typeName, getIcon(objDef));
    }
    prototype.setIcon(m_typeToIcon.value(typeName));
    prototype.setData(ElementType, ItemTypeRole);
    prototype.setData(QVariant::fromValue(getLocation(objDef)), SourceLocationRole);
    prototype.setData(QVariant::fromValue(static_cast<AST::Node*>(objDef)), NodePointerRole);
    prototype.setData(QVariant::fromValue(static_cast<AST::UiQualifiedId*>(objDef->qualifiedTypeNameId)), IdPointerRole);

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
    prototype.setData(QVariant::fromValue(getLocation(scriptBinding)), SourceLocationRole);
    prototype.setData(QVariant::fromValue(static_cast<AST::Node*>(scriptBinding)), NodePointerRole);
    prototype.setData(QVariant::fromValue(static_cast<AST::UiQualifiedId*>(scriptBinding->qualifiedId)), IdPointerRole);

    return enterNode(prototype);
}

void QmlOutlineModel::leaveScriptBinding()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterPublicMember(AST::UiPublicMember *publicMember)
{
    QmlOutlineItem prototype(this);

    prototype.setText(publicMember->name->asString());
    prototype.setIcon(m_icons->publicMemberIcon());
    prototype.setData(PropertyType, ItemTypeRole);
    prototype.setData(QVariant::fromValue(getLocation(publicMember)), SourceLocationRole);
    prototype.setData(QVariant::fromValue(static_cast<AST::Node*>(publicMember)), NodePointerRole);

    return enterNode(prototype);
}

void QmlOutlineModel::leavePublicMember()
{
    leaveNode();
}

QmlJS::AST::Node *QmlOutlineModel::nodeForIndex(const QModelIndex &index)
{
    if (index.isValid()) {
        return index.data(NodePointerRole).value<QmlJS::AST::Node*>();
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
        if (AST::UiScriptBinding *binding = dynamic_cast<AST::UiScriptBinding*>(it->member)) {
            if (binding->qualifiedId->name->asString() == "id") {
                AST::ExpressionStatement *expr = dynamic_cast<AST::ExpressionStatement*>(binding->statement);
                if (!expr)
                    continue;
                AST::IdentifierExpression *idExpr = dynamic_cast<AST::IdentifierExpression*>(expr->expression);
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
