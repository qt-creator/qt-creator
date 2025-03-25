// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "transitioneditorview.h"

#include "transitioneditortoolbar.h"
#include "transitioneditorwidget.h"

#include "transitioneditorgraphicsscene.h"
#include "transitioneditorsettingsdialog.h"

#include <auxiliarydataproperties.h>
#include <bindingproperty.h>
#include <exception.h>
#include <modelnodecontextmenu_helper.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>
#include <variantproperty.h>
#include <viewmanager.h>
#include <qmldesignerconstants.h>
#include <qmldesignericons.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>
#include <qmlobjectnode.h>
#include <qmlstate.h>
#include <qmltimeline.h>
#include <qmltimelinekeyframegroup.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QTimer>

namespace QmlDesigner {

TransitionEditorView::TransitionEditorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_transitionEditorWidget(nullptr)
{

}

TransitionEditorView::~TransitionEditorView() = default;

void TransitionEditorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    if (m_transitionEditorWidget)
        m_transitionEditorWidget->init();
}

void TransitionEditorView::modelAboutToBeDetached(Model *model)
{
    m_transitionEditorWidget->reset();

    AbstractView::modelAboutToBeDetached(model);
}

void TransitionEditorView::nodeCreated(const ModelNode & /*createdNode*/) {}

void TransitionEditorView::nodeAboutToBeRemoved(const ModelNode & /*removedNode*/) {}

void TransitionEditorView::nodeRemoved(const ModelNode & removedNode,
                               const NodeAbstractProperty &parentProperty,
                               PropertyChangeFlags /*propertyChange*/)
{
    if (parentProperty.name() == "transitions")
        widget()->updateData(removedNode);

    const ModelNode parent = parentProperty.parentModelNode();
    if (parent.metaInfo().isQtQuickTransition())
        asyncUpdate(parent);
}

void TransitionEditorView::nodeReparented(const ModelNode &node,
                                          const NodeAbstractProperty &newPropertyParent,
                                          const NodeAbstractProperty & /*oldPropertyParent*/,
                                          AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (newPropertyParent.name() == "transitions")
        asyncUpdate(node);

    const ModelNode parent = newPropertyParent.parentModelNode();

    if (parent.metaInfo().isQtQuickTransition()) {
        asyncUpdate(parent);
    }
}

void TransitionEditorView::instancePropertyChanged(
    const QList<QPair<ModelNode, PropertyName>> & /*propertyList*/)
{

}

void TransitionEditorView::variantPropertiesChanged(
    const QList<VariantProperty> & /* propertyList */,
    AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void TransitionEditorView::bindingPropertiesChanged(
    const QList<BindingProperty> & /*propertyList */,
    AbstractView::PropertyChangeFlags /* propertyChange */)
{

}

void TransitionEditorView::selectedNodesChanged(const QList<ModelNode> & /*selectedNodeList*/,
                                        const QList<ModelNode> & /*lastSelectedNodeList*/)
{

}

void TransitionEditorView::auxiliaryDataChanged(const ModelNode &modelNode,
                                                AuxiliaryDataKeyView key,
                                                const QVariant &data)
{
    if (key == lockedProperty && data.toBool() && modelNode.isValid()) {
        for (const auto &node : modelNode.allSubModelNodesAndThisNode()) {
            if (node.hasAuxiliaryData(transitionExpandedPropery))
                m_transitionEditorWidget->graphicsScene()->invalidateHeightForTarget(node);
        }
    }
}

void TransitionEditorView::propertiesAboutToBeRemoved(
    const QList<AbstractProperty> & /*propertyList */)
{

}

void TransitionEditorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const AbstractProperty &property : propertyList) {
        if (property.name() == "transitions")
            widget()->init();
    }
}

bool TransitionEditorView::hasWidget() const
{
    return true;
}

void TransitionEditorView::nodeIdChanged(const ModelNode &node, const QString &, const QString &)
{
    if (node.metaInfo().isValid() && node.metaInfo().isQtQuickTransition())
        widget()->init();
}

void TransitionEditorView::currentStateChanged(const ModelNode &)
{

}

TransitionEditorWidget *TransitionEditorView::widget() const
{
    return m_transitionEditorWidget;
}

void TransitionEditorView::registerActions()
{

}

ModelNode TransitionEditorView::addNewTransition()
{
    const auto groupMetaInfo = model()->qtQuickStateGroupMetaInfo();

    auto stateGroups = allModelNodesOfType(groupMetaInfo);

    ModelNode node = addNewTransition(rootModelNode(), dotnotShowWarning);
    if (node.isValid())
        return node;

    for (const auto &stateGroup : stateGroups) {
        node = addNewTransition(stateGroup, dotnotShowWarning);
        if (node.isValid())
            return node;
    }

    showWarningNoProperties();

    return {};
}

