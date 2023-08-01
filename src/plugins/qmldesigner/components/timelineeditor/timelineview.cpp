// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineview.h"

#include "designericons.h"
#include "easingcurve.h"
#include "timelineactions.h"
#include "timelineconstants.h"
#include "timelinecontext.h"
#include "timelinewidget.h"

#include "timelinegraphicsscene.h"
#include "timelinesettingsdialog.h"
#include "timelinetoolbar.h"

#include <auxiliarydataproperties.h>
#include <bindingproperty.h>
#include <designeractionmanager.h>
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

#include <utils/qtcassert.h>

#include <designmodecontext.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QTimer>

namespace QmlDesigner {

TimelineView::TimelineView(ExternalDependenciesInterface &externalDepoendencies)
    : AbstractView{externalDepoendencies}
    , m_timelineWidget(nullptr)
{
    EasingCurve::registerStreamOperators();
    setEnabled(false);
}

TimelineView::~TimelineView() = default;

void TimelineView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    if (!isEnabled())
        return;

    if (m_timelineWidget)
        m_timelineWidget->init();
}

void TimelineView::modelAboutToBeDetached(Model *model)
{
    if (!m_timelineWidget)
        m_timelineWidget->reset();
    const bool empty = getTimelines().isEmpty();
    if (!empty)
        setTimelineRecording(false);
    AbstractView::modelAboutToBeDetached(model);
}

void TimelineView::nodeCreated(const ModelNode & /*createdNode*/) {}

namespace {
constexpr AuxiliaryDataKeyView removedProperty{AuxiliaryDataType::Temporary, "removed"};
}

void TimelineView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (removedNode.isValid()) {
        if (QmlTimeline::isValidQmlTimeline(removedNode)) {
            auto *toolBar = widget()->toolBar();

            QString lastId = toolBar->currentTimelineId();
            toolBar->removeTimeline(QmlTimeline(removedNode));
            QString currentId = toolBar->currentTimelineId();

            removedNode.setAuxiliaryData(removedProperty, true);

            if (currentId.isEmpty())
                m_timelineWidget->graphicsScene()->clearTimeline();
            if (lastId != currentId)
                m_timelineWidget->setTimelineId(currentId);

        } else if (QmlTimeline::isValidQmlTimeline(removedNode.parentProperty().parentModelNode())) {
            if (const ModelNode target = removedNode.bindingProperty("target").resolveToModelNode()) {
                QmlTimeline timeline(removedNode.parentProperty().parentModelNode());
                if (timeline.hasKeyframeGroupForTarget(target))
                    QTimer::singleShot(0, [this, target, timeline]() {
                        if (timeline.hasKeyframeGroupForTarget(target))
                            m_timelineWidget->graphicsScene()->invalidateSectionForTarget(target);
                        else
                            m_timelineWidget->graphicsScene()->invalidateScene();
                    });
            }
        }
    }
}

void TimelineView::nodeRemoved(const ModelNode & /*removedNode*/,
                               const NodeAbstractProperty &parentProperty,
                               PropertyChangeFlags /*propertyChange*/)
{
    if (parentProperty.isValid()
        && QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(
               parentProperty.parentModelNode())) {
        QmlTimelineKeyframeGroup frames(parentProperty.parentModelNode());
        m_timelineWidget->graphicsScene()->invalidateSectionForTarget(frames.target());
        updateAnimationCurveEditor();
    } else if (parentProperty.isValid()
               && QmlTimeline::isValidQmlTimeline(parentProperty.parentModelNode())) {
        updateAnimationCurveEditor();
    }
}

void TimelineView::nodeReparented(const ModelNode &node,
                                  const NodeAbstractProperty &newPropertyParent,
                                  const NodeAbstractProperty & /*oldPropertyParent*/,
                                  AbstractView::PropertyChangeFlags propertyChange)
{
    if (newPropertyParent.isValid()
        && QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(
               newPropertyParent.parentModelNode())) {
        QmlTimelineKeyframeGroup frames(newPropertyParent.parentModelNode());
        m_timelineWidget->graphicsScene()->invalidateSectionForTarget(frames.target());

        if (propertyChange == AbstractView::NoAdditionalChanges)
            updateAnimationCurveEditor();

    } else if (QmlTimelineKeyframeGroup::checkKeyframesType(
                   node)) { /* During copy and paste type info might be incomplete */
        QmlTimelineKeyframeGroup frames(node);
        m_timelineWidget->graphicsScene()->invalidateSectionForTarget(frames.target());
        updateAnimationCurveEditor();
    }
}

