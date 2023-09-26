// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include <tuple>

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

void ActionEditor::showControls(bool show)
{
    if (m_dialog)
        m_dialog->showControls(show);
}

void QmlDesigner::ActionEditor::setMultilne(bool multiline)
{
    if (m_dialog)
        m_dialog->setMultiline(multiline);
}

QString ActionEditor::connectionValue() const
{
    if (!m_dialog)
        return {};

    QString value = m_dialog->editorValue().trimmed();

    //using parsed qml for unenclosed multistring (QDS-10681)
    const QString testingString = QString("Item { \n"
                                          " onWidthChanged: %1 \n"
                                          "}")
                                      .arg(value);

    QmlJS::Document::MutablePtr firstAttemptDoc = QmlJS::Document::create({},
                                                                          QmlJS::Dialect::QmlQtQuick2);
    firstAttemptDoc->setSource(testingString);
    firstAttemptDoc->parseQml();

    if (!firstAttemptDoc->isParsedCorrectly()) {
        const QString testingString2 = QString("Item { \n"
                                               " onWidthChanged: { \n"
                                               "  %1 \n"
                                               " } \n"
                                               "} \n")
                                           .arg(value);

        QmlJS::Document::MutablePtr secondAttemptDoc = QmlJS::Document::create({},
                                                                               QmlJS::Dialect::QmlQtQuick2);
        secondAttemptDoc->setSource(testingString2);
        secondAttemptDoc->parseQml();

        if (secondAttemptDoc->isParsedCorrectly())
            return QString("{\n%1\n}").arg(value);
    }

    return value;
}

void ActionEditor::setConnectionValue(const QString &text)
{
    if (m_dialog)
        m_dialog->setEditorValue(text);
}

QString ActionEditor::rawConnectionValue() const
{
    if (!m_dialog)
        return {};

    return m_dialog->editorValue();
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

namespace {

bool isLiteral(QmlJS::AST::Node *ast)
{
    if (QmlJS::AST::cast<QmlJS::AST::StringLiteral *>(ast)
        || QmlJS::AST::cast<QmlJS::AST::NumericLiteral *>(ast)
        || QmlJS::AST::cast<QmlJS::AST::TrueLiteral *>(ast)
        || QmlJS::AST::cast<QmlJS::AST::FalseLiteral *>(ast))
        return true;
    return false;
}

TypeName skipCpp(TypeName typeName)
{
    // TODO remove after project storage introduction

    if (typeName.contains("<cpp>."))
        typeName.remove(0, 6);

    return typeName;
}

} // namespace
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

    constexpr auto typeWhiteList = std::make_tuple(
        "string", "real", "int", "double", "bool", "QColor", "color", "QtQuick.Item", "QQuickItem");

    auto isSkippedType = [](auto &&type) {
        return !(type.isString() || type.isInteger() || type.isBool() || type.isColor()
                 || type.isFloat() || type.isQtObject());
    };
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

        for (const auto &property : modelNode.metaInfo().properties()) {
            if (isSkippedType(property.propertyType()))
                continue;

            connection.properties.append(
                ActionEditorDialog::PropertyOption(QString::fromUtf8(property.name()),
                                                   skipCpp(property.propertyType().typeName()),
                                                   property.isWritable()));
        }

        for (const VariantProperty &variantProperty : modelNode.variantProperties()) {
            if (variantProperty.isValid() && variantProperty.isDynamic()) {
                if (!variantProperty.hasDynamicTypeName(typeWhiteList))
                    continue;

                const QString name = QString::fromUtf8(variantProperty.name());
                const bool writeable = modelNode.metaInfo().property(variantProperty.name()).isWritable();

                connection.properties.append(
                    ActionEditorDialog::PropertyOption(name,
                                                       skipCpp(variantProperty.dynamicTypeName()),
                                                       writeable));
            }
        }

        for (const auto &slotName : modelNode.metaInfo().slotNames()) {
            if (slotName.startsWith("q_") || slotName.startsWith("_q_"))
                continue;

            QmlJS::Document::MutablePtr newDoc
                = QmlJS::Document::create(Utils::FilePath::fromString("<expression>"),
                                          QmlJS::Dialect::JavaScript);
            newDoc->setSource(modelNode.id() + "." + QLatin1String(slotName));
            newDoc->parseExpression();

            QmlJS::AST::ExpressionNode *expression = newDoc->expression();

            if (expression && !isLiteral(expression)) {
                if (QmlJS::ValueOwner *interp = context->valueOwner()) {
                    if (const QmlJS::Value *value = interp->convertToObject(
                            scopeChain.evaluate(expression))) {
                        if (value->asNullValue() && !methodBlackList.contains(slotName))
                            connection.methods.append(QString::fromUtf8(slotName));

                        if (const QmlJS::FunctionValue *f = value->asFunctionValue()) {
                            // Only add slots with zero arguments
                            if (f->namedArgumentCount() == 0 && !methodBlackList.contains(slotName))
                                connection.methods.append(QString::fromUtf8(slotName));
                        }
                    }
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
                    for (const auto &property : metaInfo.properties()) {
                        if (isSkippedType(property.propertyType()))
                            continue;

                        singelton.properties.append(ActionEditorDialog::PropertyOption(
                            QString::fromUtf8(property.name()),
                            skipCpp(property.propertyType().typeName()),
                            property.isWritable()));
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

void ActionEditor::invokeEditor(SignalHandlerProperty signalHandler,
                                std::function<void(SignalHandlerProperty)> removeSignalFunction,
                                bool removeOnReject,
                                QObject * parent)
{
    if (!signalHandler.isValid())
        return;

    ModelNode connectionNode = signalHandler.parentModelNode();
    if (!connectionNode.isValid())
        return;

    if (!connectionNode.bindingProperty("target").isValid())
        return;

    ModelNode targetNode = connectionNode.bindingProperty("target").resolveToModelNode();
    if (!targetNode.isValid())
        return;

    const QString source = signalHandler.source();

    QPointer<ActionEditor> editor = new ActionEditor(parent);

    editor->showWidget();
    editor->setModelNode(connectionNode);
    editor->setConnectionValue(source);
    editor->prepareConnections();
    editor->updateWindowName(targetNode.validId() + "." + signalHandler.name());

    QObject::connect(editor, &ActionEditor::accepted, [=]() {
        if (!editor)
            return;
        if (editor->m_modelNode.isValid()) {
            editor->m_modelNode.view()
                ->executeInTransaction("ActionEditor::"
                                       "invokeEditorAccepted",
                                       [=]() {
                                           if (!editor)
                                               return;

                                           const QString newSource = editor->connectionValue();
                                           if ((newSource.isNull() || newSource.trimmed().isEmpty())
                                               && removeSignalFunction) {
                                               removeSignalFunction(signalHandler);
                                           } else {
                                               editor->m_modelNode
                                                   .signalHandlerProperty(signalHandler.name())
                                                   .setSource(newSource);
                                           }
                                       });
        }

        //closing editor widget somewhy triggers rejected() signal. Lets disconect before it affects us:
        editor->disconnect();
        editor->deleteLater();
    });

    QObject::connect(editor, &ActionEditor::rejected, [=]() {
        if (!editor)
            return;

        if (removeOnReject && removeSignalFunction) {
            editor->m_modelNode.view()->executeInTransaction("ActionEditor::"
                                                             "invokeEditorOnRejectFunc",
                                                             [=]() { removeSignalFunction(signalHandler); });
        }

        //closing editor widget somewhy triggers rejected() signal 2nd time. Lets disconect before it affects us:
        editor->disconnect();
        editor->deleteLater();
    });
}

} // QmlDesigner namespace