void addAnimationsToTransition(const ModelNode &transition, const QHash<QString, QStringList> &idPropertyList)
{
    QTC_ASSERT(transition.isValid(), return);

    auto view = transition.view();
    for (auto it = idPropertyList.cbegin(); it != idPropertyList.cend(); ++it) {
        ModelNode parallelAnimation = view->createModelNode("QtQuick.ParallelAnimation");
        transition.defaultNodeAbstractProperty().reparentHere(parallelAnimation);
        for (const QString &property : it.value()) {
            ModelNode sequentialAnimation = view->createModelNode(
                "QtQuick.SequentialAnimation");
            parallelAnimation.defaultNodeAbstractProperty().reparentHere(
                sequentialAnimation);

#ifdef QDS_USE_PROJECTSTORAGE
            ModelNode pauseAnimation = view->createModelNode("PauseAnimation", {{"duration", 0}});
#else
            const NodeMetaInfo pauseMetaInfo = view->model()->metaInfo("QtQuick.PauseAnimation");

            ModelNode pauseAnimation = view->createModelNode("QtQuick.PauseAnimation",
                                                             pauseMetaInfo.majorVersion(),
                                                             pauseMetaInfo.minorVersion(),
                                                             {{"duration", 0}});
#endif
            sequentialAnimation.defaultNodeAbstractProperty().reparentHere(pauseAnimation);

#ifdef QDS_USE_PROJECTSTORAGE
            ModelNode propertyAnimation = view->createModelNode("PropertyAnimation",
                                                          {{"property", property},
                                                           {"duration", 150}});
#else
            const NodeMetaInfo propertyMetaInfo = view->model()->metaInfo("QtQuick.PauseAnimation");

            ModelNode propertyAnimation = view->createModelNode("QtQuick.PropertyAnimation",
                                                          propertyMetaInfo.majorVersion(),
                                                          propertyMetaInfo.minorVersion(),
                                                          {{"property", property},
                                                           {"duration", 150}});
#endif
            propertyAnimation.bindingProperty("target").setExpression(it.key());
            sequentialAnimation.defaultNodeAbstractProperty().reparentHere(
                propertyAnimation);
        }
    }
}

QHash<QString, QStringList> getPropertiesForStateGroup(const ModelNode &stateGroup)
{
    QList<QmlModelState> states;

    auto objectNode = QmlObjectNode(stateGroup);

    if (objectNode.isValid())
        states = objectNode.states().allStates();

    QHash<QString, QStringList> idPropertyList;

    for (const QmlModelState &state : std::as_const(states)) {
        for (const QmlPropertyChanges & change : state.propertyChanges()) {
            QStringList locList;
            const ModelNode target = change.target();
            if (auto targetMetaInfo = target.metaInfo()) {
                const QString targetId = target.id();
                for (const AbstractProperty &property : change.modelNode().properties()) {
                    auto type = targetMetaInfo.property(property.name()).propertyType();

                    if (type.isInteger() || type.isColor() || type.isFloat())
                        locList.append(QString::fromUtf8(property.name()));
                }
                if (idPropertyList.contains(targetId)) {
                    QStringList newlist = idPropertyList.value(targetId);
                    for (const QString  &str :locList)
                        if (!newlist.contains(str))
                            newlist.append(str);
                    idPropertyList.insert(targetId, newlist);
                } else {
                    if (!locList.isEmpty())
                        idPropertyList.insert(targetId, locList);
                }
            }
        }
    }

    return idPropertyList;
}

void TransitionEditorView::showWarningNoStates()
{
    Core::AsynchronousMessageBox::warning(tr("No States Defined"),
                                          tr("There are no states defined in this component."));
}

void TransitionEditorView::showWarningNoProperties()
{
    QString properties;
    const QVector<TypeName> validProperties = {
                                               "int", "real", "double", "qreal", "color", "QColor", "float"};
    for (const PropertyName &property : validProperties)
        properties.append(QString::fromUtf8(property) + ", ");
    if (!properties.isEmpty())
        properties.chop(2);
    Core::AsynchronousMessageBox::warning(
        tr("No Property Changes to Animate"),
        tr("To add transitions, first change the properties that you want to animate in states (%1).")
            .arg(properties));
}