void TimelineView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName>> &propertyList)
{
    QmlTimeline timeline = currentTimeline();
    bool updated = false;
    bool keyframeChangeFlag = false;
    for (const auto &pair : propertyList) {
        if (pair.second == "startFrame" || pair.second == "endFrame") {
            if (QmlTimeline::isValidQmlTimeline(pair.first)) {
                m_timelineWidget->invalidateTimelineDuration(pair.first);
            }
        } else if (pair.second == "currentFrame") {
            if (QmlTimeline::isValidQmlTimeline(pair.first)) {
                m_timelineWidget->invalidateTimelinePosition(pair.first);
                updateAnimationCurveEditor();
            }
        } else if (!updated && timeline.hasTimeline(pair.first, pair.second)) {
            m_timelineWidget->graphicsScene()->invalidateCurrentValues();
            updated = true;
        }

        if (!keyframeChangeFlag && pair.second == "frame") {
            m_timelineWidget->graphicsScene()->updateKeyframePositionsCache();
            keyframeChangeFlag = true;
        }
    }
}

void TimelineView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                            AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    for (const auto &property : propertyList) {
        if ((property.name() == "frame" || property.name() == "value")
            && property.parentModelNode().type() == "QtQuick.Timeline.Keyframe"
            && property.parentModelNode().hasParentProperty()) {
            const ModelNode framesNode
                = property.parentModelNode().parentProperty().parentModelNode();
            if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(framesNode)) {
                QmlTimelineKeyframeGroup frames(framesNode);
                m_timelineWidget->graphicsScene()->invalidateKeyframesForTarget(frames.target());
                updateAnimationCurveEditor();
            }
        }
    }
}

void TimelineView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                            [[maybe_unused]] AbstractView::PropertyChangeFlags propertyChange)
{
    for (const auto &property : propertyList) {
        if (property.name() == "easing.bezierCurve") {
            updateAnimationCurveEditor();
        }
    }
}

void TimelineView::selectedNodesChanged(const QList<ModelNode> & /*selectedNodeList*/,
                                        const QList<ModelNode> & /*lastSelectedNodeList*/)
{
    if (m_timelineWidget)
        m_timelineWidget->graphicsScene()->update();
}

void TimelineView::auxiliaryDataChanged(const ModelNode &modelNode,
                                        AuxiliaryDataKeyView key,
                                        const QVariant &data)
{
    if (key == lockedProperty && data.toBool() && modelNode.isValid()) {
        for (const auto &node : modelNode.allSubModelNodesAndThisNode()) {
            if (node.hasAuxiliaryData(timelineExpandedProperty))
                m_timelineWidget->graphicsScene()->invalidateHeightForTarget(node);
        }
    }
}

void TimelineView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const auto &property : propertyList) {
        if (property.isNodeListProperty()) {
            for (const auto &node : property.toNodeListProperty().toModelNodeList()) {
                nodeAboutToBeRemoved(node);
            }
        }
    }
}

void TimelineView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const auto &property : propertyList) {
        if (property.name() == "keyframes" && property.parentModelNode().isValid()) {
            if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(
                    property.parentModelNode())) {
                QmlTimelineKeyframeGroup frames(property.parentModelNode());
                m_timelineWidget->graphicsScene()->invalidateSectionForTarget(frames.target());
                updateAnimationCurveEditor();
            } else if (QmlTimeline::isValidQmlTimeline(property.parentModelNode())) {
                m_timelineWidget->graphicsScene()->invalidateScene();
                updateAnimationCurveEditor();
            }
        }
    }
}

bool TimelineView::hasWidget() const
{
    return true;
}

void TimelineView::nodeIdChanged(const ModelNode &node, const QString &, const QString &)
{
    if (QmlTimeline::isValidQmlTimeline(node))
        m_timelineWidget->init();
}

void TimelineView::currentStateChanged(const ModelNode &)
{
    if (m_timelineWidget)
        m_timelineWidget->init();
}

TimelineWidget *TimelineView::widget() const
{
    return m_timelineWidget;
}

QList<QmlModelState> getAllStates(TimelineView* view)
{
    QmlVisualNode visNode(view->rootModelNode());
    if (visNode.isValid()) {
        return visNode.states().allStates();
    }
    return {};
}

