/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "bindingmodel.h"

#include "connectionview.h"

#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <variantproperty.h>
#include <bindingproperty.h>
#include <rewritingexception.h>
#include <rewritertransaction.h>

#include <utils/fileutils.h>

#include <QItemEditorFactory>
#include <QComboBox>
#include <QMessageBox>
#include <QStyleFactory>
#include <QTimer>

namespace {

enum ColumnRoles {
    TargetModelNodeRow = 0,
    TargetPropertyNameRow = 1,
    SourceModelNodeRow = 2,
    SourcePropertyNameRow = 3
};

bool compareBindingProperties(const QmlDesigner::BindingProperty &bindingProperty01, const QmlDesigner::BindingProperty &bindingProperty02)
{
    if (bindingProperty01.parentModelNode() != bindingProperty02.parentModelNode())
        return false;
    if (bindingProperty01.name() != bindingProperty02.name())
        return false;
    return true;
}

} //internal namespace

namespace QmlDesigner {

namespace Internal {

BindingModel::BindingModel(ConnectionView *parent) :
    QStandardItemModel(parent),
    m_connectionView(parent),
    m_lock(false),
    m_handleDataChanged(false)
{
    connect(this, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(handleDataChanged(QModelIndex,QModelIndex)));
}

void BindingModel::bindingChanged(const BindingProperty &bindingProperty)
{
    m_handleDataChanged = false;

    QList<ModelNode> selectedNodes = connectionView()->selectedModelNodes();
    if (!selectedNodes.contains(bindingProperty.parentModelNode()))
        return;
    if (!m_lock) {
        int rowNumber = findRowForBinding(bindingProperty);

        if (rowNumber == -1) {
            addBindingProperty(bindingProperty);
        } else {
            updateBindingProperty(rowNumber);
        }
    }

    m_handleDataChanged = true;
}

void BindingModel::bindingRemoved(const BindingProperty &bindingProperty)
{
    m_handleDataChanged = false;

    QList<ModelNode> selectedNodes = connectionView()->selectedModelNodes();
    if (!selectedNodes.contains(bindingProperty.parentModelNode()))
        return;
    if (!m_lock) {
        int rowNumber = findRowForBinding(bindingProperty);
        removeRow(rowNumber);
    }

    m_handleDataChanged = true;
}

void BindingModel::selectionChanged(const QList<ModelNode> &selectedNodes)
{
    m_handleDataChanged = false;
    m_selectedModelNodes = selectedNodes;
    resetModel();
    m_handleDataChanged = true;
}

ConnectionView *BindingModel::connectionView() const
{
    return m_connectionView;
}

BindingProperty BindingModel::bindingPropertyForRow(int rowNumber) const
{

    const int internalId = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 1).toInt();
    const QString targetPropertyName = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 2).toString();

    ModelNode  modelNode = connectionView()->modelNodeForInternalId(internalId);

    if (modelNode.isValid())
        return modelNode.bindingProperty(targetPropertyName.toLatin1());

    return BindingProperty();
}

QStringList BindingModel::possibleTargetProperties(const BindingProperty &bindingProperty) const
{
    const ModelNode modelNode = bindingProperty.parentModelNode();

    if (!modelNode.isValid()) {
        qWarning() << " BindingModel::possibleTargetPropertiesForRow invalid model node";
        return QStringList();
    }

    NodeMetaInfo metaInfo = modelNode.metaInfo();

    if (metaInfo.isValid()) {
        QStringList possibleProperties;
        foreach (const PropertyName &propertyName, metaInfo.propertyNames()) {
            if (metaInfo.propertyIsWritable(propertyName))
                possibleProperties << QString::fromLatin1(propertyName);
        }

        return possibleProperties;
    }

    return QStringList();
}

