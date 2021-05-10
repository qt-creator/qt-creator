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

#include "formatoperation.h"
#include "utils/fileutils.h"

#include <coreplugin/icore.h>
#include <qmlobjectnode.h>
#include <nodemetainfo.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace QmlDesigner {
namespace FormatOperation{

void readFormatConfiguration(){

    if (copyableProperties.isEmpty()){
        QString source = "formatconfiguration.json";
        Utils::FilePath path = Core::ICore::resourcePath() + "/qmldesigner/" + source;
        QString errorString;
        Utils::FileReader reader;

       if (reader.fetch(path, &errorString)){
           QJsonParseError jsonError;
           QJsonDocument document =   QJsonDocument::fromJson(reader.data(), &jsonError );

           if (jsonError.error != QJsonParseError::NoError)
               return;

           QJsonObject jsonObject = document.object();
           QVariantMap rootMap = jsonObject.toVariantMap();
           QJsonArray jsonArray = rootMap["propertylist"].toJsonArray();

           for (int i=0; i< jsonArray.count(); ++i){
               auto item = jsonArray.at(i).toObject();
               QVariantMap itemMap = item.toVariantMap();
               StylePropertyStruct current;
               current.id = itemMap["id"].toString();
               QVariantList subclassVariantList = itemMap["subclasses"].toList();
               QStringList subclassList;

               for (auto subclass : subclassVariantList)
                   subclassList.append(subclass.toString());

               current.subclasses = subclassList;

               QVariantList propertyList = itemMap["properties"].toList();
               QStringList propertyStringList;

               for (auto property : propertyList)
                   propertyStringList.append(property.toString());

               current.properties = propertyStringList;
               copyableProperties.append(current);
           }
        }
    }
}

bool propertiesCopyable(const SelectionContext &selectionState)
{
    if (!selectionState.singleNodeIsSelected())
        return false;

    readFormatConfiguration();

    ModelNode modelNode = selectionState.currentSingleSelectedNode();

    for (StylePropertyStruct copyable : copyableProperties)
        for (QString copyableSubclass : copyable.subclasses)
            if (modelNode.metaInfo().isSubclassOf(copyableSubclass.toUtf8()))
                return true;

    return false;
}

bool propertiesApplyable(const SelectionContext &selectionState)
{
    if (selectionState.selectedModelNodes().isEmpty())
        return false;

    if (chosenItem.subclasses.isEmpty())
        return false;

    const ModelNode firstSelectedNode = selectionState.firstSelectedModelNode();
    bool found = false;

    for (QString copyableSubclass : chosenItem.subclasses){
        if (firstSelectedNode.metaInfo().isSubclassOf(copyableSubclass.toUtf8())){
            found = true;
            break;
        }
    }

    if (!found)
        return false;

    for (const ModelNode &modelNode : selectionState.selectedModelNodes()){
        found = false;

        for (QString subclass : chosenItem.subclasses)
            if (modelNode.metaInfo().isSubclassOf(subclass.toUtf8())){
                found = true;
                break;
            }

        if (found)
            continue;

        return false;
    }

    return true;
}

void copyFormat(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|copyFormat",[selectionState](){

        applyableProperties.clear();

        ModelNode node = selectionState.currentSingleSelectedNode();
        QStringList propertyList;
        for (StylePropertyStruct copyable : copyableProperties){
            bool found = false;
            for (QString copyableSubclass : copyable.subclasses)
                if (node.metaInfo().isSubclassOf(copyableSubclass.toUtf8())){
                    propertyList = copyable.properties;
                    chosenItem = copyable;
                    found = true;
                    break;
                }
            if (found)
                break;
         }

        QmlObjectNode qmlObjectNode(node);

        for (auto propertyName : propertyList){
            if (qmlObjectNode.propertyAffectedByCurrentState(propertyName.toUtf8())) {
                StyleProperties property;
                property.propertyName = propertyName.toUtf8();
                property.value = qmlObjectNode.modelValue(propertyName.toUtf8());
                applyableProperties.append(property);
            }
        }
    });
}

void applyFormat(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|applyFormat",[selectionState](){

        for (ModelNode node : selectionState.selectedModelNodes()) {
            QmlObjectNode qmlObjectNode(node);
            QStringList propertyList;

            for (StylePropertyStruct copyable : copyableProperties){
                bool found = false;
                for (QString copyableSubclass : copyable.subclasses)
                    if (node.metaInfo().isSubclassOf(copyableSubclass.toUtf8())){
                        propertyList = copyable.properties;
                        found = true;
                        break;
                    }
                if (found)
                    break;
            }

            for (auto propertyName : propertyList)
                if (qmlObjectNode.propertyAffectedByCurrentState(propertyName.toUtf8()))
                    qmlObjectNode.removeProperty(propertyName.toUtf8());

            for (StyleProperties currentProperty : applyableProperties)
                if (node.metaInfo().hasProperty((currentProperty.propertyName)))
                    qmlObjectNode.setVariantProperty(currentProperty.propertyName, currentProperty.value);
        }
    });
}

} // namespace FormatOperation
} // QmlDesigner
