/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "transitioneditorview.h"

#include "transitioneditortoolbar.h"
#include "transitioneditorwidget.h"

#include "transitioneditorgraphicsscene.h"
#include "transitioneditorsettingsdialog.h"

#include <bindingproperty.h>
#include <exception.h>
#include <modelnodecontextmenu_helper.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>
#include <variantproperty.h>
#include <viewmanager.h>
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

TransitionEditorView::TransitionEditorView(QObject *parent)
    : AbstractView(parent)
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
}

void TransitionEditorView::nodeReparented(const ModelNode &node,
                                          const NodeAbstractProperty &newPropertyParent,
                                          const NodeAbstractProperty & /*oldPropertyParent*/,
                                          AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (newPropertyParent.name() == "transitions")
        asyncUpdate(node);

    const ModelNode parent = newPropertyParent.parentModelNode();

    qDebug() << Q_FUNC_INFO << parent;
    if (parent.isValid() && parent.metaInfo().isValid()
        && parent.metaInfo().isSubclassOf("QtQuick.Transition")) {
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
    if (node.metaInfo().isValid() && node.metaInfo().isSubclassOf("QtQuick.Transition"))
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

    if (states.isEmpty()) {
        Core::AsynchronousMessageBox::warning(tr("No States Defined"),
                                              tr("There are no states defined in this component."));
        return {};
    }

    QHash<QString, QStringList> idPropertyList;

    const QVector<TypeName> validProperties = {"int", "real", "double", "qreal", "color", "QColor"};

    for (const QmlModelState &state : qAsConst(states)) {
        for (const QmlPropertyChanges & change : state.propertyChanges()) {
            QStringList locList;
            const ModelNode target = change.target();
            const QString targetId = target.id();
            if (target.isValid() && target.hasMetaInfo()) {
                for (const VariantProperty &property : change.modelNode().variantProperties()) {
                    TypeName typeName = target.metaInfo().propertyTypeName(property.name());

                    if (validProperties.contains(typeName))
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
                transition = createModelNode("QtQuick.Transition",
                                             2,
                                             0,
                                             {{
                                                  "from",
                                                  "*",
                                              },
                                              {
                                                  "to",
                                                  "*",
                                              }});
                transition.setAuxiliaryData("transitionDuration", 2000);
                transition.validId();
                root.nodeListProperty("transitions").reparentHere(transition);

                for (const QString &id : idPropertyList.keys()) {
                    ModelNode parallelAnimation = createModelNode("QtQuick.ParallelAnimation",
                                                                  2,
                                                                  12);
                    transition.defaultNodeAbstractProperty().reparentHere(parallelAnimation);
                    for (const QString &property : idPropertyList.value(id)) {
                        ModelNode sequentialAnimation
                            = createModelNode("QtQuick.SequentialAnimation", 2, 12);
                        parallelAnimation.defaultNodeAbstractProperty().reparentHere(
                            sequentialAnimation);

                        ModelNode pauseAnimation = createModelNode("QtQuick.PauseAnimation",
                                                                   2,
                                                                   12,
                                                                   {{"duration", 50}});
                        sequentialAnimation.defaultNodeAbstractProperty().reparentHere(
                            pauseAnimation);

                        ModelNode propertyAnimation = createModelNode("QtQuick.PropertyAnimation",
                                                                      2,
                                                                      12,
                                                                      {{"property", property},
                                                                       {"duration", 150}});
                        propertyAnimation.bindingProperty("target").setExpression(id);
                        sequentialAnimation.defaultNodeAbstractProperty().reparentHere(
                            propertyAnimation);
                    }
                }
            });
    }

    if (m_transitionEditorWidget)
        m_transitionEditorWidget->init();

    return transition;
}

TransitionEditorWidget *TransitionEditorView::createWidget()
{
    if (!m_transitionEditorWidget)
        m_transitionEditorWidget = new TransitionEditorWidget(this);

    //auto *timelineContext = new TimelineContext(m_timelineWidget);
    //Core::ICore::addContextObject(timelineContext);

    return m_transitionEditorWidget;
}

WidgetInfo TransitionEditorView::widgetInfo()
{
    return createWidgetInfo(createWidget(),
                            nullptr,
                            "TransitionEditor",
                            WidgetInfo::BottomPane,
                            0,
                            tr("Transition Editor"));
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

const QList<ModelNode> TransitionEditorView::allTransitions() const
{
    if (rootModelNode().isValid() && rootModelNode().hasProperty("transitions")) {
        NodeAbstractProperty transitions = rootModelNode().nodeAbstractProperty("transitions");
        if (transitions.isValid())
            return transitions.directSubNodes();
    }
    return {};
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