QStringList BindingModel::possibleSourceProperties(const BindingProperty &bindingProperty) const
{
    const QString expression = bindingProperty.expression();
    const QStringList stringlist = expression.split(QLatin1String("."));

    TypeName typeName;

    if (bindingProperty.parentModelNode().metaInfo().isValid()) {
        typeName = bindingProperty.parentModelNode().metaInfo().propertyTypeName(bindingProperty.name());
    } else {
        qWarning() << " BindingModel::possibleSourcePropertiesForRow no meta info for target node";
    }

    const QString id = stringlist.first();

    ModelNode modelNode = getNodeByIdOrParent(id, bindingProperty.parentModelNode());

    if (!modelNode.isValid()) {
        qWarning() << " BindingModel::possibleSourcePropertiesForRow invalid model node";
        return QStringList();
    }

    NodeMetaInfo metaInfo = modelNode.metaInfo();

    QStringList possibleProperties;

    foreach (VariantProperty variantProperty, modelNode.variantProperties()) {
        if (variantProperty.isDynamic())
            possibleProperties << QString::fromLatin1((variantProperty.name()));
    }

    foreach (BindingProperty bindingProperty, modelNode.bindingProperties()) {
        if (bindingProperty.isDynamic())
            possibleProperties << QString::fromLatin1((bindingProperty.name()));
    }

    if (metaInfo.isValid())  {
        foreach (const PropertyName &propertyName, metaInfo.propertyNames()) {
            if (metaInfo.propertyTypeName(propertyName) == typeName) //### todo proper check
                possibleProperties << QString::fromLatin1(propertyName);
        }
    } else {
        qWarning() << " BindingModel::possibleSourcePropertiesForRow no meta info for source node";
    }

    return possibleProperties;
}

void BindingModel::deleteBindindByRow(int rowNumber)
{
      BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);

      if (bindingProperty.isValid()) {
        bindingProperty.parentModelNode().removeProperty(bindingProperty.name());
      }

      resetModel();
}

static PropertyName unusedProperty(const ModelNode &modelNode)
{
    PropertyName propertyName = "none";
    if (modelNode.metaInfo().isValid()) {
        foreach (const PropertyName &propertyName, modelNode.metaInfo().propertyNames()) {
            if (modelNode.metaInfo().propertyIsWritable(propertyName) && !modelNode.hasProperty(propertyName))
                return propertyName;
        }
    }

    return propertyName;
}

void BindingModel::addBindingForCurrentNode()
{
    if (connectionView()->selectedModelNodes().count() == 1) {
        ModelNode modelNode = connectionView()->selectedModelNodes().first();
        if (modelNode.isValid()) {
            try {
                modelNode.bindingProperty(unusedProperty(modelNode)).setExpression(QLatin1String("none.none"));
            } catch (RewritingException &e) {
                m_exceptionError = e.description();
                QTimer::singleShot(200, this, SLOT(handleException()));
            }
        }
    } else {
        qWarning() << " BindingModel::addBindingForCurrentNode not one node selected";
    }
}

void BindingModel::resetModel()
{
    beginResetModel();
    clear();

    QStringList labels;

    labels << tr("Item");
    labels <<tr("Property");
    labels <<tr("Source Item");
    labels <<tr("Source Property");

    setHorizontalHeaderLabels(labels);

    foreach (const ModelNode modelNode, m_selectedModelNodes) {
        addModelNode(modelNode);
    }

    endResetModel();
}

void BindingModel::addBindingProperty(const BindingProperty &property)
{
    QStandardItem *idItem;
    QStandardItem *targetPropertyNameItem;
    QStandardItem *sourceIdItem;
    QStandardItem *sourcePropertyNameItem;

    QString idLabel = property.parentModelNode().id();
    if (idLabel.isEmpty())
        idLabel = QString::fromLatin1(property.parentModelNode().simplifiedTypeName());
    idItem = new QStandardItem(idLabel);
    updateCustomData(idItem, property);
    targetPropertyNameItem = new QStandardItem(QString::fromLatin1(property.name()));
    QList<QStandardItem*> items;

    items.append(idItem);
    items.append(targetPropertyNameItem);

    QString sourceNodeName;
    QString sourcePropertyName;
    getExpressionStrings(property, &sourceNodeName, &sourcePropertyName);

    sourceIdItem = new QStandardItem(sourceNodeName);
    sourcePropertyNameItem = new QStandardItem(sourcePropertyName);

    items.append(sourceIdItem);
    items.append(sourcePropertyNameItem);
    appendRow(items);
}

