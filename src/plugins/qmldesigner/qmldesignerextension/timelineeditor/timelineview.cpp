/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "timelineview.h"

#include "easingcurve.h"
#include "timelineactions.h"
#include "timelineconstants.h"
#include "timelinecontext.h"
#include "timelinewidget.h"

#include "timelinegraphicsscene.h"
#include "timelinesettingsdialog.h"
#include "timelinetoolbar.h"

#include <bindingproperty.h>
#include <exception.h>
#include <modelnodecontextmenu_helper.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>
#include <variantproperty.h>
#include <qmldesignericons.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>
#include <qmlobjectnode.h>
#include <qmlstate.h>
#include <qmltimeline.h>
#include <qmltimelinekeyframegroup.h>
#include <viewmanager.h>

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <designmodecontext.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QTimer>

namespace QmlDesigner {

TimelineView::TimelineView(QObject *parent)
    : AbstractView(parent)
{
    EasingCurve::registerStreamOperators();
}

TimelineView::~TimelineView() = default;

void TimelineView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    if (m_timelineWidget)
        m_timelineWidget->init();
}

void TimelineView::modelAboutToBeDetached(Model *model)
{
    m_timelineWidget->reset();
    setTimelineRecording(false);
    AbstractView::modelAboutToBeDetached(model);
}

void TimelineView::nodeCreated(const ModelNode & /*createdNode*/) {}

void TimelineView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (removedNode.isValid()) {
        if (QmlTimeline::isValidQmlTimeline(removedNode)) {
            auto *toolBar = widget()->toolBar();

            QString lastId = toolBar->currentTimelineId();
            toolBar->removeTimeline(QmlTimeline(removedNode));
            QString currentId = toolBar->currentTimelineId();

            removedNode.setAuxiliaryData("removed@Internal", true);

            if (currentId.isEmpty())
                m_timelineWidget->graphicsScene()->clearTimeline();
            if (lastId != currentId)
                m_timelineWidget->setTimelineId(currentId);
        } else if (removedNode.parentProperty().isValid()
                   && QmlTimeline::isValidQmlTimeline(removedNode.parentProperty().parentModelNode())) {
            if (removedNode.hasBindingProperty("target")) {
                const ModelNode target = removedNode.bindingProperty("target").resolveToModelNode();
                if (target.isValid()) {
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
    }
}

void TimelineView::nodeReparented(const ModelNode &node,
                                  const NodeAbstractProperty &newPropertyParent,
                                  const NodeAbstractProperty & /*oldPropertyParent*/,
                                  AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (newPropertyParent.isValid()
        && QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(
               newPropertyParent.parentModelNode())) {
        QmlTimelineKeyframeGroup frames(newPropertyParent.parentModelNode());
        m_timelineWidget->graphicsScene()->invalidateSectionForTarget(frames.target());
    } else if (QmlTimelineKeyframeGroup::checkKeyframesType(
                   node)) { /* During copy and paste type info might be incomplete */
        QmlTimelineKeyframeGroup frames(node);
        m_timelineWidget->graphicsScene()->invalidateSectionForTarget(frames.target());
    }
}

void TimelineView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName>> &propertyList)
{
    QmlTimeline timeline = currentTimeline();
    bool updated = false;
    for (const auto &pair : propertyList) {
        if (pair.second == "startFrame" || pair.second == "endFrame") {
            if (QmlTimeline::isValidQmlTimeline(pair.first)) {
                m_timelineWidget->invalidateTimelineDuration(pair.first);
            }
        } else if (pair.second == "currentFrame") {
            if (QmlTimeline::isValidQmlTimeline(pair.first)) {
                m_timelineWidget->invalidateTimelinePosition(pair.first);
            }
        } else if (!updated && timeline.hasTimeline(pair.first, pair.second)) {
            m_timelineWidget->graphicsScene()->invalidateCurrentValues();
            updated = true;
        }
    }
}

void TimelineView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                            AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    for (const auto &property : propertyList) {
        if (property.name() == "frame"
            && property.parentModelNode().type() == "QtQuick.Timeline.Keyframe"
            && property.parentModelNode().isValid()
            && property.parentModelNode().hasParentProperty()) {
            const ModelNode framesNode
                = property.parentModelNode().parentProperty().parentModelNode();
            if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(framesNode)) {
                QmlTimelineKeyframeGroup frames(framesNode);
                m_timelineWidget->graphicsScene()->invalidateKeyframesForTarget(frames.target());
            }
        }
    }
}

