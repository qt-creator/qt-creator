#ifndef QMLOUTLINEMODEL_H
#define QMLOUTLINEMODEL_H

#include "qmljseditor.h"
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsicons.h>
#include <qmljs/qmljslookupcontext.h>

#include <QStandardItemModel>

namespace QmlJS {
namespace Interpreter {
class Value;
class Context;
}
}

namespace QmlJSEditor {
namespace Internal {

class QmlOutlineModel;

class QmlOutlineItem : public QStandardItem
{
public:
    QmlOutlineItem(QmlOutlineModel *model);

    //QStandardItem
    QVariant data(int role = Qt::UserRole + 1) const;

    QmlOutlineItem &copyValues(const QmlOutlineItem &other); // so that we can assign all values at onc

private:
    QString prettyPrint(const QmlJS::Interpreter::Value *value, QmlJS::Interpreter::Context *context) const;

    QmlOutlineModel *m_outlineModel;
};

class QmlOutlineModel : public QStandardItemModel
{
    Q_OBJECT
public:

    enum CustomRoles {
        SourceLocationRole = Qt::UserRole + 1,
        ItemTypeRole,
        NodePointerRole,
        IdPointerRole
    };

    enum ItemTypes {
        ElementType,
        PropertyType
    };

    QmlOutlineModel(QObject *parent = 0);

    QmlJS::Document::Ptr document() const;
    void update(const SemanticInfo &semanticInfo);

    QModelIndex enterObjectDefinition(QmlJS::AST::UiObjectDefinition *objectDefinition);
    void leaveObjectDefiniton();

    QModelIndex enterScriptBinding(QmlJS::AST::UiScriptBinding *scriptBinding);
    void leaveScriptBinding();

    QModelIndex enterPublicMember(QmlJS::AST::UiPublicMember *publicMember);
    void leavePublicMember();

    QmlJS::AST::Node *nodeForIndex(const QModelIndex &index);

signals:
    void updated();

private:
    QModelIndex enterNode(const QmlOutlineItem &prototype);
    void leaveNode();

    QStandardItem *parentItem();

    static QString asString(QmlJS::AST::UiQualifiedId *id);
    static QmlJS::AST::SourceLocation getLocation(QmlJS::AST::UiObjectMember *objMember);
    QIcon getIcon(QmlJS::AST::UiObjectDefinition *objDef);
    static QString getId(QmlJS::AST::UiObjectDefinition *objDef);


    SemanticInfo m_semanticInfo;
    QList<int> m_treePos;
    QStandardItem *m_currentItem;
    QmlJS::Icons *m_icons;

    QmlJS::LookupContext::Ptr m_context;
    QHash<QString, QIcon> m_typeToIcon;

    friend class QmlOutlineModelSync;
    friend class QmlOutlineItem;
};

} // namespace Internal
} // namespace QmlJSEditor

Q_DECLARE_METATYPE(QmlJS::AST::SourceLocation);
Q_DECLARE_METATYPE(QmlJS::AST::Node*);
Q_DECLARE_METATYPE(QmlJS::AST::UiQualifiedId*);

#endif // QMLOUTLINEMODEL_H