QString getStateName(TimelineView* view, bool& enableInBaseState)
{
    QString currentStateName;
    if (QmlModelState state = view->currentState(); state.isValid()) {
        if (!state.isBaseState()) {
            enableInBaseState = false;
            return state.name();
        }
    }
    return QString();
}

void enableInCurrentState(
    TimelineView* view, const QString& stateName,
    const ModelNode& node, const PropertyName& propertyName)
{
    if (!stateName.isEmpty()) {
        for (auto& state : getAllStates(view)) {
            if (state.isValid()) {
                QmlPropertyChanges propertyChanges(state.propertyChanges(node));
                if (state.name() == stateName)
                    propertyChanges.modelNode().variantProperty(propertyName).setValue(true);
                else
                    propertyChanges.modelNode().variantProperty(propertyName).setValue(false);
            }
        }
    }
}

const QmlTimeline TimelineView::addNewTimeline()
{
    const TypeName timelineType = "QtQuick.Timeline.Timeline";

    QTC_ASSERT(isAttached(), return QmlTimeline());

    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_TIMELINE_ADDED);

    try {
        ensureQtQuickTimelineImport();
    } catch (const Exception &e) {
        e.showException();
    }

    NodeMetaInfo metaInfo = model()->metaInfo(timelineType);

    QTC_ASSERT(metaInfo.isValid(), return QmlTimeline());

    ModelNode timelineNode;

    executeInTransaction("TimelineView::addNewTimeline", [=, &timelineNode]() {
        bool hasTimelines = getTimelines().isEmpty();
        QString currentStateName = getStateName(this, hasTimelines);

        timelineNode = createModelNode(timelineType,
                                       metaInfo.majorVersion(),
                                       metaInfo.minorVersion());
        timelineNode.validId();

        timelineNode.variantProperty("startFrame").setValue(0);
        timelineNode.variantProperty("endFrame").setValue(1000);
        timelineNode.variantProperty("enabled").setValue(hasTimelines);

        rootModelNode().defaultNodeListProperty().reparentHere(timelineNode);

        enableInCurrentState(this, currentStateName, timelineNode, "enabled");
    });

    return QmlTimeline(timelineNode);
}

ModelNode TimelineView::addAnimation(QmlTimeline timeline)
{
    const TypeName animationType = "QtQuick.Timeline.TimelineAnimation";

    QTC_ASSERT(timeline.isValid(), return ModelNode());

    QTC_ASSERT(isAttached(), return ModelNode());

    NodeMetaInfo metaInfo = model()->metaInfo(animationType);

    QTC_ASSERT(metaInfo.isValid(), return ModelNode());

    ModelNode animationNode;

    executeInTransaction("TimelineView::addAnimation", [=, &animationNode]() {
        bool hasAnimations = getAnimations(timeline).isEmpty();
        QString currentStateName = getStateName(this, hasAnimations);

        animationNode = createModelNode(animationType,
                                        metaInfo.majorVersion(),
                                        metaInfo.minorVersion());
        animationNode.variantProperty("duration").setValue(timeline.duration());
        animationNode.validId();

        animationNode.variantProperty("from").setValue(timeline.startKeyframe());
        animationNode.variantProperty("to").setValue(timeline.endKeyframe());

        animationNode.variantProperty("loops").setValue(1);

        animationNode.variantProperty("running").setValue(hasAnimations);

        timeline.modelNode().nodeListProperty("animations").reparentHere(animationNode);

        if (timeline.modelNode().hasProperty("currentFrame"))
            timeline.modelNode().removeProperty("currentFrame");

        enableInCurrentState(this, currentStateName, animationNode, "running");
    });

    return animationNode;
}

void TimelineView::addNewTimelineDialog()
{
    auto timeline = addNewTimeline();
    addAnimation(timeline);
    setCurrentTimeline(timeline);
    openSettingsDialog();
}

void TimelineView::openSettingsDialog()
{
    auto dialog = new TimelineSettingsDialog(Core::ICore::dialogParent(), this);

    auto timeline = m_timelineWidget->graphicsScene()->currentTimeline();
    if (timeline.isValid())
        dialog->setCurrentTimeline(timeline);

    QObject::connect(dialog, &TimelineSettingsDialog::rejected, [this, dialog]() {
        m_timelineWidget->init();
        dialog->deleteLater();
    });

    QObject::connect(dialog, &TimelineSettingsDialog::accepted, [this, dialog]() {
        m_timelineWidget->init();
        dialog->deleteLater();
    });

    dialog->show();
}