void TimelineView::selectedNodesChanged(const QList<ModelNode> & /*selectedNodeList*/,
                                        const QList<ModelNode> & /*lastSelectedNodeList*/)
{
    if (m_timelineWidget)
        m_timelineWidget->graphicsScene()->update();
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
            } else if (QmlTimeline::isValidQmlTimeline(property.parentModelNode())) {
                m_timelineWidget->graphicsScene()->invalidateScene();
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

const QmlTimeline TimelineView::addNewTimeline()
{
    const TypeName timelineType = "QtQuick.Timeline.Timeline";

    QTC_ASSERT(isAttached(), return QmlTimeline());

    try {
        ensureQtQuickTimelineImport();
    } catch (const Exception &e) {
        e.showException();
    }

    NodeMetaInfo metaInfo = model()->metaInfo(timelineType);

    QTC_ASSERT(metaInfo.isValid(), return QmlTimeline());

    ModelNode timelineNode;

    try {
        RewriterTransaction transaction(beginRewriterTransaction("TimelineView::addNewTimeline"));

        bool hasTimelines = getTimelines().isEmpty();

        timelineNode = createModelNode(timelineType,
                                       metaInfo.majorVersion(),
                                       metaInfo.minorVersion());
        timelineNode.validId();

        timelineNode.variantProperty("startFrame").setValue(0);
        timelineNode.variantProperty("endFrame").setValue(1000);
        timelineNode.variantProperty("enabled").setValue(hasTimelines);

        rootModelNode().defaultNodeListProperty().reparentHere(timelineNode);
        transaction.commit();
    } catch (const Exception &e) {
        e.showException();
    }

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
    try {
        RewriterTransaction transaction(
            beginRewriterTransaction("TimelineSettingsDialog::addAnimation"));

        animationNode = createModelNode(animationType,
                                        metaInfo.majorVersion(),
                                        metaInfo.minorVersion());
        animationNode.variantProperty("duration").setValue(timeline.duration());
        animationNode.validId();

        animationNode.variantProperty("from").setValue(timeline.startKeyframe());
        animationNode.variantProperty("to").setValue(timeline.endKeyframe());

        animationNode.variantProperty("loops").setValue(1);

        animationNode.variantProperty("running").setValue(getAnimations(timeline).isEmpty());

        timeline.modelNode().nodeListProperty("animations").reparentHere(animationNode);

        if (timeline.modelNode().hasProperty("currentFrame"))
            timeline.modelNode().removeProperty("currentFrame");
        transaction.commit();
    } catch (const Exception &e) {
        e.showException();
    }

    return animationNode;
}

void TimelineView::addNewTimelineDialog()
{
    auto timeline = addNewTimeline();
    addAnimation(timeline);
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
    ModelNode node = widget()->graphicsScene()->currentTimeline();

    QTC_ASSERT(node.isValid(), return );

    if (value) {
        activateTimelineRecording(node);
    } else {
        deactivateTimelineRecording();
        activateTimeline(node);
    }
}

void TimelineView::customNotification(const AbstractView * /*view*/,
                                      const QString &identifier,
                                      const QList<ModelNode> &nodeList,
                                      const QList<QVariant> &data)
{
    if (identifier == QStringLiteral("reset QmlPuppet")) {
        QmlTimeline timeline = widget()->graphicsScene()->currentTimeline();
        if (timeline.isValid())
            timeline.modelNode().removeAuxiliaryData("currentFrame@NodeInstance");
    } else if (identifier == "INSERT_KEYFRAME" && !nodeList.isEmpty() && !data.isEmpty()) {
        insertKeyframe(nodeList.constFirst(), data.constFirst().toString().toUtf8());
    }
}

void TimelineView::insertKeyframe(const ModelNode &target, const PropertyName &propertyName)
{
    QmlTimeline timeline = widget()->graphicsScene()->currentTimeline();
    ModelNode targetNode = target;
    if (timeline.isValid() && targetNode.isValid()
        && QmlObjectNode::isValidQmlObjectNode(targetNode)) {
        try {
            RewriterTransaction transaction(
                beginRewriterTransaction("TimelineView::insertKeyframe"));

            targetNode.validId();

            QmlTimelineKeyframeGroup timelineFrames(
                timeline.keyframeGroup(targetNode, propertyName));

            QTC_ASSERT(timelineFrames.isValid(), return );

            const qreal frame
                = timeline.modelNode().auxiliaryData("currentFrame@NodeInstance").toReal();
            const QVariant value = QmlObjectNode(targetNode).instanceValue(propertyName);

            timelineFrames.setValue(value, frame);

            transaction.commit();
        } catch (const Exception &e) {
            e.showException();
        }
    }
}

QList<QmlTimeline> TimelineView::getTimelines() const
{
    QList<QmlTimeline> timelines;

    if (!isAttached())
        return timelines;

    for (const ModelNode &modelNode : allModelNodes()) {
        if (QmlTimeline::isValidQmlTimeline(modelNode) && !modelNode.hasAuxiliaryData("removed@Internal")) {
            timelines.append(modelNode);
        }
    }
    return timelines;
}

QList<ModelNode> TimelineView::getAnimations(const QmlTimeline &timeline)
{
    if (!timeline.isValid())
        return QList<ModelNode>();

    if (isAttached()) {
        return Utils::filtered(timeline.modelNode().directSubModelNodes(),
                               [timeline](const ModelNode &node) {
                                   if (node.metaInfo().isValid() && node.hasParentProperty()
                                       && (node.parentProperty().parentModelNode()
                                           == timeline.modelNode()))
                                       return node.metaInfo().isSubclassOf(
                                           "QtQuick.Timeline.TimelineAnimation");
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

    for (const QmlModelState &state : QmlItemNode(rootModelNode()).states().allStates()) {
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

    SelectionContextPredicate timelineHasKeyframes =
            [this](const SelectionContext &context) {
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
                                                    TimelineConstants::priorityTimelineCategory,
                                                    timelineEnabled,
                                                    &SelectionContextFunctors::always));

    actionManager.addDesignerAction(
                new ModelNodeContextMenuAction("commandId timeline delete",
                                               TimelineConstants::timelineDeleteKeyframesDisplayName,
    {},
                                               TimelineConstants::timelineCategory,
                                               QKeySequence(),
                                               160,
                                               deleteKeyframes,
                                               timelineHasKeyframes));

    actionManager.addDesignerAction(
                new ModelNodeContextMenuAction("commandId timeline insert",
                                               TimelineConstants::timelineInsertKeyframesDisplayName,
    {},
                                               TimelineConstants::timelineCategory,
                                               QKeySequence(),
                                               140,
                                               insertKeyframes,
                                               timelineHasKeyframes));

    actionManager.addDesignerAction(
                new ModelNodeContextMenuAction("commandId timeline copy",
                                               TimelineConstants::timelineCopyKeyframesDisplayName,
    {},
                                               TimelineConstants::timelineCategory,
                                               QKeySequence(),
                                               120,
                                               copyKeyframes,
                                               timelineHasKeyframes));

    actionManager.addDesignerAction(
                new ModelNodeContextMenuAction("commandId timeline paste",
                                               TimelineConstants::timelinePasteKeyframesDisplayName,
    {},
                                               TimelineConstants::timelineCategory,
                                               QKeySequence(),
                                               100,
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
                            nullptr,
                            QStringLiteral("Timelines"),
                            WidgetInfo::BottomPane,
                            0,
                            tr("Timeline"));
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

} // namespace QmlDesigner
