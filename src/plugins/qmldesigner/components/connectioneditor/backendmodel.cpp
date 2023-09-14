// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/algorithm.h>

#include "backendmodel.h"

#include "bindingproperty.h"
#include "connectionview.h"
#include "exception.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "rewriterview.h"

#include "addnewbackenddialog.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

BackendModel::BackendModel(ConnectionView *parent) :
    QStandardItemModel(parent)
    ,m_connectionView(parent)
{
    connect(this, &QStandardItemModel::dataChanged, this, &BackendModel::handleDataChanged);
}

ConnectionView *BackendModel::connectionView() const
{
    return m_connectionView;
}

void BackendModel::resetModel()
{
    if (!m_connectionView->model())
           return;

    RewriterView *rewriterView = m_connectionView->model()->rewriterView();

    m_lock = true;

    beginResetModel();
    clear();

    setHorizontalHeaderLabels(QStringList({ tr("Type"), tr("Name"), tr("Singleton"), tr("Local") }));

    ModelNode rootNode = connectionView()->rootModelNode();

    static const PropertyTypeList simpleTypes = {"int", "real", "color", "string"};

    if (rewriterView)
        for (const QmlTypeData &cppTypeData : rewriterView->getQMLTypes())
            if (cppTypeData.isSingleton) {
                NodeMetaInfo metaInfo = m_connectionView->model()->metaInfo(
                    cppTypeData.typeName.toUtf8());
                if (metaInfo.isValid() && !metaInfo.isQtQuickItem()) {
                    auto type = new QStandardItem(cppTypeData.typeName);
                    type->setData(cppTypeData.typeName, Qt::UserRole + 1);
                    type->setData(true, Qt::UserRole + 2);
                    type->setEditable(false);

                    auto name = new QStandardItem(cppTypeData.typeName);
                    name->setEditable(false);

                    QStandardItem *singletonItem = new QStandardItem("");
                    singletonItem->setCheckState(Qt::Checked);

                    singletonItem->setCheckable(true);
                    singletonItem->setEnabled(false);

                    QStandardItem *inlineItem = new QStandardItem("");

                    inlineItem->setCheckState(Qt::Unchecked);

                    inlineItem->setCheckable(true);
                    inlineItem->setEnabled(false);

                    appendRow({type, name, singletonItem, inlineItem});
                }
            }

    if (rootNode.isValid()) {
        const QList<AbstractProperty> properties = rootNode.properties();
        for (const AbstractProperty &property : properties)
            if (property.isDynamic() && !simpleTypes.contains(property.dynamicTypeName())) {

                NodeMetaInfo metaInfo = m_connectionView->model()->metaInfo(property.dynamicTypeName());
                if (metaInfo.isValid() && !metaInfo.isQtQuickItem()) {
                    QStandardItem *type = new QStandardItem(QString::fromUtf8(property.dynamicTypeName()));
                    type->setEditable(false);

                    type->setData(QString::fromUtf8(property.name()), Qt::UserRole + 1);
                    type->setData(false, Qt::UserRole + 2);
                    QStandardItem *name = new QStandardItem(QString::fromUtf8(property.name()));

                    QStandardItem *singletonItem = new QStandardItem("");
                    singletonItem->setCheckState(Qt::Unchecked);

                    singletonItem->setCheckable(true);
                    singletonItem->setEnabled(false);

                    QStandardItem *inlineItem = new QStandardItem("");

                    inlineItem->setCheckState(property.isNodeProperty() ? Qt::Checked : Qt::Unchecked);

                    inlineItem->setCheckable(true);
                    inlineItem->setEnabled(false);

                    appendRow({ type, name, singletonItem, inlineItem });
                }
            }
    }

    m_lock = false;

    endResetModel();
}

QStringList BackendModel::possibleCppTypes() const
{
    RewriterView *rewriterView = m_connectionView->model()->rewriterView();

    QStringList list;

    if (rewriterView) {
        const QList<QmlTypeData> cppTypes = rewriterView->getQMLTypes();
        for (const QmlTypeData &cppTypeData : cppTypes)
            list.append(cppTypeData.typeName);
    }

    return list;
}

QmlTypeData BackendModel::cppTypeDataForType(const QString &typeName) const
{
    RewriterView *rewriterView = m_connectionView->model()->rewriterView();

    if (!rewriterView)
        return QmlTypeData();

    return Utils::findOr(rewriterView->getQMLTypes(), QmlTypeData(), [&typeName](const QmlTypeData &data) {
        return typeName == data.typeName;
    });
}

void BackendModel::deletePropertyByRow(int rowNumber)
{
    Model *model = m_connectionView->model();
    if (!model)
        return;

    /* singleton case remove the import */
    if (data(index(rowNumber, 0), Qt::UserRole + 1).toBool()) {
        const QString typeName = data(index(rowNumber, 0), Qt::UserRole + 1).toString();
         QmlTypeData cppTypeData = cppTypeDataForType(typeName);

         if (cppTypeData.isSingleton) {

             Import import = Import::createLibraryImport(cppTypeData.importUrl, cppTypeData.versionString);

             try {
                 if (model->hasImport(import))
                     model->changeImports({}, {import});
             } catch (const Exception &e) {
                 e.showException();
             }
         }
    } else {
        const QString propertyName = data(index(rowNumber, 0), Qt::UserRole + 1).toString();

        ModelNode  modelNode = connectionView()->rootModelNode();

        try {
            modelNode.removeProperty(propertyName.toUtf8());
        } catch (const Exception &e) {
            e.showException();
        }
    }

    resetModel();
}

