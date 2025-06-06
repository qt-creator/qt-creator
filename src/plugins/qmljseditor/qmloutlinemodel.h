// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljseditordocument.h"
#include <utils/changeset.h>
#include <qmljs/qmljsdocument.h>

#include <QStandardItemModel>

namespace QmlJS {
class Value;
class Context;
}

namespace QmlJSEditor {
namespace Internal {

class QmlOutlineModel;

class QmlOutlineItem : public QStandardItem
{
public:
    QmlOutlineItem(QmlOutlineModel *model);

    // QStandardItem
    QVariant data(int role = Qt::UserRole + 1) const override;
    int type() const override;

    void setItemData(const QMap<int, QVariant> &roles);

private:
    QString prettyPrint(const QmlJS::Value *value, const QmlJS::ContextPtr &context) const;

    QmlOutlineModel *m_outlineModel;
};

class QmlOutlineModel : public QStandardItemModel
{
    Q_OBJECT
public:

    enum CustomRoles {
        ItemTypeRole = Qt::UserRole + 1,
        ElementTypeRole,
        AnnotationRole
    };

    enum ItemTypes {
        ElementType,
        ElementBindingType, // might contain elements as children
        NonElementBindingType // can be filtered out
    };

    QmlOutlineModel(QmlJSEditorDocument *document);

    // QStandardItemModel
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QmlJS::Document::Ptr document() const;
    void update(const QmlJSTools::SemanticInfo &semanticInfo);

    QmlJS::AST::Node *nodeForIndex(const QModelIndex &index) const;
    QmlJS::SourceLocation sourceLocation(const QModelIndex &index) const;
    QmlJS::AST::UiQualifiedId *idNode(const QModelIndex &index) const;
    QIcon icon(const QModelIndex &index) const;

signals:
    void updated();

private:
    QModelIndex enterObjectDefinition(QmlJS::AST::UiObjectDefinition *objectDefinition);
    void leaveObjectDefiniton();

    QModelIndex enterObjectBinding(QmlJS::AST::UiObjectBinding *objectBinding);
    void leaveObjectBinding();

    QModelIndex enterArrayBinding(QmlJS::AST::UiArrayBinding *arrayBinding);
    void leaveArrayBinding();

    QModelIndex enterScriptBinding(QmlJS::AST::UiScriptBinding *scriptBinding);
    void leaveScriptBinding();

    QModelIndex enterPublicMember(QmlJS::AST::UiPublicMember *publicMember);
    void leavePublicMember();

    QModelIndex enterEnumDeclaration(QmlJS::AST::UiEnumDeclaration *enumDecl);
    void leaveEnumDeclaration();

    QModelIndex enterFunctionDeclaration(QmlJS::AST::FunctionDeclaration *functionDeclaration);
    void leaveFunctionDeclaration();

    QModelIndex enterFieldMemberExpression(QmlJS::AST::FieldMemberExpression *expression,
                                           QmlJS::AST::FunctionExpression *functionExpression);
    void leaveFieldMemberExpression();

    QModelIndex enterTestCase(QmlJS::AST::ObjectPattern *objectLiteral);
    void leaveTestCase();

    QModelIndex enterTestCaseProperties(QmlJS::AST::PatternPropertyList *propertyAssignmentList);
    void leaveTestCaseProperties();

private:
    QmlOutlineItem *enterNode(QMap<int, QVariant> data, QmlJS::AST::Node *node, QmlJS::AST::UiQualifiedId *idNode, const QIcon &icon);
    void leaveNode();

    void reparentNodes(QmlOutlineItem *targetItem, int targetRow, QList<QmlOutlineItem*> itemsToMove);
    void moveObjectMember(QmlJS::AST::Node *toMove, QmlJS::AST::UiObjectMember *newParent,
                          bool insertionOrderSpecified, QmlJS::AST::UiObjectMember *insertAfter,
                          Utils::ChangeSet *changeSet, Utils::ChangeSet::Range *addedRange);

    QStandardItem *parentItem();

    static QString asString(QmlJS::AST::UiQualifiedId *id);
    static QmlJS::SourceLocation getLocation(QmlJS::AST::UiObjectMember *objMember);
    static QmlJS::SourceLocation getLocation(QmlJS::AST::ExpressionNode *exprNode);
    static QmlJS::SourceLocation getLocation(QmlJS::AST::PatternProperty *propertyNode);
    static QmlJS::SourceLocation getLocation(QmlJS::AST::PatternPropertyList *propertyNode);
    QIcon getIcon(QmlJS::AST::UiQualifiedId *objDef);

    QString getAnnotation(QmlJS::AST::UiObjectInitializer *objInitializer);
    QString getAnnotation(QmlJS::AST::Statement *statement);
    QString getAnnotation(QmlJS::AST::ExpressionNode *expression);
    QHash<QString,QString> getScriptBindings(QmlJS::AST::UiObjectInitializer *objInitializer);


    QmlJSTools::SemanticInfo m_semanticInfo;
    QList<int> m_treePos;
    QStandardItem *m_currentItem;

    QHash<QString, QIcon> m_typeToIcon;
    QHash<QmlOutlineItem*,QIcon> m_itemToIcon;
    QHash<QmlOutlineItem*,QmlJS::AST::Node*> m_itemToNode;
    QHash<QmlOutlineItem*,QmlJS::AST::UiQualifiedId*> m_itemToIdNode;
    QmlJSEditorDocument *m_editorDocument;


    friend class QmlOutlineModelSync;
    friend class QmlOutlineItem;
};

} // namespace Internal
} // namespace QmlJSEditor