void TransitionEditorView::resetTransitionToStateGroup(const ModelNode &transition, const ModelNode &stateGroup)
{
    QTC_ASSERT(transition.isValid() && stateGroup.isValid(), return);

    QTC_ASSERT(transition.metaInfo().isQtQuickTransition(), return);

    auto stateGroupObject = QmlObjectNode(stateGroup);
    QTC_ASSERT(stateGroupObject.isValid(), return);

    const ModelNode root = transition.view()->rootModelNode();

    auto states = stateGroupObject.states().allStates();

    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TRANSITION_ADDED);

    if (states.isEmpty()) {
        showWarningNoStates();
        return;
    }

    QHash<QString, QStringList> idPropertyList = getPropertiesForStateGroup(stateGroup);

    if (!idPropertyList.isEmpty()) {
        executeInTransaction(
            " TransitionEditorView::addNewTransition", [&transition, idPropertyList, root, stateGroup]() {
                for (auto &propertyName : transition.propertyNames())
                    transition.removeProperty(propertyName);

                transition.variantProperty("from").setValue("*");
                transition.variantProperty("to").setValue("*");

                stateGroup.nodeListProperty("transitions").reparentHere(transition);

                addAnimationsToTransition(transition, idPropertyList);
            });
    } else {
        showWarningNoProperties();
    }
}

ModelNode TransitionEditorView::addNewTransition(const ModelNode &stateGroup, ShowWarning warning)
{
    QList<QmlModelState> states;
    const ModelNode parentNode = stateGroup;

    auto objectNode = QmlObjectNode(stateGroup);

    if (objectNode.isValid())
        states = objectNode.states().allStates();

    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TRANSITION_ADDED);

    if (warning == showWarning && states.isEmpty()) {
        showWarningNoStates();
        return {};
    }

    QHash<QString, QStringList> idPropertyList = getPropertiesForStateGroup(stateGroup);

    ModelNode transition;

    if (!idPropertyList.isEmpty()) {
        executeInTransaction(
            " TransitionEditorView::addNewTransition",
            [&transition, idPropertyList, parentNode, this]() {

#ifdef QDS_USE_PROJECTSTORAGE
                transition = createModelNode("Transition",
                                             {{
                                                  "from",
                                                  "*",
                                              },
                                              {
                                                  "to",
                                                  "*",
                                              }});
#else
                const NodeMetaInfo transitionMetaInfo = model()->metaInfo("QtQuick.Transition");
                transition = createModelNode("QtQuick.Transition",
                                             transitionMetaInfo.majorVersion(),
                                             transitionMetaInfo.minorVersion(),
                                             {{
                                                  "from",
                                                  "*",
                                              },
                                              {
                                                  "to",
                                                  "*",
                                              }});
#endif
                transition.setAuxiliaryData(transitionDurationProperty, 2000);
                transition.ensureIdExists();
                parentNode.nodeListProperty("transitions").reparentHere(transition);

                addAnimationsToTransition(transition, idPropertyList);
            });
    } else {
        if (warning == showWarning)
            showWarningNoProperties();
    }

    if (m_transitionEditorWidget)
        m_transitionEditorWidget->init();

    return transition;
}

TransitionEditorWidget *TransitionEditorView::createWidget()
{
    if (!m_transitionEditorWidget)
        m_transitionEditorWidget = new TransitionEditorWidget(this);

    return m_transitionEditorWidget;
}

WidgetInfo TransitionEditorView::widgetInfo()
{
    return createWidgetInfo(createWidget(),
                            "TransitionEditor",
                            WidgetInfo::BottomPane,
                            tr("Transitions"),
                            tr("Transitions view"));
}

void TransitionEditorView::openSettingsDialog()
{
    auto dialog = new TransitionEditorSettingsDialog(Core::ICore::dialogParent(), this);

    auto transition = widget()->graphicsScene()->transitionModelNode();
    if (transition.isValid())
        dialog->setCurrentTransition(transition);

    QObject::connect(dialog, &TransitionEditorSettingsDialog::rejected, [this, dialog]() {
        widget()->init();
        dialog->deleteLater();
    });

    QObject::connect(dialog, &TransitionEditorSettingsDialog::accepted, [this, dialog]() {
        widget()->init();
        dialog->deleteLater();
    });

    dialog->show();
}

QList<ModelNode> TransitionEditorView::allTransitions() const
{
    const auto transitionMetaInfo = model()->qtQuickTransistionMetaInfo();

    return allModelNodesOfType(transitionMetaInfo);
}

void TransitionEditorView::asyncUpdate(const ModelNode &transition)
{
    static bool updateTriggered = false;

    if (!updateTriggered && (transition.id() == widget()->toolBar()->currentTransitionId())) {
        updateTriggered = true;
        QTimer::singleShot(0, [this, transition]() {
            widget()->updateData(transition);
            updateTriggered = false;
        });
    }
}

} // namespace QmlDesigner