void TimelineView::setTimelineRecording(bool value)
{
    const ModelNode node = timelineForState(currentState()).modelNode();

    if (value && node.isValid()) {
        activateTimelineRecording(node);
    } else {
        deactivateTimelineRecording();
        setCurrentTimeline(node);
    }
}

void TimelineView::customNotification(const AbstractView * /*view*/,
                                      const QString &identifier,
                                      [[maybe_unused]] const QList<ModelNode> &nodeList,
                                      [[maybe_unused]] const QList<QVariant> &data)
{
    if (identifier == QStringLiteral("reset QmlPuppet")) {
        QmlTimeline timeline = widget()->graphicsScene()->currentTimeline();
        if (timeline.isValid())
            timeline.modelNode().removeAuxiliaryData(currentFrameProperty);
    }
}

void TimelineView::insertKeyframe(const ModelNode &target, const PropertyName &propertyName)
{
    QmlTimeline timeline = currentTimeline();

    if (timeline.isValid() && target.isValid() && QmlObjectNode::isValidQmlObjectNode(target)) {
        executeInTransaction("TimelineView::insertKeyframe", [=, &timeline, &target]() {
            timeline.insertKeyframe(target, propertyName);
        });
    }
}

QList<QmlTimeline> TimelineView::getTimelines() const
{
    QList<QmlTimeline> timelines;

    if (!isAttached())
        return timelines;

    for (const ModelNode &modelNode : allModelNodes()) {
        if (QmlTimeline::isValidQmlTimeline(modelNode)
            && !modelNode.hasAuxiliaryData(removedProperty)) {
            timelines.append(modelNode);
        }
    }
    return timelines;
}

QList<ModelNode> TimelineView::getAnimations(const QmlTimeline &timeline)
{
    if (!timeline.isValid())
        return {};

    if (isAttached()) {
        return Utils::filtered(timeline.modelNode().directSubModelNodes(),
                               [timeline](const ModelNode &node) {
                                   if (node.metaInfo().isValid() && node.hasParentProperty()
                                       && (node.parentProperty().parentModelNode()
                                           == timeline.modelNode()))
                                       return node.metaInfo().isQtQuickTimelineTimelineAnimation();
                                   return false;
                               });
    }
    return {};
}

QmlTimeline TimelineView::timelineForState(const ModelNode &state) const
{
    QmlModelState modelState(state);

    const QList<QmlTimeline> &timelines = getTimelines();

    if (modelState.isBaseState()) {
        for (const auto &timeline : timelines) {
            if (timeline.modelNode().hasVariantProperty("enabled")
                && timeline.modelNode().variantProperty("enabled").value().toBool())
                return timeline;
        }
        return QmlTimeline();
    }

    for (const auto &timeline : timelines) {
        if (modelState.affectsModelNode(timeline)) {
            QmlPropertyChanges propertyChanges(modelState.propertyChanges(timeline));

            if (propertyChanges.isValid() && propertyChanges.modelNode().hasProperty("enabled")
                && propertyChanges.modelNode().variantProperty("enabled").value().toBool())
                return timeline;
        }
    }
    return QmlTimeline();
}

QmlModelState TimelineView::stateForTimeline(const QmlTimeline &timeline)
{
    if (timeline.modelNode().hasVariantProperty("enabled")
        && timeline.modelNode().variantProperty("enabled").value().toBool()) {
        return QmlModelState(rootModelNode());
    }

    for (const QmlModelState &state : QmlVisualNode(rootModelNode()).states().allStates()) {
        if (timelineForState(state) == timeline)
            return state;
    }

    return QmlModelState();
}

