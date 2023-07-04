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

#include <designmodecontext.h>

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

    if (!isEnabled())
        return;

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
    QList<QmlModelState> states;
    const ModelNode root = rootModelNode();

    if (QmlVisualNode::isValidQmlVisualNode(root)) {
        states = QmlVisualNode(root).states().allStates();
    }

    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TRANSITION_ADDED);

    if (states.isEmpty()) {
        Core::AsynchronousMessageBox::warning(tr("No States Defined"),
                                              tr("There are no states defined in this component."));
        return {};
    }

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

    ModelNode transition;

    if (!idPropertyList.isEmpty()) {
        executeInTransaction(
                    " TransitionEditorView::addNewTransition", [&transition, idPropertyList, root, this]() {

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
            transition.setAuxiliaryData(transitionDurationProperty, 2000);
            transition.validId();
            root.nodeListProperty("transitions").reparentHere(transition);

            for (auto it = idPropertyList.cbegin(); it != idPropertyList.cend(); ++it) {
                ModelNode parallelAnimation = createModelNode("QtQuick.ParallelAnimation");
                transition.defaultNodeAbstractProperty().reparentHere(parallelAnimation);
                for (const QString &property : it.value()) {
                    ModelNode sequentialAnimation
                            = createModelNode("QtQuick.SequentialAnimation");
                    parallelAnimation.defaultNodeAbstractProperty().reparentHere(
                                sequentialAnimation);

                    const NodeMetaInfo pauseMetaInfo = model()->metaInfo("QtQuick.PauseAnimation");

                    ModelNode pauseAnimation = createModelNode("QtQuick.PauseAnimation",
                                                               pauseMetaInfo.majorVersion(),
                                                               pauseMetaInfo.minorVersion(),
                                                               {{"duration", 50}});
                    sequentialAnimation.defaultNodeAbstractProperty().reparentHere(
                                pauseAnimation);

                    const NodeMetaInfo propertyMetaInfo = model()->metaInfo("QtQuick.PauseAnimation");

                    ModelNode propertyAnimation = createModelNode("QtQuick.PropertyAnimation",
                                                                  propertyMetaInfo.majorVersion(),
                                                                  propertyMetaInfo.minorVersion(),
                                                                  {{"property", property},
                                                                   {"duration", 150}});
                    propertyAnimation.bindingProperty("target").setExpression(it.key());
                    sequentialAnimation.defaultNodeAbstractProperty().reparentHere(
                                propertyAnimation);
                }
            }
            });
    } else {
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

    if (m_transitionEditorWidget)
        m_transitionEditorWidget->init();

    return transition;
}

TransitionEditorWidget *TransitionEditorView::createWidget()
{
    if (!m_transitionEditorWidget)
        m_transitionEditorWidget = new TransitionEditorWidget(this);

    auto *transitionContext = new TransitionContext(m_transitionEditorWidget);
    Core::ICore::addContextObject(transitionContext);

    return m_transitionEditorWidget;
}

WidgetInfo TransitionEditorView::widgetInfo()
{
    return createWidgetInfo(createWidget(),
                            "TransitionEditor",
                            WidgetInfo::BottomPane,
                            0,
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
    return rootModelNode().nodeAbstractProperty("transitions").directSubNodes();
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
