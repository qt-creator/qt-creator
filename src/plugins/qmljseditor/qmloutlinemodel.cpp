#include "qmloutlinemodel.h"
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

QmlOutlineItem &QmlOutlineItem::copyValues(const QmlOutlineItem &other)
{
    *this = other;
    emitDataChanged();
    return *this;
}

class QmlOutlineModelSync : protected AST::Visitor
{
public:
    QmlOutlineModelSync(QmlOutlineModel *model) :
        m_model(model),
        indent(0)
    {
    }

    void operator()(Document::Ptr doc, const Snapshot &snapshot)
    {
        m_nodeToIndex.clear();

        // Set up lookup context once to do the element type lookup
        //
        // We're simplifying here by using the root context everywhere
        // (empty node list). However, creating the LookupContext is quite expensive (about 3ms),
        // and there is AFAIK no way to introduce new type names in a sub-context.
        m_context = LookupContext::create(doc, snapshot, QList<AST::Node*>());

        if (debug)
            qDebug() << "QmlOutlineModel ------";
        if (doc && doc->ast())
            doc->ast()->accept(this);

        m_context.clear();
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


    typedef QPair<QString,QString> ElementType;
    bool visit(AST::UiObjectDefinition *objDef)
    {
        AST::SourceLocation location = getLocation(objDef);

        const QString typeName = asString(objDef->qualifiedTypeNameId);

        if (!m_typeToIcon.contains(typeName)) {
            m_typeToIcon.insert(typeName, getIcon(objDef));
        }
        const QIcon icon = m_typeToIcon.value(typeName);
        QString id = getId(objDef);

        QModelIndex index = m_model->enterElement(typeName, id, icon, location);
        m_nodeToIndex.insert(objDef, index);
        return true;
    }

    void endVisit(AST::UiObjectDefinition * /*objDef*/)
    {
        m_model->leaveElement();
    }

    bool visit(AST::UiScriptBinding *scriptBinding)
    {
        AST::SourceLocation location = getLocation(scriptBinding);

        QModelIndex index = m_model->enterProperty(asString(scriptBinding->qualifiedId), false, location);
        m_nodeToIndex.insert(scriptBinding, index);

        return true;
    }

    void endVisit(AST::UiScriptBinding * /*scriptBinding*/)
    {
        m_model->leaveProperty();
    }

    bool visit(AST::UiPublicMember *publicMember)
    {
        AST::SourceLocation location = getLocation(publicMember);
        QModelIndex index = m_model->enterProperty(publicMember->name->asString(), true, location);
        m_nodeToIndex.insert(publicMember, index);

        return true;
    }

    void endVisit(AST::UiPublicMember * /*publicMember*/)
    {
        m_model->leaveProperty();
    }

    QIcon getIcon(AST::UiObjectDefinition *objDef) {
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

                QIcon icon = m_model->m_icons->icon(module, typeName);
                if (! icon.isNull())
                    return icon;

                objectValue = objectValue->prototype(m_context->context());
            } while (objectValue);
        }
        return QIcon();
    }

    QString getId(AST::UiObjectDefinition *objDef) {
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

    AST::SourceLocation getLocation(AST::UiObjectMember *objMember) {
        AST::SourceLocation location;
        location.offset = objMember->firstSourceLocation().offset;
        location.length = objMember->lastSourceLocation().offset
                - objMember->firstSourceLocation().offset
                + objMember->lastSourceLocation().length;
        return location;
    }

    QmlOutlineModel *m_model;
    LookupContext::Ptr m_context;

    QHash<AST::Node*, QModelIndex> m_nodeToIndex;
    QHash<QString, QIcon> m_typeToIcon;
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
    return m_document;
}

void QmlOutlineModel::update(QmlJS::Document::Ptr doc, const QmlJS::Snapshot &snapshot)
{
    m_document = doc;

    m_treePos.clear();
    m_treePos.append(0);
    m_currentItem = invisibleRootItem();

    QmlOutlineModelSync syncModel(this);
    syncModel(doc, snapshot);

    emit updated();
}

QModelIndex QmlOutlineModel::enterElement(const QString &type, const QString &id, const QIcon &icon, const AST::SourceLocation &sourceLocation)
{
    QmlOutlineItem prototype;

    if (!id.isEmpty()) {
        prototype.setText(id);
    } else {
        prototype.setText(type);
    }
    prototype.setIcon(icon);
    prototype.setToolTip(type);
    prototype.setData(ElementType, ItemTypeRole);
    prototype.setData(QVariant::fromValue(sourceLocation), SourceLocationRole);

    return enterNode(prototype);
}

void QmlOutlineModel::leaveElement()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterProperty(const QString &name, bool isCustomProperty, const AST::SourceLocation &sourceLocation)
{
    QmlOutlineItem prototype;

    prototype.setText(name);
    if (isCustomProperty) {
        prototype.setIcon(m_icons->publicMemberIcon());
    } else {
        prototype.setIcon(m_icons->scriptBindingIcon());
    }
    prototype.setData(PropertyType, ItemTypeRole);
    prototype.setData(QVariant::fromValue(sourceLocation), SourceLocationRole);

    return enterNode(prototype);
}

void QmlOutlineModel::leaveProperty()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterNode(const QmlOutlineItem &prototype)
{
    int siblingIndex = m_treePos.last();
    if (siblingIndex == 0) {
        // first child
        if (!m_currentItem->hasChildren()) {
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << m_currentItem->text();

            QmlOutlineItem *newItem = new QmlOutlineItem;
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

            QmlOutlineItem *newItem = new QmlOutlineItem;
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

} // namespace Internal
} // namespace QmlJSEditor