void BindingModel::updateBindingProperty(int rowNumber)
{
    BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);

    if (bindingProperty.isValid()) {
        QString targetPropertyName = QString::fromLatin1(bindingProperty.name());
        updateDisplayRole(rowNumber, TargetPropertyNameRow, targetPropertyName);
        QString sourceNodeName;
        QString sourcePropertyName;
        getExpressionStrings(bindingProperty, &sourceNodeName, &sourcePropertyName);
        updateDisplayRole(rowNumber, SourceModelNodeRow, sourceNodeName);
        updateDisplayRole(rowNumber, SourcePropertyNameRow, sourcePropertyName);
    }
}

void BindingModel::addModelNode(const ModelNode &modelNode)
{
    foreach (const BindingProperty &bindingProperty, modelNode.bindingProperties()) {
        addBindingProperty(bindingProperty);
    }
}

void BindingModel::updateExpression(int row)
{
    BindingProperty bindingProperty = bindingPropertyForRow(row);

    const QString sourceNode = data(index(row, SourceModelNodeRow)).toString();
    const QString sourceProperty = data(index(row, SourcePropertyNameRow)).toString();

    QString expression;
    if (sourceProperty.isEmpty()) {
        expression = sourceNode;
    } else {
        expression = sourceNode + QLatin1String(".") + sourceProperty;
    }

    RewriterTransaction transaction =
        connectionView()->beginRewriterTransaction(QByteArrayLiteral("BindingModel::updateExpression"));
    try {
        bindingProperty.setExpression(expression);
        transaction.commit(); //committing in the try block
    } catch (Exception &e) {
        m_exceptionError = e.description();
        QTimer::singleShot(200, this, SLOT(handleException()));
    }
}

void BindingModel::updatePropertyName(int rowNumber)
{
    BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);

    const PropertyName newName = data(index(rowNumber, TargetPropertyNameRow)).toString().toLatin1();
    const QString expression = bindingProperty.expression();
    const PropertyName dynamicPropertyType = bindingProperty.dynamicTypeName();
    ModelNode targetNode = bindingProperty.parentModelNode();

    if (!newName.isEmpty()) {
        RewriterTransaction transaction =
            connectionView()->beginRewriterTransaction(QByteArrayLiteral("BindingModel::updatePropertyName"));
        try {
            if (bindingProperty.isDynamic()) {
                targetNode.bindingProperty(newName).setDynamicTypeNameAndExpression(dynamicPropertyType, expression);
            } else {
                targetNode.bindingProperty(newName).setExpression(expression);
            }
            targetNode.removeProperty(bindingProperty.name());
            transaction.commit(); //committing in the try block
        } catch (Exception &e) { //better save then sorry
            m_exceptionError = e.description();
            QTimer::singleShot(200, this, SLOT(handleException()));
        }

        QStandardItem* idItem = item(rowNumber, 0);
        BindingProperty newBindingProperty = targetNode.bindingProperty(newName);
        updateCustomData(idItem, newBindingProperty);

    } else {
        qWarning() << "BindingModel::updatePropertyName invalid property name";
    }
}

ModelNode BindingModel::getNodeByIdOrParent(const QString &id, const ModelNode &targetNode) const
{
    ModelNode modelNode;

    if (id != QLatin1String("parent")) {
        modelNode = connectionView()->modelNodeForId(id);
    } else {
        if (targetNode.hasParentProperty()) {
            modelNode = targetNode.parentProperty().parentModelNode();
        }
    }
    return modelNode;
}

void BindingModel::updateCustomData(QStandardItem *item, const BindingProperty &bindingProperty)
{
    item->setData(bindingProperty.parentModelNode().internalId(), Qt::UserRole + 1);
    item->setData(bindingProperty.name(), Qt::UserRole + 2);
}

int BindingModel::findRowForBinding(const BindingProperty &bindingProperty)
{
    for (int i=0; i < rowCount(); i++) {
        if (compareBindingProperties(bindingPropertyForRow(i), bindingProperty))
            return i;
    }
    //not found
    return -1;
}

bool BindingModel::getExpressionStrings(const BindingProperty &bindingProperty, QString *sourceNode, QString *sourceProperty)
{
    //### todo we assume no expressions yet

    const QString expression = bindingProperty.expression();

    if (true) {
        const QStringList stringList = expression.split(QLatin1String("."));

        *sourceNode = stringList.first();

        QString propertyName;

        for (int i=1; i < stringList.count(); i++) {
            propertyName += stringList.at(i);
            if (i != stringList.count() - 1)
                propertyName += QLatin1String(".");
        }
        *sourceProperty = propertyName;
    }
    return true;
}