void BackendModel::addNewBackend()
{
    Model *model = m_connectionView->model();
    if (!model)
        return;

    AddNewBackendDialog dialog(Core::ICore::dialogParent());

    RewriterView *rewriterView = model->rewriterView();

    QStringList availableTypes;

    if (rewriterView)
        dialog.setupPossibleTypes(Utils::filtered(rewriterView->getQMLTypes(), [model](const QmlTypeData &cppTypeData) {
            return !cppTypeData.isSingleton || !model->metaInfo(cppTypeData.typeName.toUtf8()).isValid();
            /* Only show singletons if the import is missing */
        }));

    dialog.exec();

    if (dialog.applied()) {
        QStringList importSplit = dialog.importString().split(" ");
        if (importSplit.size() != 2) {
            qWarning() << Q_FUNC_INFO << "invalid import" << importSplit;
            QTC_ASSERT(false, return);
        }

        QString typeName = dialog.type();

        Import import = Import::createLibraryImport(importSplit.constFirst(), importSplit.constLast());

        /* We cannot add an import and add a node from that import in a single transaction.
             * We need the import to have the meta info available.
             */

        if (!model->hasImport(import))
            model->changeImports({import}, {});

        QString propertyName = m_connectionView->model()->generateNewId(typeName);

        NodeMetaInfo metaInfo = model->metaInfo(typeName.toUtf8());

        QTC_ASSERT(metaInfo.isValid(), return);

        /* Add a property for non singleton types. For singletons just adding the import is enough. */
        if (!dialog.isSingleton()) {
            m_connectionView->executeInTransaction("BackendModel::addNewBackend", [=, &dialog](){
                int minorVersion = metaInfo.minorVersion();
                int majorVersion = metaInfo.majorVersion();

                if (dialog.localDefinition()) {
                    ModelNode newNode = m_connectionView->createModelNode(useProjectStorage()
                                                                              ? typeName.toUtf8()
                                                                              : metaInfo.typeName(),
                                                                          majorVersion,
                                                                          minorVersion);

                    m_connectionView->rootModelNode().nodeProperty(propertyName.toUtf8()).setDynamicTypeNameAndsetModelNode(
                                typeName.toUtf8(), newNode);
                } else {
                    m_connectionView->rootModelNode().bindingProperty(
                                propertyName.toUtf8()).setDynamicTypeNameAndExpression(typeName.toUtf8(), "null");
                }
            });
        }
    }
    resetModel();
}

void BackendModel::updatePropertyName(int rowNumber)
{
    const PropertyName newName = data(index(rowNumber, 1)).toString().toUtf8();
    const PropertyName oldName  = data(index(rowNumber, 0), Qt::UserRole + 1).toString().toUtf8();

    m_connectionView->executeInTransaction("BackendModel::updatePropertyName", [this, newName, oldName](){

        ModelNode rootModelNode = m_connectionView->rootModelNode();
        if (rootModelNode.property(oldName).isNodeProperty()) {

            const TypeName typeName = rootModelNode.nodeProperty(oldName).dynamicTypeName();
            const ModelNode targetModelNode = rootModelNode.nodeProperty(oldName).modelNode();
            const TypeName fullTypeName = targetModelNode.type();
            const int majorVersion = targetModelNode.majorVersion();
            const int minorVersion = targetModelNode.minorVersion();

            rootModelNode.removeProperty(oldName);
            ModelNode newNode = m_connectionView->createModelNode(fullTypeName, majorVersion, minorVersion);
            m_connectionView->rootModelNode().nodeProperty(newName).setDynamicTypeNameAndsetModelNode(typeName, newNode);

        } else if (rootModelNode.property(oldName).isBindingProperty()) {
            const QString expression = rootModelNode.bindingProperty(oldName).expression();
            const TypeName typeName = rootModelNode.bindingProperty(oldName).dynamicTypeName();

            rootModelNode.removeProperty(oldName);
            rootModelNode.bindingProperty(newName).setDynamicTypeNameAndExpression(typeName, expression);
        } else {
            qWarning() << Q_FUNC_INFO << oldName << newName << "failed...";
            QTC_ASSERT(false, return);
        }
    });
}

void BackendModel::handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (m_lock)
        return;

    if (topLeft != bottomRight) {
        qWarning() << "BackendModel::handleDataChanged multi edit?";
        return;
    }

    m_lock = true;

    int currentColumn = topLeft.column();
    int currentRow = topLeft.row();

    switch (currentColumn) {
    case 0: {
        //updating user data
    } break;
    case 1: {
        updatePropertyName(currentRow);
    } break;

    default: qWarning() << "BindingModel::handleDataChanged column" << currentColumn;
    }

    m_lock = false;
}

} // namespace QmlDesigner
