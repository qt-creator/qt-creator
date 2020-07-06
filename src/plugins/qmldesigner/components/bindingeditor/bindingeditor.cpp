/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "bindingeditor.h"

#include <qmldesignerplugin.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <metainfo.h>
#include <qmlmodelnodeproxy.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <propertyeditorvalue.h>

#include <bindingproperty.h>
#include <variantproperty.h>

namespace QmlDesigner {

static BindingEditor *s_lastBindingEditor = nullptr;

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
    if (s_lastBindingEditor)
        s_lastBindingEditor->hideWidget();
    s_lastBindingEditor = this;

    m_dialog = new BindingEditorDialog(Core::ICore::dialogParent());


    QObject::connect(m_dialog, &BindingEditorDialog::accepted,
                     this, &BindingEditor::accepted);
    QObject::connect(m_dialog, &BindingEditorDialog::rejected,
                     this, &BindingEditor::rejected);

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
    if (s_lastBindingEditor == this)
        s_lastBindingEditor = nullptr;
    if (m_dialog)
    {
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

        if (node.isValid())
        {
            m_backendValueTypeName = node.metaInfo().propertyTypeName(propertyEditorValue->name());

            if (m_backendValueTypeName == "alias" || m_backendValueTypeName == "unknown")
                if (QmlObjectNode::isValidQmlObjectNode(node))
                    m_backendValueTypeName = QmlObjectNode(node).instanceType(propertyEditorValue->name());
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

        if (backendObjectCasted) {
            m_modelNode = backendObjectCasted->qmlObjectNode().modelNode();
        }

        emit modelNodeBackendChanged();
    }
}

void BindingEditor::setStateModelNode(const QVariant &stateModelNode)
{
    if (stateModelNode.isValid())
    {
        m_stateModelNode = stateModelNode;
        m_modelNode = m_stateModelNode.value<QmlDesigner::ModelNode>();

        if (m_modelNode.isValid())
            m_backendValueTypeName = "bool";

        emit stateModelNodeChanged();
    }
}

void BindingEditor::setModelNode(const ModelNode &modelNode)
{
    if (modelNode.isValid())
        m_modelNode = modelNode;
}

void BindingEditor::setBackendValueTypeName(const TypeName &backendValueTypeName)
{
    m_backendValueTypeName = backendValueTypeName;

    emit backendValueChanged();
}

void BindingEditor::prepareBindings()
{
    if (!m_modelNode.isValid() || m_backendValueTypeName.isEmpty())
        return;

    const QList<QmlDesigner::ModelNode> allNodes = m_modelNode.view()->allModelNodes();

    QList<BindingEditorDialog::BindingOption> bindings;

    const QList<TypeName> variantTypes = {"alias", "unknown", "variant", "var"};
    const QList<TypeName> numericTypes = {"double", "real", "int"};
    const QList<TypeName> colorTypes = {"QColor", "color"};
    auto isNumeric = [&numericTypes](TypeName compareType) { return numericTypes.contains(compareType); };
    auto isColor = [&colorTypes](TypeName compareType) { return colorTypes.contains(compareType); };

    const bool skipTypeFiltering = variantTypes.contains(m_backendValueTypeName);
    const bool targetTypeIsNumeric = isNumeric(m_backendValueTypeName);

    for (const auto &objnode : allNodes) {
        BindingEditorDialog::BindingOption binding;
        for (const auto &propertyName : objnode.metaInfo().propertyNames())
        {
            TypeName propertyTypeName = objnode.metaInfo().propertyTypeName(propertyName);

            if (skipTypeFiltering
                    || (m_backendValueTypeName == propertyTypeName)
                    || variantTypes.contains(propertyTypeName)
                    || (targetTypeIsNumeric && isNumeric(propertyTypeName))) {
                binding.properties.append(QString::fromUtf8(propertyName));
            }
        }

        //dynamic properties:
        for (const BindingProperty &bindingProperty : objnode.bindingProperties()) {
            if (bindingProperty.isValid()) {
                if (bindingProperty.isDynamic()) {
                    const TypeName dynamicTypeName = bindingProperty.dynamicTypeName();
                    if (skipTypeFiltering
                            || (dynamicTypeName == m_backendValueTypeName)
                            || variantTypes.contains(dynamicTypeName)
                            || (targetTypeIsNumeric && isNumeric(dynamicTypeName))) {
                        binding.properties.append(QString::fromUtf8(bindingProperty.name()));
                    }
                }
            }
        }
        for (const VariantProperty &variantProperty : objnode.variantProperties()) {
            if (variantProperty.isValid()) {
                if (variantProperty.isDynamic()) {
                    const TypeName dynamicTypeName = variantProperty.dynamicTypeName();
                    if (skipTypeFiltering
                            || (dynamicTypeName == m_backendValueTypeName)
                            || variantTypes.contains(dynamicTypeName)
                            || (targetTypeIsNumeric && isNumeric(dynamicTypeName))) {
                        binding.properties.append(QString::fromUtf8(variantProperty.name()));
                    }
                }
            }
        }

        if (!binding.properties.isEmpty() && objnode.hasId()) {
            binding.item = objnode.displayName();
            bindings.append(binding);
        }
    }

    //singletons:
    if (RewriterView* rv = m_modelNode.view()->rewriterView()) {
        for (const QmlTypeData &data : rv->getQMLTypes()) {
            if (!data.typeName.isEmpty()) {
                NodeMetaInfo metaInfo = m_modelNode.view()->model()->metaInfo(data.typeName.toUtf8());

                if (metaInfo.isValid()) {
                    BindingEditorDialog::BindingOption binding;

                    for (const PropertyName &propertyName : metaInfo.propertyNames()) {
                        TypeName propertyTypeName = metaInfo.propertyTypeName(propertyName);

                        if (skipTypeFiltering
                                || (m_backendValueTypeName == propertyTypeName)
                                || (variantTypes.contains(propertyTypeName))
                                || (targetTypeIsNumeric && isNumeric(propertyTypeName))
                                || (isColor(m_backendValueTypeName) && isColor(propertyTypeName))) {
                            binding.properties.append(QString::fromUtf8(propertyName));
                        }
                    }

                    if (!binding.properties.isEmpty()) {
                        binding.item = data.typeName;
                        bindings.append(binding);
                    }
                }
            }
        }
    }

    if (!bindings.isEmpty() && !m_dialog.isNull())
        m_dialog->setAllBindings(bindings);

    updateWindowName();
}

void BindingEditor::updateWindowName()
{
    if (!m_dialog.isNull() && !m_backendValueTypeName.isEmpty())
    {
        m_dialog->setWindowTitle(m_dialog->defaultTitle() + " [" + m_backendValueTypeName + "]");
    }
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
