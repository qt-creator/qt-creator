/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLOUTLINEMODEL_H
#define QMLOUTLINEMODEL_H

#include "qmljseditor.h"
#include <utils/changeset.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsicons.h>
#include <qmljs/qmljslookupcontext.h>

#include <QtGui/QStandardItemModel>

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

    // QStandardItem
    QVariant data(int role = Qt::UserRole + 1) const;
    int type() const;

    void setItemData(const QMap<int, QVariant> &roles);

private:
    QString prettyPrint(const QmlJS::Interpreter::Value *value, const QmlJS::Interpreter::Context *context) const;

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

    QmlOutlineModel(QmlJSTextEditor *editor);

    // QStandardItemModel
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex  &parent);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QmlJS::Document::Ptr document() const;
    void update(const SemanticInfo &semanticInfo);

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
    QIcon getIcon(QmlJS::AST::UiQualifiedId *objDef);

    QString getAnnotation(QmlJS::AST::UiObjectInitializer *objInitializer);
    QString getAnnotation(QmlJS::AST::Statement *statement);
    QString getAnnotation(QmlJS::AST::ExpressionNode *expression);
    QHash<QString,QString> getScriptBindings(QmlJS::AST::UiObjectInitializer *objInitializer);


    SemanticInfo m_semanticInfo;
    QList<int> m_treePos;
    QStandardItem *m_currentItem;
    QmlJS::Icons *m_icons;

    QHash<QString, QIcon> m_typeToIcon;
    QHash<QmlOutlineItem*,QIcon> m_itemToIcon;
    QHash<QmlOutlineItem*,QmlJS::AST::Node*> m_itemToNode;
    QHash<QmlOutlineItem*,QmlJS::AST::UiQualifiedId*> m_itemToIdNode;
    QmlJSTextEditor *m_textEditor;


    friend class QmlOutlineModelSync;
    friend class QmlOutlineItem;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLOUTLINEMODEL_H
