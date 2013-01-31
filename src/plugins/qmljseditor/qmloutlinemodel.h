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

#ifndef QMLOUTLINEMODEL_H
#define QMLOUTLINEMODEL_H

#include "qmljseditor.h"
#include <utils/changeset.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsicons.h>

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
    QVariant data(int role = Qt::UserRole + 1) const;
    int type() const;

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

    QmlOutlineModel(QmlJSTextEditorWidget *editor);

    // QStandardItemModel
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex  &parent);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QmlJS::Document::Ptr document() const;
    void update(const QmlJSTools::SemanticInfo &semanticInfo);

    QmlJS::AST::Node *nodeForIndex(const QModelIndex &index) const;
    QmlJS::AST::SourceLocation sourceLocation(const QModelIndex &index) const;
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

    QModelIndex enterFunctionDeclaration(QmlJS::AST::FunctionDeclaration *functionDeclaration);
    void leaveFunctionDeclaration();

    QModelIndex enterTestCase(QmlJS::AST::ObjectLiteral *objectLiteral);
    void leaveTestCase();

    QModelIndex enterTestCaseProperties(QmlJS::AST::PropertyNameAndValueList *propertyNameAndValueList);
    void leaveTestCaseProperties();

private:
    QmlOutlineItem *enterNode(QMap<int, QVariant> data, QmlJS::AST::Node *node, QmlJS::AST::UiQualifiedId *idNode, const QIcon &icon);
    void leaveNode();

    void reparentNodes(QmlOutlineItem *targetItem, int targetRow, QList<QmlOutlineItem*> itemsToMove);
    void moveObjectMember(QmlJS::AST::UiObjectMember *toMove, QmlJS::AST::UiObjectMember *newParent,
                          bool insertionOrderSpecified, QmlJS::AST::UiObjectMember *insertAfter,
                          Utils::ChangeSet *changeSet, Utils::ChangeSet::Range *addedRange);

    QStandardItem *parentItem();

    static QString asString(QmlJS::AST::UiQualifiedId *id);
    static QmlJS::AST::SourceLocation getLocation(QmlJS::AST::UiObjectMember *objMember);
    static QmlJS::AST::SourceLocation getLocation(QmlJS::AST::ExpressionNode *exprNode);
    static QmlJS::AST::SourceLocation getLocation(QmlJS::AST::PropertyNameAndValueList *propertyNode);
    QIcon getIcon(QmlJS::AST::UiQualifiedId *objDef);

    QString getAnnotation(QmlJS::AST::UiObjectInitializer *objInitializer);
    QString getAnnotation(QmlJS::AST::Statement *statement);
    QString getAnnotation(QmlJS::AST::ExpressionNode *expression);
    QHash<QString,QString> getScriptBindings(QmlJS::AST::UiObjectInitializer *objInitializer);


    QmlJSTools::SemanticInfo m_semanticInfo;
    QList<int> m_treePos;
    QStandardItem *m_currentItem;
    QmlJS::Icons *m_icons;

    QHash<QString, QIcon> m_typeToIcon;
    QHash<QmlOutlineItem*,QIcon> m_itemToIcon;
    QHash<QmlOutlineItem*,QmlJS::AST::Node*> m_itemToNode;
    QHash<QmlOutlineItem*,QmlJS::AST::UiQualifiedId*> m_itemToIdNode;
    QmlJSTextEditorWidget *m_textEditor;


    friend class QmlOutlineModelSync;
    friend class QmlOutlineItem;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLOUTLINEMODEL_H
