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

#include "qmlmodelview.h"
#include "qmlobjectnode.h"
#include "qmlitemnode.h"
#include "itemlibraryinfo.h"
#include "mathutils.h"
#include "invalididexception.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <utils/fileutils.h>
#include "nodeabstractproperty.h"
#include "variantproperty.h"
#include "rewritingexception.h"
#include "rewriterview.h"
#include "plaintexteditmodifier.h"
#include "modelmerger.h"
#include "nodemetainfo.h"

#include <utils/qtcassert.h>

namespace QmlDesigner {

QmlModelView::QmlModelView(QObject *parent)
    : AbstractView(parent)
{
}

void QmlModelView::setCurrentState(const QmlModelState &state)
{
    if (!model() && !state.isValid())
        return;

    if (actualStateNode() != state.modelNode())
        setAcutalStateNode(state.modelNode());
}

QmlModelState QmlModelView::currentState() const
{
    return QmlModelState(actualStateNode());
}

QmlModelState QmlModelView::baseState() const
{
    return QmlModelState::createBaseState(this);
}

QmlModelStateGroup QmlModelView::rootStateGroup() const
{
    return QmlModelStateGroup(rootModelNode());
}

QmlItemNode QmlModelView::createQmlItemNodeFromImage(const QString &imageName, const QPointF &position, QmlItemNode parentQmlItemNode)
{
    if (!parentQmlItemNode.isValid() && rootQmlItemNode().isValid())
        parentQmlItemNode = rootQmlItemNode();
    else
        return QmlItemNode();

    QmlItemNode newQmlItemNode;
    RewriterTransaction transaction = beginRewriterTransaction();
    {
        const QString newImportUrl = QLatin1String("QtQuick");
        const QString newImportVersion = QLatin1String("1.1");
        Import newImport = Import::createLibraryImport(newImportUrl, newImportVersion);

        foreach (const Import &import, model()->imports()) {
            if (import.isLibraryImport()
                && import.url() == newImport.url()) {
                // reuse this import
                newImport = import;
                break;
            }
        }

        if (!model()->hasImport(newImport, true, true))
            model()->changeImports(QList<Import>() << newImport, QList<Import>());

        QList<QPair<PropertyName, QVariant> > propertyPairList;
        propertyPairList.append(qMakePair(PropertyName("x"), QVariant( round(position.x(), 4))));
        propertyPairList.append(qMakePair(PropertyName("y"), QVariant( round(position.y(), 4))));

        QString relativeImageName = imageName;

        //use relative path
        if (QFileInfo(model()->fileUrl().toLocalFile()).exists()) {
            QDir fileDir(QFileInfo(model()->fileUrl().toLocalFile()).absolutePath());
            relativeImageName = fileDir.relativeFilePath(imageName);
        }

        propertyPairList.append(qMakePair(PropertyName("source"), QVariant(relativeImageName)));
        NodeMetaInfo metaInfo = model()->metaInfo("QtQuick.Image");
        if (metaInfo.isValid()) {
            int minorVersion = metaInfo.minorVersion();
            int majorVersion = metaInfo.majorVersion();
            newQmlItemNode = QmlItemNode(createModelNode("QtQuick.Image", majorVersion, minorVersion, propertyPairList));
            parentQmlItemNode.nodeAbstractProperty("data").reparentHere(newQmlItemNode);
        }

        Q_ASSERT(newQmlItemNode.isValid());

        QString id;
        int i = 1;
        QString name("image");
        name.remove(QLatin1Char(' '));
        do {
            id = name + QString::number(i);
            i++;
        } while (hasId(id)); //If the name already exists count upwards

        newQmlItemNode.setId(id);
        if (!currentState().isBaseState()) {
            newQmlItemNode.modelNode().variantProperty("opacity") = 0;
            newQmlItemNode.setVariantProperty("opacity", 1);
        }

        Q_ASSERT(newQmlItemNode.isValid());
    }

    Q_ASSERT(newQmlItemNode.isValid());

    return newQmlItemNode;
}

QmlItemNode QmlModelView::createQmlItemNode(const ItemLibraryEntry &itemLibraryEntry, const QPointF &position, QmlItemNode parentQmlItemNode)
{
    if (!parentQmlItemNode.isValid())
        parentQmlItemNode = rootQmlItemNode();

    Q_ASSERT(parentQmlItemNode.isValid());

    QmlItemNode newQmlItemNode;

    try {
        RewriterTransaction transaction = beginRewriterTransaction();

        NodeMetaInfo metaInfo = model()->metaInfo(itemLibraryEntry.typeName());

        int minorVersion = metaInfo.minorVersion();
        int majorVersion = metaInfo.majorVersion();

        if (itemLibraryEntry.typeName().contains('.')) {

            const QString newImportUrl = itemLibraryEntry.requiredImport();

            if (!itemLibraryEntry.requiredImport().isEmpty()) {
                const QString newImportVersion = QString("%1.%2").arg(QString::number(itemLibraryEntry.majorVersion()), QString::number(itemLibraryEntry.minorVersion()));

                Import newImport = Import::createLibraryImport(newImportUrl, newImportVersion);
                if (itemLibraryEntry.majorVersion() == -1 && itemLibraryEntry.minorVersion() == -1)
                    newImport = Import::createFileImport(newImportUrl, QString());
                else
                    newImport = Import::createLibraryImport(newImportUrl, newImportVersion);

                foreach (const Import &import, model()->imports()) {
                    if (import.isLibraryImport()
                            && import.url() == newImport.url()
                            && import.version() == newImport.version()) {
                        // reuse this import
                        newImport = import;
                        break;
                    }
                }

                if (!model()->hasImport(newImport, true, true))
                    model()->changeImports(QList<Import>() << newImport, QList<Import>());
            }
        }

        QList<QPair<PropertyName, QVariant> > propertyPairList;
        propertyPairList.append(qMakePair(PropertyName("x"), QVariant(round(position.x(), 4))));
        propertyPairList.append(qMakePair(PropertyName("y"), QVariant(round(position.y(), 4))));

        if (itemLibraryEntry.qml().isEmpty()) {
            foreach (const PropertyContainer &property, itemLibraryEntry.properties())
                propertyPairList.append(qMakePair(property.name(), property.value()));

            newQmlItemNode = QmlItemNode(createModelNode(itemLibraryEntry.typeName(), majorVersion, minorVersion, propertyPairList));
        } else {
            QScopedPointer<Model> inputModel(Model::create("QtQuick.Rectangle", 1, 0, model()));
            inputModel->setFileUrl(model()->fileUrl());
            QPlainTextEdit textEdit;


            textEdit.setPlainText(itemLibraryEntry.qmlSource());
            NotIndentingTextEditModifier modifier(&textEdit);

            QScopedPointer<RewriterView> rewriterView(new RewriterView(RewriterView::Amend, 0));
            rewriterView->setCheckSemanticErrors(false);
            rewriterView->setTextModifier(&modifier);
            inputModel->setRewriterView(rewriterView.data());

            if (rewriterView->errors().isEmpty() && rewriterView->rootModelNode().isValid()) {
                ModelNode rootModelNode = rewriterView->rootModelNode();
                inputModel->detachView(rewriterView.data());

                rootModelNode.variantProperty("x") = propertyPairList.first().second;
                rootModelNode.variantProperty("y") = propertyPairList.at(1).second;

                ModelMerger merger(this);
                newQmlItemNode = merger.insertModel(rootModelNode);
            }
        }

        if (parentQmlItemNode.hasDefaultProperty())
            parentQmlItemNode.nodeAbstractProperty(parentQmlItemNode.defaultProperty()).reparentHere(newQmlItemNode);

        if (!newQmlItemNode.isValid())
            return newQmlItemNode;

        QString id;
        int i = 1;
        QString name(itemLibraryEntry.name().toLower());
        //remove forbidden characters
        name.replace(QRegExp(QLatin1String("[^a-zA-Z0-9_]")), QLatin1String("_"));
        do {
            id = name + QString::number(i);
            i++;
        } while (hasId(id)); //If the name already exists count upwards

        newQmlItemNode.setId(id);

        if (!currentState().isBaseState()) {
            newQmlItemNode.modelNode().variantProperty("opacity") = 0;
            newQmlItemNode.setVariantProperty("opacity", 1);
        }

        Q_ASSERT(newQmlItemNode.isValid());
    }
    catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
    }
    catch (InvalidIdException &e) {
        // should never happen
        QMessageBox::warning(0, tr("Invalid Id"), e.description());
    }

    Q_ASSERT(newQmlItemNode.isValid());

    return newQmlItemNode;
}

QmlItemNode QmlModelView::rootQmlItemNode() const
{
    return QmlItemNode(rootModelNode());
}

NodeInstance QmlModelView::instanceForModelNode(const ModelNode &modelNode)
{
    return nodeInstanceView()->instanceForNode(modelNode);
}

bool QmlModelView::hasInstanceForModelNode(const ModelNode &modelNode)
{
    return nodeInstanceView() && nodeInstanceView()->hasInstanceForNode(modelNode);
}

void QmlModelView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
}

void QmlModelView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

ModelNode QmlModelView::createQmlState(const QmlDesigner::PropertyListType &propertyList)
{

    QTC_CHECK(rootModelNode().majorQtQuickVersion() < 3);

    if (rootModelNode().majorQtQuickVersion() > 1)
        return createModelNode("QtQuick.State", 2, 0, propertyList);
    else
        return createModelNode("QtQuick.State", 1, 0, propertyList);
}


} //QmlDesigner