void TimelineView::registerActions()
{
    auto &actionManager = QmlDesignerPlugin::instance()->viewManager().designerActionManager();

    SelectionContextPredicate timelineEnabled = [this](const SelectionContext &context) {
        return context.singleNodeIsSelected()
               && widget()->graphicsScene()->currentTimeline().isValid();
    };

    SelectionContextPredicate timelineHasKeyframes = [this](const SelectionContext &context) {
        auto timeline = widget()->graphicsScene()->currentTimeline();
        return !timeline.keyframeGroupsForTarget(context.currentSingleSelectedNode()).isEmpty();
    };

    SelectionContextPredicate timelineHasClipboard = [](const SelectionContext &context) {
        return !context.fastUpdate() && TimelineActions::clipboardContainsKeyframes();
    };

    SelectionContextOperation deleteKeyframes = [this](const SelectionContext &context) {
        auto mutator = widget()->graphicsScene()->currentTimeline();
        if (mutator.isValid())
            TimelineActions::deleteAllKeyframesForTarget(context.currentSingleSelectedNode(),
                                                         mutator);
    };

    SelectionContextOperation insertKeyframes = [this](const SelectionContext &context) {
        auto mutator = widget()->graphicsScene()->currentTimeline();
        if (mutator.isValid())
            TimelineActions::insertAllKeyframesForTarget(context.currentSingleSelectedNode(),
                                                         mutator);
    };

    SelectionContextOperation copyKeyframes = [this](const SelectionContext &context) {
        auto mutator = widget()->graphicsScene()->currentTimeline();
        if (mutator.isValid())
            TimelineActions::copyAllKeyframesForTarget(context.currentSingleSelectedNode(), mutator);
    };

    SelectionContextOperation pasteKeyframes = [this](const SelectionContext &context) {
        auto mutator = widget()->graphicsScene()->currentTimeline();
        if (mutator.isValid())
            TimelineActions::pasteKeyframesToTarget(context.currentSingleSelectedNode(), mutator);
    };

    actionManager.addDesignerAction(new ActionGroup(TimelineConstants::timelineCategoryDisplayName,
                                                    TimelineConstants::timelineCategory,
                                                    actionManager.contextIcon(DesignerIcons::TimelineIcon),
                                                    ComponentCoreConstants::Priorities::TimelineCategory,
                                                    timelineEnabled,
                                                    &SelectionContextFunctors::always));

    actionManager.addDesignerAction(
        new ModelNodeContextMenuAction("commandId timeline delete",
                                       TimelineConstants::timelineDeleteKeyframesDisplayName,
                                       {},
                                       TimelineConstants::timelineCategory,
                                       QKeySequence(),
                                       3,
                                       deleteKeyframes,
                                       timelineHasKeyframes));

    actionManager.addDesignerAction(
        new ModelNodeContextMenuAction("commandId timeline insert",
                                       TimelineConstants::timelineInsertKeyframesDisplayName,
                                       {},
                                       TimelineConstants::timelineCategory,
                                       QKeySequence(),
                                       1,
                                       insertKeyframes,
                                       timelineHasKeyframes));

    actionManager.addDesignerAction(
        new ModelNodeContextMenuAction("commandId timeline copy",
                                       TimelineConstants::timelineCopyKeyframesDisplayName,
                                       {},
                                       TimelineConstants::timelineCategory,
                                       QKeySequence(),
                                       4,
                                       copyKeyframes,
                                       timelineHasKeyframes));

    actionManager.addDesignerAction(
        new ModelNodeContextMenuAction("commandId timeline paste",
                                       TimelineConstants::timelinePasteKeyframesDisplayName,
                                       {},
                                       TimelineConstants::timelineCategory,
                                       QKeySequence(),
                                       5,
                                       pasteKeyframes,
                                       timelineHasClipboard));
}

TimelineWidget *TimelineView::createWidget()
{
    if (!m_timelineWidget)
        m_timelineWidget = new TimelineWidget(this);

    auto *timelineContext = new TimelineContext(m_timelineWidget);
    Core::ICore::addContextObject(timelineContext);

    return m_timelineWidget;
}

WidgetInfo TimelineView::widgetInfo()
{
    return createWidgetInfo(createWidget(),
                            QStringLiteral("Timelines"),
                            WidgetInfo::BottomPane,
                            0,
                            tr("Timeline"),
                            tr("Timeline view"));
}

bool TimelineView::hasQtQuickTimelineImport()
{
    if (isAttached()) {
        Import import = Import::createLibraryImport("QtQuick.Timeline", "1.0");
        return model()->hasImport(import, true, true);
    }

    return false;
}

void TimelineView::ensureQtQuickTimelineImport()
{
    if (!hasQtQuickTimelineImport()) {
        Import timelineImport = Import::createLibraryImport("QtQuick.Timeline", "1.0");
        model()->changeImports({timelineImport}, {});
    }
}

void TimelineView::updateAnimationCurveEditor()
{
    if (!m_timelineWidget)
        return;

    QmlTimeline currentTimeline = timelineForState(currentState());
    if (currentTimeline.isValid())
        m_timelineWidget->toolBar()->setCurrentTimeline(currentTimeline);
    else
        m_timelineWidget->toolBar()->reset();
}

} // namespace QmlDesigner
