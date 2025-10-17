// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bindingeditor.h"

#include <qmldesignerplugin.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <bindingeditor/bindingeditordialog.h>
#include <qmldesignerconstants.h>

#include <qmlmodelnodeproxy.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <propertyeditorvalue.h>

#include <bindingproperty.h>
#include <variantproperty.h>

namespace QmlDesigner {

BindingEditor::BindingEditor(QObject *)
{
}

BindingEditor::~BindingEditor()
{
    hideWidget();
}

void BindingEditor::registerDeclarativeType()
{
    qmlRegisterType<BindingEditor>("HelperWidgets", 2, 0, "BindingEditor");
}

void BindingEditor::prepareDialog()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_BINDINGEDITOR_OPENED);

    m_dialog = Utils::makeUniqueObjectPtr<BindingEditorDialog>(Core::ICore::dialogParent());

    QObject::connect(m_dialog.get(), &AbstractEditorDialog::accepted, this, &BindingEditor::accepted);
    QObject::connect(m_dialog.get(), &AbstractEditorDialog::rejected, this, &BindingEditor::rejected);

    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
}

void BindingEditor::showWidget()
{
    prepareDialog();
    m_dialog->showWidget();
}

void BindingEditor::showWidget(int x, int y)
{
    prepareDialog();
    m_dialog->showWidget(x, y);
}

void BindingEditor::hideWidget()
{
    if (m_dialog) {
        m_dialog->unregisterAutoCompletion(); //we have to do it separately, otherwise we have an autocompletion action override
        m_dialog->close();
    }
}

QString BindingEditor::bindingValue() const
{
    if (!m_dialog)
        return {};

    return m_dialog->editorValue();
}

void BindingEditor::setBindingValue(const QString &text)
{
    if (m_dialog)
        m_dialog->setEditorValue(text);
}

void BindingEditor::setBackendValue(const QVariant &backendValue)
{
    if (!backendValue.isNull() && backendValue.isValid()) {
        m_backendValue = backendValue;
        const QObject *backendValueObj = backendValue.value<QObject*>();
        const PropertyEditorValue *propertyEditorValue = qobject_cast<const PropertyEditorValue *>(backendValueObj);
        const ModelNode node = propertyEditorValue->modelNode();

        if (node.isValid()) {
            m_backendValueType = node.metaInfo().property(propertyEditorValue->name()).propertyType();

            QString nodeId = node.displayName();

            m_targetName = nodeId + "." + propertyEditorValue->name();

            if (!m_backendValueType) {
                AbstractProperty prop = node.property(propertyEditorValue->name());
                if (prop.isDynamic()) {
                    TypeName typeName = prop.dynamicTypeName();
                    // Alias as is a special case as it has no separate metaInfo for it
                    if (typeName == "alias")
                        m_isAlias = true;
                    else
                        m_backendValueType = node.model()->metaInfo(typeName);
                }
            }

            if (!m_isAlias && !m_backendValueType) {
                if (QmlObjectNode::isValidQmlObjectNode(node)) {
                    m_backendValueType = node.model()->metaInfo(
                        QmlObjectNode(node).instanceType(propertyEditorValue->name()));
                }
            }
        }

        emit backendValueChanged();
    }
}

void BindingEditor::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    if (!modelNodeBackend.isNull() && modelNodeBackend.isValid()) {
        m_modelNodeBackend = modelNodeBackend;

        const auto modelNodeBackendObject = m_modelNodeBackend.value<QObject*>();

        const auto backendObjectCasted =
                qobject_cast<const QmlDesigner::QmlModelNodeProxy *>(modelNodeBackendObject);

        if (backendObjectCasted)
            m_modelNode = backendObjectCasted->qmlObjectNode().modelNode();

        emit modelNodeBackendChanged();
    }
}

void BindingEditor::setStateModelNode(const QVariant &stateModelNode)
{
    if (stateModelNode.isValid()) {
        m_stateModelNode = stateModelNode;
        m_modelNode = m_stateModelNode.value<QmlDesigner::ModelNode>();

        if (m_modelNode.isValid())
            m_backendValueType = m_modelNode.model()->boolMetaInfo();

        emit stateModelNodeChanged();
    }
}

void BindingEditor::setStateName(const QString &name)
{
    m_targetName = name;
    m_targetName += ".when";
}

void BindingEditor::setModelNode(const ModelNode &modelNode)
{
    if (modelNode.isValid())
        m_modelNode = modelNode;
}

