/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <utils/algorithm.h>

#include "backendmodel.h"

#include "bindingproperty.h"
#include "connectionview.h"
#include "exception.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "rewriterview.h"
#include "rewritertransaction.h"

#include "addnewbackenddialog.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

namespace Internal {

BackendModel::BackendModel(ConnectionView *parent) :
    QStandardItemModel(parent)
    ,m_connectionView(parent)
{
    connect(this, &QStandardItemModel::dataChanged, this, &BackendModel::handleDataChanged);
}

ConnectionView *QmlDesigner::Internal::BackendModel::connectionView() const
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
        for (const CppTypeData &cppTypeData : rewriterView->getCppTypes())
            if (cppTypeData.isSingleton) {
                NodeMetaInfo metaInfo = m_connectionView->model()->metaInfo(cppTypeData.typeName.toUtf8());
                  if (metaInfo.isValid() && !metaInfo.isSubclassOf("QtQuick.Item")) {
                      QStandardItem *type = new QStandardItem(cppTypeData.typeName);
                      type->setData(cppTypeData.typeName, Qt::UserRole + 1);
                      type->setData(true, Qt::UserRole + 2);
                      type->setEditable(false);

                      QStandardItem *name = new QStandardItem(cppTypeData.typeName);
                      name->setEditable(false);

                      QStandardItem *singletonItem = new QStandardItem("");
                      singletonItem->setCheckState(Qt::Checked);

                      singletonItem->setCheckable(true);
                      singletonItem->setEnabled(false);

                      QStandardItem *inlineItem = new QStandardItem("");

                      inlineItem->setCheckState(Qt::Unchecked);

                      inlineItem->setCheckable(true);
                      inlineItem->setEnabled(false);

                      appendRow({ type, name, singletonItem, inlineItem });
                  }
            }

    if (rootNode.isValid())
        foreach (const AbstractProperty &property ,rootNode.properties())
            if (property.isDynamic() && !simpleTypes.contains(property.dynamicTypeName())) {

                NodeMetaInfo metaInfo = m_connectionView->model()->metaInfo(property.dynamicTypeName());
                if (metaInfo.isValid() && !metaInfo.isSubclassOf("QtQuick.Item")) {
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

    m_lock = false;

    endResetModel();
}

QStringList BackendModel::possibleCppTypes() const
{
    RewriterView *rewriterView = m_connectionView->model()->rewriterView();

    QStringList list;

    if (rewriterView)
        foreach (const CppTypeData &cppTypeData, rewriterView->getCppTypes())
            list.append(cppTypeData.typeName);

    return list;
}

CppTypeData BackendModel::cppTypeDataForType(const QString &typeName) const
{
    RewriterView *rewriterView = m_connectionView->model()->rewriterView();

    QStringList list;

    if (!rewriterView)
        return CppTypeData();

    return Utils::findOr(rewriterView->getCppTypes(), CppTypeData(), [&typeName](const CppTypeData &data) {
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
         CppTypeData cppTypeData = cppTypeDataForType(typeName);

         if (cppTypeData.isSingleton) {

             Import import = Import::createLibraryImport(cppTypeData.importUrl, cppTypeData.versionString);

             try {
                 if (model->hasImport(import))
                     model->changeImports(QList<Import>(), QList<Import> { import } );
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

    AddNewBackendDialog dialog(Core::ICore::mainWindow());

    RewriterView *rewriterView = model->rewriterView();

    QStringList availableTypes;

    if (rewriterView)
        dialog.setupPossibleTypes(Utils::filtered(rewriterView->getCppTypes(), [model](const CppTypeData &cppTypeData) {
            return !cppTypeData.isSingleton || !model->metaInfo(cppTypeData.typeName.toUtf8()).isValid();
            /* Only show singletons if the import is missing */
        }));

    dialog.exec();

    if (dialog.applied()) {
        QStringList importSplit = dialog.importString().split(" ");
        if (importSplit.count() != 2) {
            qWarning() << Q_FUNC_INFO << "invalid import" << importSplit;
            QTC_ASSERT(false, return);
        }

        QString typeName = dialog.type();

        Import import = Import::createLibraryImport(importSplit.first(), importSplit.last());

        try {

            /* We cannot add an import and add a node from that import in a single transaction.
             * We need the import to have the meta info available.
             */

            if (!model->hasImport(import))
                model->changeImports(QList<Import> { import }, QList<Import>());

            QString propertyName = m_connectionView->generateNewId(typeName);

            NodeMetaInfo metaInfo = model->metaInfo(typeName.toUtf8());

            QTC_ASSERT(metaInfo.isValid(), return);

            int minorVersion = metaInfo.minorVersion();
            int majorVersion = metaInfo.majorVersion();

            /* Add a property for non singleton types. For singletons just adding the import is enough. */
            if (!dialog.isSingleton()) {
                RewriterTransaction transaction = m_connectionView->beginRewriterTransaction("BackendModel::addNewBackend");

                if (dialog.localDefinition()) {
                    ModelNode newNode = m_connectionView->createModelNode(metaInfo.typeName(), majorVersion, minorVersion);

                    m_connectionView->rootModelNode().nodeProperty(propertyName.toUtf8()).setDynamicTypeNameAndsetModelNode(
                                typeName.toUtf8(), newNode);
                } else {
                    m_connectionView->rootModelNode().bindingProperty(
                                propertyName.toUtf8()).setDynamicTypeNameAndExpression(typeName.toUtf8(), "null");
                }
                transaction.commit();
            }

        } catch (const Exception &e) {
            e.showException();
        }
    }

    resetModel();
}

void BackendModel::updatePropertyName(int rowNumber)
{
    const PropertyName newName = data(index(rowNumber, 1)).toString().toUtf8();
    const PropertyName oldName  = data(index(rowNumber, 0), Qt::UserRole + 1).toString().toUtf8();

    ModelNode rootModelNode = m_connectionView->rootModelNode();

    try {
        RewriterTransaction transaction = m_connectionView->beginRewriterTransaction("BackendModel::updatePropertyName");

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

        transaction.commit();

    } catch (const Exception &e) {
        e.showException();
    }
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

} // namespace Internal

} // namespace QmlDesigner
