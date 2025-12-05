// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formatoperation.h"
#include "componentcoretracing.h"

#include <qmlobjectnode.h>
#include <nodemetainfo.h>

#include <coreplugin/icore.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace QmlDesigner::FormatOperation {

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

namespace {
struct StylePropertyStruct
{
    QString id;
    QByteArray moduleName;
    QByteArrayList subclasses;
    QByteArrayList properties;
};

struct StyleProperties
{
    QmlDesigner::PropertyName propertyName;
    QVariant value;
};

QList<StylePropertyStruct> copyableProperties = {};
QList<StyleProperties> applyableProperties = {};
StylePropertyStruct chosenItem = {};

Module resolveModule(QHash<QByteArray, Module> &modules, Model *model, const QByteArray &moduleName)
{
    Module module;
    if (modules.contains(moduleName)) {
        module = modules[moduleName];
    } else {
        module = model->module(moduleName, Storage::ModuleKind::QmlLibrary);
        modules.insert(moduleName, module);
    }
    return module;
}

} // namespace

void readFormatConfiguration()
{
    NanotraceHR::Tracer tracer{"format operation read format configuration", category()};

    if (copyableProperties.isEmpty()) {
        QString source = "formatconfiguration.json";
        Utils::FilePath path = Core::ICore::resourcePath("qmldesigner") / source;
        Utils::Result<QByteArray> res = path.fileContents();

       if (res) {
           QJsonParseError jsonError;
           QJsonDocument document = QJsonDocument::fromJson(*res, &jsonError);

           if (jsonError.error != QJsonParseError::NoError)
               return;

           QJsonObject jsonObject = document.object();
           QVariantMap rootMap = jsonObject.toVariantMap();
           QJsonArray jsonArray = rootMap["propertylist"].toJsonArray();

           for (int i = 0; i < jsonArray.size(); ++i) {
               auto item = jsonArray.at(i).toObject();
               QVariantMap itemMap = item.toVariantMap();
               StylePropertyStruct current;
               current.id = itemMap["id"].toString();
               current.moduleName = itemMap["module"].toByteArray();
               const QVariantList subclassVariantList = itemMap["subclasses"].toList();
               QByteArrayList subclassList;

               for (const QVariant &subclass : subclassVariantList)
                   subclassList.append(subclass.toByteArray());

               current.subclasses = subclassList;

               const QVariantList propertyList = itemMap["properties"].toList();
               QByteArrayList propertyStringList;

               for (const QVariant &property : propertyList)
                   propertyStringList.append(property.toByteArray());

               current.properties = propertyStringList;
               copyableProperties.append(current);
           }
        }
    }
}

bool propertiesCopyable(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"format operation properties copyable", category()};

    if (!selectionState.singleNodeIsSelected() || !selectionState.model())
        return false;

    readFormatConfiguration();

    ModelNode modelNode = selectionState.currentSingleSelectedNode();

    QHash<QByteArray, Module> modules;

    for (const StylePropertyStruct &copyable : std::as_const(copyableProperties)) {
        Module module = resolveModule(modules, selectionState.model(), copyable.moduleName);
        for (const QByteArray &copyableSubclass : std::as_const(copyable.subclasses)) {
            if (modelNode.metaInfo().isBasedOn(modelNode.model()->metaInfo(module, copyableSubclass)))
                return true;
        }
    }
    return false;
}

bool propertiesApplyable(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"format operation properties applyable", category()};

    if (selectionState.selectedModelNodes().isEmpty() || !selectionState.model())
        return false;

    if (chosenItem.subclasses.isEmpty())
        return false;

    auto module = selectionState.model()->module(chosenItem.moduleName,
                                                 Storage::ModuleKind::QmlLibrary);

    const ModelNode firstSelectedNode = selectionState.firstSelectedModelNode();
    bool found = false;

    for (const QByteArray &copyableSubclass : std::as_const(chosenItem.subclasses)) {
        auto base = firstSelectedNode.model()->metaInfo(module, copyableSubclass);
        if (firstSelectedNode.metaInfo().isBasedOn(base)) {
            found = true;
            break;
        }
    }

    if (!found)
        return false;

    const QList<ModelNode> nodes = selectionState.selectedModelNodes();
    for (const ModelNode &modelNode : nodes) {
        found = false;

        for (const QByteArray &subclass : std::as_const(chosenItem.subclasses)) {
            auto base = modelNode.model()->metaInfo(module, subclass);
            if (modelNode.metaInfo().isBasedOn(base)) {
                found = true;
                break;
            }
        }

        if (found)
            continue;

        return false;
    }

    return true;
}

void copyFormat(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"format operation copy format", category()};

    if (!selectionState.view() || !selectionState.model())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|copyFormat", [selectionState]() {
        applyableProperties.clear();

        ModelNode node = selectionState.currentSingleSelectedNode();
        QByteArrayList propertyList;
        QHash<QByteArray, Module> modules;
        for (const StylePropertyStruct &copyable : std::as_const(copyableProperties)) {
            bool found = false;
            Module module = resolveModule(modules, selectionState.model(), copyable.moduleName);
            for (const QByteArray &copyableSubclass : std::as_const(copyable.subclasses)) {
                auto base = node.model()->metaInfo(module, copyableSubclass);
                if (node.metaInfo().isBasedOn(base)) {
                    propertyList = copyable.properties;
                    chosenItem = copyable;
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }

        QmlObjectNode qmlObjectNode(node);

        for (const QByteArray &propertyName : std::as_const(propertyList)) {
            if (qmlObjectNode.propertyAffectedByCurrentState(propertyName)) {
                StyleProperties property;
                property.propertyName = propertyName;
                property.value = qmlObjectNode.modelValue(propertyName);
                applyableProperties.append(property);
            }
        }
    });
}

void applyFormat(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"format operation apply format", category()};

    if (!selectionState.view() || !selectionState.model())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|applyFormat",[selectionState](){
        const QList<ModelNode> nodes = selectionState.selectedModelNodes();
        for (const ModelNode &node : nodes) {
            QmlObjectNode qmlObjectNode(node);
            QByteArrayList propertyList;
            QHash<QByteArray, Module> modules;

            for (const StylePropertyStruct &copyable : std::as_const(copyableProperties)) {
                bool found = false;
                Module module = resolveModule(modules, selectionState.model(), copyable.moduleName);
                for (const QByteArray &copyableSubclass : std::as_const(copyable.subclasses)) {
                    auto base = node.model()->metaInfo(module, copyableSubclass);
                    if (node.metaInfo().isBasedOn(base)) {
                        propertyList = copyable.properties;
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }

            for (const QByteArray &propertyName : std::as_const(propertyList)) {
                if (qmlObjectNode.propertyAffectedByCurrentState(propertyName))
                    qmlObjectNode.removeProperty(propertyName);
            }

            for (const StyleProperties &currentProperty : std::as_const(applyableProperties)) {
                if (node.metaInfo().hasProperty((currentProperty.propertyName)))
                    qmlObjectNode.setVariantProperty(currentProperty.propertyName, currentProperty.value);
            }
        }
    });
}

} // namespace QmlDesigner::FormatOperation