void BindingEditor::setBackendValueType(const NodeMetaInfo &backendValueType)
{
    m_backendValueType = backendValueType;

    emit backendValueChanged();
}

void BindingEditor::setTargetName(const QString &target)
{
    m_targetName = target;
}

namespace {

bool compareTypes(const NodeMetaInfo &sourceType, const NodeMetaInfo &targetType)
{
    return targetType.isVariant() || sourceType.isVariant()
           || (targetType.isNumber() && sourceType.isNumber()) || sourceType.isBasedOn(targetType);
}
} // namespace

void BindingEditor::prepareBindings()
{
    if (!m_modelNode.isValid() || (!m_backendValueType && !m_isAlias))
        return;

    const QList<QmlDesigner::ModelNode> allNodes = m_modelNode.view()->allModelNodes();

    QList<BindingEditorDialog::BindingOption> bindings;

    for (const auto &objnode : allNodes) {
        BindingEditorDialog::BindingOption binding;
        for (const auto &property :
             MetaInfoUtils::addInflatedValueAndReferenceProperties(objnode.metaInfo().properties())) {
            const auto &propertyType = property.property.propertyType();

            if (m_isAlias || compareTypes(m_backendValueType, propertyType))
                binding.properties.append(QString::fromUtf8(property.name()));
        }

        //dynamic properties:
        for (const BindingProperty &bindingProperty : objnode.bindingProperties()) {
            if (bindingProperty.isValid()) {
                if (bindingProperty.isDynamic()) {
                    auto model = bindingProperty.model();
                    const auto dynamicType = model->metaInfo(bindingProperty.dynamicTypeName());
                    if (m_isAlias || compareTypes(m_backendValueType, dynamicType))
                        binding.properties.append(QString::fromUtf8(bindingProperty.name()));
                }
            }
        }
        for (const VariantProperty &variantProperty : objnode.variantProperties()) {
            if (variantProperty.isValid()) {
                if (variantProperty.isDynamic()) {
                    auto model = variantProperty.model();
                    const auto dynamicType = model->metaInfo(variantProperty.dynamicTypeName());
                    if (m_isAlias || compareTypes(m_backendValueType, dynamicType))
                        binding.properties.append(QString::fromUtf8(variantProperty.name()));
                }
            }
        }

        if (!binding.properties.isEmpty() && objnode.hasId()) {
            binding.item = objnode.displayName();
            bindings.append(binding);
        }
    }

    //singletons:
    if (auto model = m_modelNode.view()->model()) {
        for (const auto &metaInfo : model->singletonMetaInfos()) {
            BindingEditorDialog::BindingOption binding;

            for (const auto &property : metaInfo.properties()) {
                const auto propertyType = property.propertyType();

                if (m_isAlias || compareTypes(m_backendValueType, propertyType))
                    binding.properties.append(QString::fromUtf8(property.name()));
            }

            if (!binding.properties.isEmpty()) {
                binding.item = model->exportedTypeNameForMetaInfo(metaInfo).name.toQString();
                if (binding.item.size())
                    bindings.append(binding);
            }
        }
    }

    for (auto &binding : bindings) {
        std::ranges::sort(binding.properties);
        auto removed = std::ranges::unique(binding.properties);
        binding.properties.erase(removed.begin(), removed.end());
    }

    if (!bindings.isEmpty() && m_dialog)
        m_dialog->setAllBindings(bindings, m_backendValueType);
}

void BindingEditor::updateWindowName()
{
    if (m_dialog && (m_isAlias || m_backendValueType)) {
        QString targetString;
        QString typeName;
        if (m_isAlias) {
            typeName = "alias";
        } else {
            auto exportedTypeName = m_modelNode.model()->exportedTypeNameForMetaInfo(m_backendValueType);
            if (exportedTypeName.name.size())
                typeName = exportedTypeName.name.toQString();
        }
        targetString = " [" + (m_targetName.isEmpty() ? QString() : (m_targetName + ": "))
                       + typeName + "]";

        m_dialog->setWindowTitle(m_dialog->defaultTitle() + targetString);
    }
}

QString BindingEditor::targetName() const
{
    return m_targetName;
}

QString BindingEditor::stateName() const
{
    if (m_targetName.endsWith(".when"))
        return m_targetName.chopped(5);

    return {};
}

QVariant BindingEditor::backendValue() const
{
    return m_backendValue;
}

QVariant BindingEditor::modelNodeBackend() const
{
    return m_modelNodeBackend;
}

QVariant BindingEditor::stateModelNode() const
{
    return m_stateModelNode;
}

} // QmlDesigner namespace