void BindingModel::updateDisplayRole(int row, int columns, const QString &string)
{
    QModelIndex modelIndex = index(row, columns);
    if (data(modelIndex).toString() != string)
        setData(modelIndex, string);
}

void BindingModel::handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_handleDataChanged)
        return;

    if (topLeft != bottomRight) {
        qWarning() << "BindingModel::handleDataChanged multi edit?";
        return;
    }

    m_lock = true;

    int currentColumn = topLeft.column();
    int currentRow = topLeft.row();

    switch (currentColumn) {
    case TargetModelNodeRow: {
        //updating user data
    } break;
    case TargetPropertyNameRow: {
        updatePropertyName(currentRow);
    } break;
    case SourceModelNodeRow: {
        updateExpression(currentRow);
    } break;
    case SourcePropertyNameRow: {
        updateExpression(currentRow);
    } break;

    default: qWarning() << "BindingModel::handleDataChanged column" << currentColumn;
    }

    m_lock = false;
}

void BindingModel::handleException()
{
    QMessageBox::warning(0, tr("Error"), m_exceptionError);
    resetModel();
}

BindingDelegate::BindingDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
    static QItemEditorFactory *factory = 0;
        if (factory == 0) {
            factory = new QItemEditorFactory;
            QItemEditorCreatorBase *creator
                = new QItemEditorCreator<BindingComboBox>("text");
            factory->registerEditor(QVariant::String, creator);
        }

        setItemEditorFactory(factory);
}

QWidget *BindingDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
        QWidget *widget = QStyledItemDelegate::createEditor(parent, option, index);

        const BindingModel *model = qobject_cast<const BindingModel*>(index.model());

        model->connectionView()->allModelNodes();

        BindingComboBox *bindingComboBox = qobject_cast<BindingComboBox*>(widget);

        if (!model) {
            qWarning() << "BindingDelegate::createEditor no model";
            return widget;
        }

        if (!model->connectionView()) {
            qWarning() << "BindingDelegate::createEditor no connection view";
            return widget;
        }

        if (!bindingComboBox) {
            qWarning() << "BindingDelegate::createEditor no bindingComboBox";
            return widget;
        }

        BindingProperty bindingProperty = model->bindingPropertyForRow(index.row());

        switch (index.column()) {
        case TargetModelNodeRow: {
            return 0; //no editor
            foreach (const ModelNode &modelNode, model->connectionView()->allModelNodes()) {
                if (!modelNode.id().isEmpty()) {
                    bindingComboBox->addItem(modelNode.id());
                }
            }
        } break;
        case TargetPropertyNameRow: {
            bindingComboBox->addItems(model->possibleTargetProperties(bindingProperty));
        } break;
        case SourceModelNodeRow: {
            foreach (const ModelNode &modelNode, model->connectionView()->allModelNodes()) {
                if (!modelNode.id().isEmpty()) {
                    bindingComboBox->addItem(modelNode.id());
                }
            }
            if (!bindingProperty.parentModelNode().isRootNode())
                bindingComboBox->addItem(QLatin1String("parent"));
        } break;
        case SourcePropertyNameRow: {
            bindingComboBox->addItems(model->possibleSourceProperties(bindingProperty));
        } break;
        default: qWarning() << "BindingDelegate::createEditor column" << index.column();
        }

        connect(bindingComboBox, SIGNAL(activated(QString)), this, SLOT(emitCommitData(QString)));

        return widget;
}

void BindingDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    opt.state &= ~QStyle::State_HasFocus;
    QStyledItemDelegate::paint(painter, opt, index);
}

void BindingDelegate::emitCommitData(const QString & /*text*/)
{
    BindingComboBox *bindingComboBox = qobject_cast<BindingComboBox*>(sender());
    emit commitData(bindingComboBox);
}

BindingComboBox::BindingComboBox(QWidget *parent) : QComboBox(parent)
{
    static QScopedPointer<QStyle> style(QStyleFactory::create(QLatin1String("windows")));
    setEditable(true);
    if (style)
        setStyle(style.data());
}

QString BindingComboBox::text() const
{
    return currentText();
}

void BindingComboBox::setText(const QString &text)
{
    setEditText(text);
}

} // namespace Internal

} // namespace QmlDesigner
