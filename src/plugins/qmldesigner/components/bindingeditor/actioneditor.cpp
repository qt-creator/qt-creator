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

#include "actioneditor.h"

#include <qmldesignerplugin.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <bindingeditor/actioneditordialog.h>

#include <metainfo.h>
#include <qmlmodelnodeproxy.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <propertyeditorvalue.h>

#include <bindingproperty.h>
#include <variantproperty.h>

#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsvalueowner.h>

static Q_LOGGING_CATEGORY(ceLog, "qtc.qmldesigner.connectioneditor", QtWarningMsg)

namespace QmlDesigner {

static ActionEditor *s_lastActionEditor = nullptr;

ActionEditor::ActionEditor(QObject *)
    : m_index(QModelIndex())
{
}

ActionEditor::~ActionEditor()
{
    hideWidget();
}

void ActionEditor::registerDeclarativeType()
{
    qmlRegisterType<ActionEditor>("HelperWidgets", 2, 0, "ActionEditor");
}

void ActionEditor::prepareDialog()
{
    if (s_lastActionEditor)
        s_lastActionEditor->hideWidget();

    s_lastActionEditor = this;

    m_dialog = new ActionEditorDialog(Core::ICore::dialogParent());

    QObject::connect(m_dialog, &AbstractEditorDialog::accepted,
                     this, &ActionEditor::accepted);
    QObject::connect(m_dialog, &AbstractEditorDialog::rejected,
                     this, &ActionEditor::rejected);

    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
}

void ActionEditor::showWidget()
{
    prepareDialog();
    m_dialog->showWidget();
}

void ActionEditor::showWidget(int x, int y)
{
    prepareDialog();
    m_dialog->showWidget(x, y);
}

void ActionEditor::hideWidget()
{
    if (s_lastActionEditor == this)
        s_lastActionEditor = nullptr;

    if (m_dialog) {
        m_dialog->unregisterAutoCompletion(); // we have to do it separately, otherwise we have an autocompletion action override
        m_dialog->close();
    }
}

QString ActionEditor::connectionValue() const
{
    if (!m_dialog)
        return {};

    return m_dialog->editorValue();
}

void ActionEditor::setConnectionValue(const QString &text)
{
    if (m_dialog)
        m_dialog->setEditorValue(text);
}

bool ActionEditor::hasModelIndex() const
{
    return m_index.isValid();
}

void ActionEditor::resetModelIndex()
{
    m_index = QModelIndex();
}

QModelIndex ActionEditor::modelIndex() const
{
    return m_index;
}

void ActionEditor::setModelIndex(const QModelIndex &index)
{
    m_index = index;
}

void ActionEditor::setModelNode(const ModelNode &modelNode)
{
    if (modelNode.isValid())
        m_modelNode = modelNode;
}

bool isLiteral(QmlJS::AST::Node *ast)
{
    if (QmlJS::AST::cast<QmlJS::AST::StringLiteral *>(ast)
        || QmlJS::AST::cast<QmlJS::AST::NumericLiteral *>(ast)
        || QmlJS::AST::cast<QmlJS::AST::TrueLiteral *>(ast)
        || QmlJS::AST::cast<QmlJS::AST::FalseLiteral *>(ast))
        return true;
    else
        return false;
}

void ActionEditor::prepareConnections()
{
    if (!m_modelNode.isValid())
        return;

    BindingEditorWidget *bindingEditorWidget = m_dialog->bindingEditorWidget();

    if (!bindingEditorWidget) {
        qCInfo(ceLog) << Q_FUNC_INFO << "BindingEditorWidget is missing!";
        return;
    }

    if (!bindingEditorWidget->qmlJsEditorDocument()) {
        qCInfo(ceLog) << Q_FUNC_INFO << "QmlJsEditorDocument is missing!";
        return;
    }

    // Prepare objects for analysing slots
    const QmlJSTools::SemanticInfo &semanticInfo = bindingEditorWidget->qmljsdocument->semanticInfo();
    const QList<QmlJS::AST::Node *> path = semanticInfo.rangePath(0);
    const QmlJS::ContextPtr &context = semanticInfo.context;
    const QmlJS::ScopeChain &scopeChain = semanticInfo.scopeChain(path);

    static QList<TypeName> typeWhiteList({"string",
                                          "real", "int", "double",
                                          "bool",
                                          "QColor", "color",
                                          "QtQuick.Item", "QQuickItem"});

    static QList<PropertyName> methodBlackList({"toString", "destroy"});

    QList<ActionEditorDialog::ConnectionOption> connections;
    QList<ActionEditorDialog::SingletonOption> singletons;
    QStringList states;

    const QList<QmlDesigner::ModelNode> allNodes = m_modelNode.view()->allModelNodes();
    for (const auto &modelNode : allNodes) {
        // Skip nodes without specified id
        if (!modelNode.hasId())
            continue;

        ActionEditorDialog::ConnectionOption connection(modelNode.id());

        for (const auto &propertyName : modelNode.metaInfo().propertyNames()) {
            if (!typeWhiteList.contains(modelNode.metaInfo().propertyTypeName(propertyName)))
                continue;

            const QString name = QString::fromUtf8(propertyName);
            TypeName type = modelNode.metaInfo().propertyTypeName(propertyName);
            if (type.contains("<cpp>."))
                type.remove(0, 6);

            connection.properties.append(ActionEditorDialog::PropertyOption(name, type));
        }

        for (const VariantProperty &variantProperty : modelNode.variantProperties()) {
            if (variantProperty.isValid()) {
                if (variantProperty.isDynamic()) {
                    if (!typeWhiteList.contains(variantProperty.dynamicTypeName()))
                        continue;

                    const QString name = QString::fromUtf8(variantProperty.name());
                    TypeName type = variantProperty.dynamicTypeName();
                    if (type.contains("<cpp>."))
                        type.remove(0, 6);

                    connection.properties.append(ActionEditorDialog::PropertyOption(name, type));
                }
            }
        }

        for (const auto &slotName : modelNode.metaInfo().slotNames()) {
            if (slotName.startsWith("q_") || slotName.startsWith("_q_"))
                continue;

            QmlJS::Document::MutablePtr newDoc = QmlJS::Document::create(
                        QLatin1String("<expression>"), QmlJS::Dialect::JavaScript);
            newDoc->setSource(modelNode.id() + "." + QLatin1String(slotName));
            newDoc->parseExpression();

            QmlJS::AST::ExpressionNode *expression = newDoc->expression();

            if (expression && !isLiteral(expression)) {
                QmlJS::ValueOwner *interp = context->valueOwner();
                const QmlJS::Value *value = interp->convertToObject(scopeChain.evaluate(expression));

                if (const QmlJS::FunctionValue *f = value->asFunctionValue()) {
                    // Only add slots with zero arguments
                    if (f->namedArgumentCount() == 0 && !methodBlackList.contains(slotName))
                        connection.methods.append(QString::fromUtf8(slotName));
                }
            }
        }

        connection.methods.removeDuplicates();
        connections.append(connection);
    }

    // Singletons
    if (RewriterView *rv = m_modelNode.view()->rewriterView()) {
        for (const QmlTypeData &data : rv->getQMLTypes()) {
            if (!data.typeName.isEmpty()) {
                NodeMetaInfo metaInfo = m_modelNode.view()->model()->metaInfo(data.typeName.toUtf8());
                if (metaInfo.isValid()) {
                    ActionEditorDialog::SingletonOption singelton;
                    for (const PropertyName &propertyName : metaInfo.propertyNames()) {
                        TypeName type = metaInfo.propertyTypeName(propertyName);

                        if (!typeWhiteList.contains(type))
                            continue;

                        const QString name = QString::fromUtf8(propertyName);
                        if (type.contains("<cpp>."))
                            type.remove(0, 6);

                        singelton.properties.append(ActionEditorDialog::PropertyOption(name, type));
                    }

                    if (!singelton.properties.isEmpty()) {
                        singelton.item = data.typeName;
                        singletons.append(singelton);
                    }
                }
            }
        }
    }

    // States
    for (const QmlModelState &state : QmlItemNode(m_modelNode.view()->rootModelNode()).states().allStates())
        states.append(state.name());

    if (!connections.isEmpty() && !m_dialog.isNull())
        m_dialog->setAllConnections(connections, singletons, states);
}

void ActionEditor::updateWindowName(const QString &targetName)
{
    if (!m_dialog.isNull()) {
        if (targetName.isEmpty())
            m_dialog->setWindowTitle(m_dialog->defaultTitle());
        else
            m_dialog->setWindowTitle(m_dialog->defaultTitle() + " [" + targetName + "]");
        m_dialog->raise();
    }
}

} // QmlDesigner namespace
