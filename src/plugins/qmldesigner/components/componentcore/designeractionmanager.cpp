// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designeractionmanager.h"

#include "anchoraction.h"
#include "bundlehelper.h"
#include "changestyleaction.h"
#include "componentcoretracing.h"
#include "designeractionmanagerview.h"
#include "designericons.h"
#include "designermcumanager.h"
#include "designmodewidget.h"
#include "formatoperation.h"
#include "groupitemaction.h"
#include "modelnodecontextmenu_helper.h"
#include "propertytreemodel.h"
#include "qmldesignerconstants.h"
#include "qmleditormenu.h"
#include "rewritingexception.h"

#include <bindingproperty.h>
#include <customnotifications.h>
#include <nodehints.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <qmldesignertr.h>
#include <theme.h>
#include <variantproperty.h>

#include <formeditortoolbutton.h>

#include <actioneditor.h>
#include <documentmanager.h>
#include <modelutils.h>
#include <viewmanager.h>
#include <qmldesignerplugin.h>

#include <listmodeleditor/listmodeleditordialog.h>
#include <listmodeleditor/listmodeleditormodel.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <qmlprojectmanager/qmlproject.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QGraphicsLinearLayout>
#include <QHBoxLayout>
#include <QImageReader>
#include <QMessageBox>
#include <QMimeData>
#include <QScopeGuard>

#include <exception>

namespace QmlDesigner {

using ComponentCoreTracing::category;
using NanotraceHR::keyValue;

DesignerActionManagerView *DesignerActionManager::view()
{
    NanotraceHR::Tracer tracer{"designer action manager view", category()};

    return m_designerActionManagerView;
}

DesignerActionToolBar *DesignerActionManager::createToolBar(QWidget *parent) const
{
    NanotraceHR::Tracer tracer{"designer action manager create tool bar", category()};

    auto toolBar = new DesignerActionToolBar(parent);

    QList<ActionInterface* > categories = Utils::filtered(designerActions(), [](ActionInterface *action) {
            return action->type() ==  ActionInterface::ContextMenu;
    });

    Utils::sort(categories, [](ActionInterface *l, ActionInterface *r) {
        return l->priority() < r->priority();
    });

    for (auto *categoryAction : std::as_const(categories)) {
        QList<ActionInterface* > actions = Utils::filtered(designerActions(), [categoryAction](ActionInterface *action) {
                return action->category() == categoryAction->menuId();
        });

        Utils::sort(actions, [](ActionInterface *l, ActionInterface *r) {
            return l->priority() < r->priority();
        });

        bool addSeparator = false;

        for (auto *action : std::as_const(actions)) {
            if ((action->type() == ActionInterface::Action || action->type() == ActionInterface::ToolBarAction)
                    && action->action()) {
                toolBar->registerAction(action);
                addSeparator = true;
            } else if (addSeparator && action->action()->isSeparator()) {
                toolBar->registerAction(action);
            }
        }
    }

    return toolBar;
}

void DesignerActionManager::polishActions() const
{
    NanotraceHR::Tracer tracer{"designer action manager polish actions", category()};

    QList<ActionInterface *> actions = Utils::filtered(designerActions(), [](ActionInterface *action) {
        return action->type() != ActionInterface::ContextMenu;
    });

    Core::Context qmlDesignerFormEditorContext(Constants::qmlFormEditorContextId);
    Core::Context qmlDesignerEditor3DContext(Constants::qml3DEditorContextId);
    Core::Context qmlDesignerNavigatorContext(Constants::qmlNavigatorContextId);
    Core::Context qmlDesignerMaterialBrowserContext(Constants::qmlMaterialBrowserContextId);
    Core::Context qmlDesignerAssetsLibraryContext(Constants::qmlAssetsLibraryContextId);

    Core::Context qmlDesignerUIContext;
    qmlDesignerUIContext.add(qmlDesignerFormEditorContext);
    qmlDesignerUIContext.add(qmlDesignerEditor3DContext);
    qmlDesignerUIContext.add(qmlDesignerNavigatorContext);
    qmlDesignerUIContext.add(qmlDesignerMaterialBrowserContext);
    qmlDesignerUIContext.add(qmlDesignerAssetsLibraryContext);

    for (auto *action : actions) {
        if (!action->menuId().isEmpty()) {
            const Utils::Id id = Utils::Id("QmlDesigner.").withSuffix(action->menuId());
            Core::Command *cmd = Core::ActionManager::registerAction(action->action(), id, qmlDesignerUIContext);

            cmd->setDefaultKeySequence(action->action()->shortcut());
            cmd->setDescription(action->action()->toolTip());

            action->action()->setToolTip(cmd->action()->toolTip());
            action->action()->setShortcut(cmd->action()->shortcut());
            action->action()->setShortcutContext(Qt::WidgetShortcut); //Hack to avoid conflicting shortcuts. We use the Core::Command for the shortcut.
        }
    }
}

QGraphicsWidget *DesignerActionManager::createFormEditorToolBar(QGraphicsItem *parent)
{
    NanotraceHR::Tracer tracer{"designer action manager create form editor tool bar", category()};

    QList<ActionInterface *> actions = Utils::filtered(designerActions(), [](ActionInterface *action) {
        return action->type() == ActionInterface::FormEditorAction && action->action()->isVisible();
    });

    Utils::sort(actions, [](ActionInterface *l, ActionInterface *r) {
        return l->priority() < r->priority();
    });

    QGraphicsWidget *toolbar = new QGraphicsWidget(parent);

    auto layout = new QGraphicsLinearLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    toolbar->setLayout(layout);

    for (ActionInterface *action : std::as_const(actions)) {
        auto button = new FormEditorToolButton(action->action(), toolbar);
        layout->addItem(button);
    }

    toolbar->resize(toolbar->preferredSize());

    layout->invalidate();
    layout->activate();

    toolbar->update();

    return toolbar;
}

DesignerActionManager &DesignerActionManager::instance()
{
    NanotraceHR::Tracer tracer{"designer action manager instance", category()};

    return QmlDesignerPlugin::instance()->viewManager().designerActionManager();
}

void DesignerActionManager::setupContext()
{
    NanotraceHR::Tracer tracer{"designer action manager setup context", category()};

    m_designerActionManagerView->setupContext();
}

QList<AddResourceHandler> DesignerActionManager::addResourceHandler() const
{
    NanotraceHR::Tracer tracer{"designer action manager add resource handler", category()};

    return m_addResourceHandler;
}

void DesignerActionManager::registerAddResourceHandler(const AddResourceHandler &handler)
{
    NanotraceHR::Tracer tracer{"designer action manager register add resource handler", category()};

    m_addResourceHandler.append(handler);
}

void DesignerActionManager::unregisterAddResourceHandlers(const QString &category)
{
    NanotraceHR::Tracer tracer{"designer action manager unregister add resource handlers",
                               ComponentCoreTracing::category(),
                               keyValue("category", category)};

    for (int i = m_addResourceHandler.size() - 1; i >= 0; --i) {
        const AddResourceHandler &handler = m_addResourceHandler[i];
        if (handler.category == category)
            m_addResourceHandler.removeAt(i);
    }
}

void DesignerActionManager::registerModelNodePreviewHandler(const ModelNodePreviewImageHandler &handler)
{
    NanotraceHR::Tracer tracer{"designer action manager register model node preview handler",
                               category()};

    m_modelNodePreviewImageHandlers.append(handler);
}

bool DesignerActionManager::hasModelNodePreviewHandler(const ModelNode &node) const
{
    NanotraceHR::Tracer tracer{"designer action manager has model node preview handler", category()};

    const bool isComponent = node.isComponent();
    for (const auto &handler : std::as_const(m_modelNodePreviewImageHandlers)) {
        if ((isComponent || !handler.componentOnly)) {
            if (auto base = node.model()->metaInfo(handler.type); node.metaInfo().isBasedOn(base))
                return true;
        }
    }
    return false;
}

ModelNodePreviewImageOperation DesignerActionManager::modelNodePreviewOperation(const ModelNode &node) const
{
    NanotraceHR::Tracer tracer{"designer action manager model node preview operation", category()};

    ModelNodePreviewImageOperation op = nullptr;
    int prio = -1;
    const bool isComponent = node.isComponent();
    for (const auto &handler : std::as_const(m_modelNodePreviewImageHandlers)) {
        if ((isComponent || !handler.componentOnly) && handler.priority > prio) {
            if (auto base = node.model()->metaInfo(handler.type); node.metaInfo().isBasedOn(base)) {
                op = handler.operation;
                prio = handler.priority;
            }
        }
    }
    return op;
}

bool DesignerActionManager::externalDragHasSupportedAssets(const QMimeData *mimeData) const
{
    NanotraceHR::Tracer tracer{"designer action manager external drag has supported assets",
                               category()};

    if (!mimeData->hasUrls() || mimeData->hasFormat(Constants::MIME_TYPE_ASSETS))
        return false;

    QSet<QString> filtersSet;
    const QList<AddResourceHandler> handlers = addResourceHandler();
    for (const AddResourceHandler &handler : handlers)
        filtersSet.insert(handler.filter);

    const QList<QUrl> urls = mimeData->urls();
    for (const QUrl &url : urls) {
        QString suffix = "*." + url.fileName().split('.').last().toLower();
        if (filtersSet.contains(suffix)) // accept drop if it contains a valid file
            return true;
    }

    return false;
}

QHash<QString, QStringList> DesignerActionManager::handleExternalAssetsDrop(const QMimeData *mimeData) const
{
    NanotraceHR::Tracer tracer{"designer action manager handle external assets drop", category()};

    const QList<AddResourceHandler> handlers = addResourceHandler();
    // create suffix to categry and category to operation hashes
    QHash<QString, QString> suffixCategory;
    QHash<QString, AddResourceOperation> categoryOperation;
    for (const AddResourceHandler &handler : handlers) {
        suffixCategory.insert(handler.filter, handler.category);
        categoryOperation.insert(handler.category, handler.operation);
    }

    // add files grouped by categories (so that files under same category run under 1 operation)
    QHash<QString, QStringList> categoryFiles;
    const QList<QUrl> urls = mimeData->urls();
    for (const QUrl &url : urls) {
        QString suffix = "*." + url.fileName().split('.').last().toLower();
        QString category = suffixCategory.value(suffix);
        if (!category.isEmpty())
            categoryFiles[category].append(url.toLocalFile());
    }

    QHash<QString, QStringList> addedCategoryFiles;

    // run operations
    const QStringList categories = categoryFiles.keys();
    for (const QString &category : categories) {
        AddResourceOperation operation = categoryOperation.value(category);
        QStringList files = categoryFiles.value(category);
        AddFilesResult result = operation(files, {}, true);
        if (result.status() == AddFilesResult::Succeeded)
            addedCategoryFiles.insert(category, files);
    }

    return addedCategoryFiles;
}

QIcon DesignerActionManager::contextIcon(int contextId) const
{
    NanotraceHR::Tracer tracer{"designer action manager context icon", category()};

    return m_designerIcons->icon(DesignerIcons::IconId(contextId), DesignerIcons::ContextMenuArea);
}

QIcon DesignerActionManager::toolbarIcon(int contextId) const
{
    NanotraceHR::Tracer tracer{"designer action manager toolbar icon", category()};

    return m_designerIcons->icon(DesignerIcons::IconId(contextId), DesignerIcons::ToolbarArea);
}

void DesignerActionManager::addAddActionCallback(ActionAddedInterface callback)
{
    NanotraceHR::Tracer tracer{"designer action manager add add action callback", category()};

    m_callBacks.append(callback);
}

namespace {

class VisiblityModelNodeAction : public ModelNodeContextMenuAction
{
public:
    VisiblityModelNodeAction(const QByteArray &id, const QString &description, const QIcon &icon, const QByteArray &category, const QKeySequence &key, int priority,
                             SelectionContextOperation action,
                             SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                             SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        ModelNodeContextMenuAction(id, description, icon, category, key, priority, action, enabled, visibility)
    {
        NanotraceHR::Tracer tracer{"visibility model node action constructor",
                                   ComponentCoreTracing::category()};
    }

    void updateContext() override
    {
        NanotraceHR::Tracer tracer{"visibility model node action update context", category()};

        pureAction()->setSelectionContext(selectionContext());
        if (selectionContext().isValid()) {
            action()->setEnabled(isEnabled(selectionContext()));
            action()->setVisible(isVisible(selectionContext()));

            action()->setCheckable(true);
            QmlItemNode itemNode = QmlItemNode(selectionContext().currentSingleSelectedNode());
            if (itemNode.isValid())
                action()->setChecked(itemNode.instanceValue("visible").toBool());
            else
                action()->setEnabled(false);
        }
    }
};

class FillLayoutModelNodeAction : public ModelNodeContextMenuAction
{
public:
    FillLayoutModelNodeAction(const QByteArray &id, const QString &description, const QByteArray &category, const QKeySequence &key, int priority,
                              SelectionContextOperation action,
                              SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                              SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        ModelNodeContextMenuAction(id, description, {}, category, key, priority, action, enabled, visibility)
    {
        NanotraceHR::Tracer tracer{"fill layout model node action constructor",
                                   ComponentCoreTracing::category()};
    }

    void updateContext() override
    {
        NanotraceHR::Tracer tracer{"fill layout model node action update context", category()};

        pureAction()->setSelectionContext(selectionContext());
        if (selectionContext().isValid()) {
            action()->setEnabled(isEnabled(selectionContext()));
            action()->setVisible(isVisible(selectionContext()));

            action()->setCheckable(true);
            QmlItemNode itemNode = QmlItemNode(selectionContext().currentSingleSelectedNode());
            if (itemNode.isValid()) {
                bool flag = false;
                if (itemNode.modelNode().hasProperty(m_propertyName)
                        || itemNode.propertyAffectedByCurrentState(m_propertyName)) {
                    flag = itemNode.modelValue(m_propertyName).toBool();
                }
                action()->setChecked(flag);
            } else {
                action()->setEnabled(false);
            }
        }
    }
protected:
    PropertyName m_propertyName;
};

class FillWidthModelNodeAction : public FillLayoutModelNodeAction
{
public:
    FillWidthModelNodeAction(const QByteArray &id, const QString &description, const QByteArray &category, const QKeySequence &key, int priority,
                             SelectionContextOperation action,
                             SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                             SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        FillLayoutModelNodeAction(id, description, category, key, priority, action, enabled, visibility)
    {
        NanotraceHR::Tracer tracer{"fill width model node action constructor",
                                   ComponentCoreTracing::category()};

        m_propertyName = "Layout.fillWidth";
    }
};

class FillHeightModelNodeAction : public FillLayoutModelNodeAction
{
public:
    FillHeightModelNodeAction(const QByteArray &id, const QString &description, const QByteArray &category, const QKeySequence &key, int priority,
                              SelectionContextOperation action,
                              SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                              SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        FillLayoutModelNodeAction(id, description, category, key, priority, action, enabled, visibility)
    {
        NanotraceHR::Tracer tracer{"fill height model node action constructor",
                                   ComponentCoreTracing::category()};

        m_propertyName = "Layout.fillHeight";
    }
};

class SelectionModelNodeAction : public ActionGroup
{
public:
    SelectionModelNodeAction(const QString &displayName, const QByteArray &menuId, const QIcon &icon, int priority) :
        ActionGroup(displayName, menuId, icon, priority,
                    &SelectionContextFunctors::always, &SelectionContextFunctors::selectionEnabled)

    {
        NanotraceHR::Tracer tracer{"selection model node action constructor", category()};
    }

    void updateContext() override
    {
        NanotraceHR::Tracer tracer{"selection model node action update context", category()};

        menu()->clear();
        if (selectionContext().isValid()) {
            action()->setEnabled(isEnabled(selectionContext()));
            action()->setVisible(isVisible(selectionContext()));
        } else {
            return;
        }
        if (action()->isEnabled()) {
            ModelNode parentNode;
            if (selectionContext().singleNodeIsSelected()
                    && !selectionContext().currentSingleSelectedNode().isRootNode()
                    && selectionContext().currentSingleSelectedNode().hasParentProperty()) {

                parentNode = selectionContext().currentSingleSelectedNode().parentProperty().parentModelNode();

                if (!ModelUtils::isThisOrAncestorLocked(parentNode)) {
                    ActionTemplate *selectionAction = new ActionTemplate("SELECTION", {}, &ModelNodeOperations::select);
                    selectionAction->setParent(menu());
                    selectionAction->setText(QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Parent")));

                    SelectionContext nodeSelectionContext = selectionContext();
                    nodeSelectionContext.setTargetNode(parentNode);
                    selectionAction->setSelectionContext(nodeSelectionContext);

                    menu()->addAction(selectionAction);
                }
            }
            for (const ModelNode &node : selectionContext().view()->allModelNodes()) {
                if (node != selectionContext().currentSingleSelectedNode() && node != parentNode
                    && SelectionContextHelpers::contains(node, selectionContext().scenePosition())
                    && !node.isRootNode() && !ModelUtils::isThisOrAncestorLocked(node)) {
                    selectionContext().setTargetNode(node);
                    QString what = QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select: %1"))
                                       .arg(node.displayName());
                    ActionTemplate *selectionAction = new ActionTemplate("SELECT", what, &ModelNodeOperations::select);

                    SelectionContext nodeSelectionContext = selectionContext();
                    nodeSelectionContext.setTargetNode(node);
                    selectionAction->setSelectionContext(nodeSelectionContext);

                    menu()->addAction(selectionAction);
                }
            }

            if (menu()->isEmpty())
                action()->setEnabled(false);
        }
    }
};

QString prependSignal(QString signalHandlerName)
{
    NanotraceHR::Tracer tracer{"designer action prepend signal", category()};

    if (signalHandlerName.isNull() || signalHandlerName.isEmpty())
        return {};

    QChar firstChar = signalHandlerName.at(0).toUpper();
    signalHandlerName[0] = firstChar;
    signalHandlerName.prepend(QLatin1String("on"));

    return signalHandlerName;
}

QStringList getSignalsList(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"designer action get signals list", category()};

    if (!node.isValid())
        return {};

    if (!node.hasMetaInfo())
        return {};

    QStringList signalList;

    std::vector<PropertyName> signalVector = PropertyTreeModel::sortedAndFilteredSignalNames(
        node.metaInfo());

    std::vector<QString> signalVectorString = Utils::transform(signalVector,
                                                               [](const PropertyName &name) {
                                                                   return QString::fromUtf8(name);
                                                               });

    signalList.reserve(Utils::ssize(signalVectorString));
    std::copy(signalVectorString.begin(), signalVectorString.end(), std::back_inserter(signalList));

    return signalList;
}

struct SlotEntry
{
    QString name;
    std::function<void(SignalHandlerProperty)> action;
};

struct SlotList
{
    QString categoryName;
    QList<SlotEntry> slotEntries;
};

QList<ModelNode> stateGroups(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"designer action state groups", category()};

    if (!node.view()->isAttached())
        return {};

    const auto groupMetaInfo = node.view()->model()->qtQuickStateGroupMetaInfo();

    return node.view()->allModelNodesOfType(groupMetaInfo);
}

QList<SlotList> getSlotsLists(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"designer action get slots lists", category()};

    if (!node.isValid())
        return {};

    if (!node.view()->rootModelNode().isValid())
        return {};

    QList<SlotList> resultList;

    ModelNode rootNode = node.view()->rootModelNode();
    QmlObjectNode rootObjectNode(rootNode);

    const QString changeStateStr = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Change State");
    const QString changeStateGroupStr = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                          "Change State Group");
    const QString defaultStateStr = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Default State");
    auto createStateChangeSlot = [](ModelNode node,
                                    const QString &stateName,
                                    const QString &displayName) {
        return SlotEntry(
            {displayName, [node, stateName](SignalHandlerProperty signalHandler) mutable {
                 signalHandler.setSource(QString("%1.state = \"%2\"").arg(node.validId(), stateName));
             }});
    };

    {
        SlotList states = {changeStateStr, {}};

        const SlotEntry defaultState = createStateChangeSlot(rootNode, "", defaultStateStr);
        states.slotEntries.push_back(defaultState);

        for (const auto &stateName : rootObjectNode.states().names()) {
            const SlotEntry entry = createStateChangeSlot(rootNode, stateName, stateName);

            states.slotEntries.push_back(entry);
        }

        resultList.push_back(states);
    }

    const QList<ModelNode> groups = stateGroups(node);
    for (const auto &stateGroup : groups) {
        QmlObjectNode stateGroupObjectNode(stateGroup);
        SlotList stateGroupCategory = {changeStateGroupStr + " " + stateGroup.displayName(), {}};

        const SlotEntry defaultGroupState = createStateChangeSlot(stateGroup, "", defaultStateStr);
        stateGroupCategory.slotEntries.push_back(defaultGroupState);

        for (const auto &stateName : stateGroupObjectNode.states().names()) {
            const SlotEntry entry = createStateChangeSlot(stateGroup, stateName, stateName);
            stateGroupCategory.slotEntries.push_back(entry);
        }

        resultList.push_back(stateGroupCategory);
    }

    return resultList;
}

//creates connection without signalHandlerProperty
ModelNode createNewConnection(ModelNode targetNode)
{
    NanotraceHR::Tracer tracer{"designer action create new connection", category()};

    ModelNode newConnectionNode = targetNode.view()->createModelNode("Connections");

    if (QmlItemNode::isValidQmlItemNode(targetNode)) {
        targetNode.nodeAbstractProperty("data").reparentHere(newConnectionNode);
    } else {
        targetNode.view()->rootModelNode().defaultNodeAbstractProperty().reparentHere(
            newConnectionNode);
    }

    newConnectionNode.bindingProperty("target").setExpression(targetNode.id());

    return newConnectionNode;
}

void removeSignal(SignalHandlerProperty signalHandler)
{
    NanotraceHR::Tracer tracer{"designer action remove signal", category()};

    if (!signalHandler.isValid())
        return;
    auto connectionNode = signalHandler.parentModelNode();
    auto connectionSignals = connectionNode.signalProperties();
    if (connectionSignals.size() > 1) {
        if (connectionSignals.contains(signalHandler))
            connectionNode.removeProperty(signalHandler.name());
    } else {
        connectionNode.destroy();
    }
}

class ConnectionsModelNodeActionGroup : public ActionGroup
{
public:
    ConnectionsModelNodeActionGroup(const QString &displayName, const QByteArray &menuId, const QIcon &icon, int priority)
        : ActionGroup(displayName,
                      menuId,
                      icon,
                      priority,
                      &SelectionContextFunctors::always,
                      &SelectionContextFunctors::selectionEnabled)
    {
        NanotraceHR::Tracer tracer{"connections model node action group constructor", category()};
    }

    void updateContext() override
    {
        NanotraceHR::Tracer tracer{"connections model node action group update context", category()};

        menu()->clear();

        const auto selection = selectionContext();

        bool showMenu = false;
        const QScopeGuard cleanup([&]{ menu()->setEnabled(showMenu); });

        if (!selection.isValid())
            return;
        if (!selection.singleNodeIsSelected())
            return;

        ModelNode currentNode = selection.currentSingleSelectedNode();

        if (!currentNode.isValid())
            return;
        if (!currentNode.hasId())
            return;
        showMenu = true;

        QmlObjectNode currentObjectNode(currentNode);

        QStringList signalsList = getSignalsList(currentNode);
        QList<SlotList> slotsLists = getSlotsLists(currentNode);

        for (const ModelNode &connectionNode : currentObjectNode.getAllConnections()) {
            for (const AbstractProperty &property : connectionNode.properties()) {
                if (property.isSignalHandlerProperty() && property.name() != "target") {
                    const auto signalHandler = property.toSignalHandlerProperty();

                    const QString propertyName = QString::fromUtf8(signalHandler.name());

                    QMenu *activeSignalHandlerGroup = new QmlEditorMenu(propertyName, menu());

                    QMenu *editSignalGroup = new QmlEditorMenu(QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                                 "Change Signal"),
                                                               menu());

                    for (const auto &signalStr : signalsList) {
                        if (prependSignal(signalStr).toUtf8() == signalHandler.name())
                            continue;

                        ActionTemplate *newSignalAction = new ActionTemplate(
                            (signalStr + "Id").toLatin1(),
                            signalStr,
                            [signalStr, signalHandler](const SelectionContext &) {
                                signalHandler.parentModelNode().view()->executeInTransaction(
                                    "ConnectionsModelNodeActionGroup::"
                                    "changeSignal",
                                    [signalStr, signalHandler]() {
                                        auto connectionNode = signalHandler.parentModelNode();
                                        auto newHandler = connectionNode.signalHandlerProperty(
                                            prependSignal(signalStr).toLatin1());
                                        newHandler.setSource(signalHandler.source());
                                        connectionNode.removeProperty(signalHandler.name());
                                    });
                            });
                        editSignalGroup->addAction(newSignalAction);
                    }

                    activeSignalHandlerGroup->addMenu(editSignalGroup);

                    if (!slotsLists.isEmpty()) {
                        QMenu *editSlotGroup = new QmlEditorMenu(QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                                   "Change Slot"),
                                                                 menu());

                        if (slotsLists.size() == 1) {
                            for (const auto &slot : slotsLists.at(0).slotEntries) {
                                ActionTemplate *newSlotAction = new ActionTemplate(
                                    (slot.name + "Id").toLatin1(),
                                    (slotsLists.at(0).categoryName
                                     + (QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                          " to ")) //context: Change State _to_ state1
                                     + slot.name),
                                    [slot, signalHandler](const SelectionContext &) {
                                        signalHandler.parentModelNode().view()->executeInTransaction(
                                            "ConnectionsModelNodeActionGroup::"
                                            "changeSlot",
                                            [slot, signalHandler]() { slot.action(signalHandler); });
                                    });
                                editSlotGroup->addAction(newSlotAction);
                            }
                        } else {
                            for (const auto &slotCategory : slotsLists) {
                                QMenu *slotCategoryMenu = new QmlEditorMenu(slotCategory.categoryName, menu());
                                for (const auto &slot : slotCategory.slotEntries) {
                                    ActionTemplate *newSlotAction = new ActionTemplate(
                                        (slot.name + "Id").toLatin1(),
                                        slot.name,
                                        [slot, signalHandler](const SelectionContext &) {
                                            signalHandler.parentModelNode().view()->executeInTransaction(
                                                "ConnectionsModelNodeActionGroup::"
                                                "changeSlot",
                                                [slot, signalHandler]() {
                                                    slot.action(signalHandler);
                                                });
                                        });
                                    slotCategoryMenu->addAction(newSlotAction);
                                }
                                editSlotGroup->addMenu(slotCategoryMenu);
                            }
                        }
                        activeSignalHandlerGroup->addMenu(editSlotGroup);
                    }

                    ActionTemplate *openEditorAction = new ActionTemplate(
                        (propertyName + "OpenEditorId").toLatin1(),
                        QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Edit the Connection")),
                        [=](const SelectionContext &) {
                            QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("ConnectionView");
                            signalHandler.view()
                                ->emitCustomNotification(EditConnectionNotification,
                                                         {signalHandler.parentModelNode()},
                                                         {signalHandler.name().toByteArray()});
                            //ActionEditor::invokeEditor(signalHandler, removeSignal);
                        });

                    activeSignalHandlerGroup->addAction(openEditorAction);

                    ActionTemplate *removeSignalHandlerAction = new ActionTemplate(
                        (propertyName + "RemoveSignalHandlerId").toLatin1(),
                        QString(
                            QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Remove the Connection")),
                        [signalHandler](const SelectionContext &) {
                            signalHandler.parentModelNode().view()->executeInTransaction(
                                "ConnectionsModelNodeActionGroup::"
                                "removeSignalHandler",
                                [signalHandler]() { removeSignal(signalHandler); });
                        });

                    activeSignalHandlerGroup->addAction(removeSignalHandlerAction);

                    menu()->addMenu(activeSignalHandlerGroup);
                }
            }
        }

        //singular add connection:
        QMenu *addConnection = new QmlEditorMenu(QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                           "Add new Connection")),
                                                 menu());

        for (const auto &signalStr : signalsList) {
            QMenu *newSignal = new QmlEditorMenu(signalStr, addConnection);

            if (!slotsLists.isEmpty()) {
                if (slotsLists.size() == 1) {
                    for (const auto &slot : slotsLists.at(0).slotEntries) {
                        ActionTemplate *newSlot = new ActionTemplate(
                            QString(signalStr + slot.name + "Id").toLatin1(),
                            (slotsLists.at(0).categoryName
                             + (QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                  " to ")) //context: Change State _to_ state1
                             + slot.name),
                            [=](const SelectionContext &) {
                                currentNode.view()->executeInTransaction(
                                    "ConnectionsModelNodeActionGroup::addConnection", [=]() {
                                        ModelNode newConnectionNode = createNewConnection(currentNode);
                                        slot.action(newConnectionNode.signalHandlerProperty(
                                            prependSignal(signalStr).toLatin1()));
                                    });
                            });
                        newSignal->addAction(newSlot);
                    }
                } else {
                    for (const auto &slotCategory : slotsLists) {
                        QMenu *slotCategoryMenu = new QmlEditorMenu(slotCategory.categoryName, menu());
                        for (const auto &slot : slotCategory.slotEntries) {
                            ActionTemplate *newSlot = new ActionTemplate(
                                QString(signalStr + slot.name + "Id").toLatin1(),
                                slot.name,
                                [=](const SelectionContext &) {
                                    currentNode.view()->executeInTransaction(
                                        "ConnectionsModelNodeActionGroup::addConnection", [=]() {
                                            ModelNode newConnectionNode = createNewConnection(
                                                currentNode);
                                            slot.action(newConnectionNode.signalHandlerProperty(
                                                prependSignal(signalStr).toLatin1()));
                                        });
                                });
                            slotCategoryMenu->addAction(newSlot);
                        }
                        newSignal->addMenu(slotCategoryMenu);
                    }
                }
            }

            ActionTemplate *openEditorAction = new ActionTemplate(
                (signalStr + "OpenEditorId").toLatin1(),
                QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Add new Connection")),
                [=](const SelectionContext &) {
                    QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("ConnectionView");
                    currentNode.view()->emitCustomNotification(AddConnectionNotification,
                                                               {currentNode},
                                                               {signalStr});
                });
            newSignal->addAction(openEditorAction);

            addConnection->addMenu(newSignal);
        }

        menu()->addMenu(addConnection);
    }
};

class DocumentError : public std::exception
{
public:
    const char *what() const noexcept override { return "Current document contains errors."; }
};

class EditListModelAction final : public ModelNodeContextMenuAction
{
public:
    EditListModelAction()
        : ModelNodeContextMenuAction("EditListModel",
                                     ComponentCoreConstants::editListModelDisplayName,
                                     {},
                                     ComponentCoreConstants::rootCategory,
                                     QKeySequence("Alt+e"),
                                     ComponentCoreConstants::Priorities::EditListModel,
                                     &openDialog,
                                     &isListViewInBaseStateAndHasListModel,
                                     &isListViewInBaseStateAndHasListModel)
    {
        NanotraceHR::Tracer tracer{"designer action edit list model action constructor", category()};
    }

    static bool isListViewInBaseStateAndHasListModel(const SelectionContext &selectionState)
    {
        NanotraceHR::Tracer tracer{
            "designer action edit list model action is list view in base state and has list model",
            ComponentCoreTracing::category()};

        if (!selectionState.isInBaseState() || !selectionState.singleNodeIsSelected())
            return false;

        const ModelNode singleSelectedNode = selectionState.currentSingleSelectedNode();

        return singleSelectedNode.metaInfo().isListOrGridView()
               && singleSelectedNode.property("model").toNodeProperty().modelNode().documentTypeRepresentation()
                      == "ListModel";
    }

    bool isEnabled(const SelectionContext &) const override
    {
        NanotraceHR::Tracer tracer{"designer action edit list model action is enabled", category()};
        return true;
    }

    static void openDialog(const SelectionContext &selectionState)
    {
        NanotraceHR::Tracer tracer{"designer action edit list model action open dialog",
                                   ComponentCoreTracing::category()};

        ModelNode targetNode = selectionState.targetNode();
        if (!targetNode.isValid())
            targetNode = selectionState.currentSingleSelectedNode();
        if (!targetNode.isValid())
            return;

        AbstractView *view = targetNode.view();

        ListModelEditorModel model{[&] { return view->createModelNode("ListModel"); },
                                   [&] { return view->createModelNode("ListElement"); },
                                   [&](const ModelNode &node) {
                                       bool isNowInComponent = ModelNodeOperations::goIntoComponent(
                                           node);

                                       Model *currentModel = QmlDesignerPlugin::instance()
                                                                 ->currentDesignDocument()
                                                                 ->currentModel();

                                       if (currentModel->rewriterView()
                                           && !currentModel->rewriterView()->errors().isEmpty()) {
                                           throw DocumentError{};
                                       }

                                       if (isNowInComponent)
                                           return view->rootModelNode();

                                       return node;
                                   }};

        model.setListView(targetNode);

        ListModelEditorDialog dialog{Core::ICore::dialogParent()};
        dialog.setModel(&model);

        try {
            dialog.exec();
        } catch (const DocumentError &) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 Tr::tr("Document Has Errors"),
                                 Tr::tr("The document which contains the list model "
                                        "contains errors. So we cannot edit it."));
        } catch (const RewritingException &) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                                 Tr::tr("Document Cannot Be Written"),
                                 Tr::tr("An error occurred during a write attemp."));
        }
    }
};

const char xProperty[] = "x";
const char yProperty[] = "y";
const char widthProperty[] = "width";
const char heightProperty[] = "height";
const char triggerSlot[] = "trigger";

using namespace SelectionContextFunctors;

bool multiSelection(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action multi selection", category()};

    return !singleSelection(context) && selectionNotEmpty(context);
}

bool selectionHasProperty1or2(const SelectionContext &context, const char *x, const char *y)
{
    NanotraceHR::Tracer tracer{"designer action selection has property 1 or 2", category()};

    return selectionHasProperty(context, x) || selectionHasProperty(context, y);
}

bool selectionHasSameParentAndInBaseState(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action selection has same parent and in base state",
                               category()};

    return selectionHasSameParent(context) && inBaseState(context);
}

bool multiSelectionAndHasSameParent(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action multi selection and has same parent", category()};

    return multiSelection(context) && selectionHasSameParent(context);
}

bool selectionCanBeLayouted(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action selection can be layouted", category()};

    return multiSelection(context) && selectionHasSameParentAndInBaseState(context);
}

bool selectionCanBeLayoutedAndQtQuickLayoutPossible(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{
        "designer action selection can be layouted and qt quick layout possible", category()};

    return selectionCanBeLayouted(context);
}

bool selectionCanBeLayoutedAndQtQuickLayoutPossibleAndNotMCU(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{
        "designer action selection can be layouted and qt quick layout possible and not mcu",
        category()};

    return selectionCanBeLayoutedAndQtQuickLayoutPossible(context)
           && !DesignerMcuManager::instance().isMCUProject();
}

bool selectionNotEmptyAndHasWidthOrHeightProperty(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{
        "designer action selection not empty and has width or height property", category()};

    return selectionNotEmpty(context)
           && selectionHasProperty1or2(context, widthProperty, heightProperty);
}

bool singleSelectionItemIsNotAnchoredAndSingleSelectionNotRoot(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{
        "designer action single selection item is not anchored and single selection not root",
        category()};

    return singleSelectionItemIsNotAnchored(context) && singleSelectionNotRoot(context);
}

bool singleSelectionItemHasNoFillAnchorAndSingleSelectionNotRoot(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{
        "designer action single selection item has no fill anchor and single selection not root",
        category()};

    return singleSelection(context) && !singleSelectionItemHasAnchor(context, AnchorLineFill)
           && singleSelectionNotRoot(context);
}

bool selectionNotEmptyAndHasXorYProperty(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action selection not empty and has x or y property",
                               category()};

    return selectionNotEmpty(context) && selectionHasProperty1or2(context, xProperty, yProperty);
}

bool singleSelectionAndHasSlotTrigger(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action single selection and has slot trigger", category()};

    if (!singleSelection(context))
        return false;

    return selectionHasSlot(context, triggerSlot);
}

bool singleSelectionAndInQtQuickLayout(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action single selection and in qt quick layout", category()};

    if (!singleSelection(context))
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();
    if (!currentSelectedNode.isValid())
        return false;

    if (!currentSelectedNode.hasParentProperty())
        return false;

    ModelNode parentModelNode = currentSelectedNode.parentProperty().parentModelNode();

    NodeMetaInfo metaInfo = parentModelNode.metaInfo();

    return metaInfo.isQtQuickLayoutsLayout();
}

bool isStackedContainer(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action is stacked container", category()};

    if (!singleSelection(context))
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    return NodeHints::fromModelNode(currentSelectedNode).isStackedContainer();
}

bool isStackedContainerAndIndexIsVariantOrResolvableBinding(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{
        "designer action is stacked container and index is variant or resolvable binding", category()};

    if (!isStackedContainer(context))
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    const PropertyName propertyName = ModelNodeOperations::getIndexPropertyName(currentSelectedNode);

    QTC_ASSERT(currentSelectedNode.metaInfo().hasProperty(propertyName), return false);

    QmlItemNode containerItemNode(currentSelectedNode);

    QTC_ASSERT(containerItemNode.isValid(), return false);

    if (containerItemNode.hasBindingProperty(propertyName)) {
        const AbstractProperty resolvedProperty = containerItemNode.bindingProperty(propertyName)
                                                      .resolveToProperty();
        if (resolvedProperty.isValid() && resolvedProperty.isVariantProperty()) {
            auto variantProperty = resolvedProperty.toVariantProperty();
            if (!variantProperty.isValid())
                return false;

            return true;
        }

        return false;
    }

    return containerItemNode.modelNode().hasVariantProperty(propertyName);
}

bool isStackedContainerWithoutTabBar(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action is stacked container without tab bar", category()};

    if (!isStackedContainer(context))
        return false;

    if (!context.view()->model())
        return false;

    if (!context.view()->model()->metaInfo("TabBar").isValid())
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    const PropertyName propertyName = ModelNodeOperations::getIndexPropertyName(currentSelectedNode);

    QTC_ASSERT(currentSelectedNode.metaInfo().hasProperty(propertyName), return false);

    BindingProperty binding = currentSelectedNode.bindingProperty(propertyName);

    /* There is already a TabBar or something similar attached */
    return !(binding.isValid() && binding.resolveToProperty().isValid());
}

bool isStackedContainerAndIndexCanBeDecreased(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action is stacked container and index can be decreased",
                               category()};

    if (!isStackedContainer(context))
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    const PropertyName propertyName = ModelNodeOperations::getIndexPropertyName(currentSelectedNode);

    QTC_ASSERT(currentSelectedNode.metaInfo().hasProperty(propertyName), return false);

    QmlItemNode containerItemNode(currentSelectedNode);

    QTC_ASSERT(containerItemNode.isValid(), return false);

    auto isMoreThan = [](const QVariant &variant, int min) -> bool {
        if (!variant.isValid())
            return false;

        bool ok = false;
        int value = variant.toInt(&ok);
        return ok && value > min;
    };

    if (currentSelectedNode.hasBindingProperty(propertyName)) {
        const AbstractProperty resolvedProperty = currentSelectedNode.bindingProperty(propertyName)
                                                      .resolveToProperty();
        if (resolvedProperty.isValid() && resolvedProperty.isVariantProperty()) {
            const auto variantProperty = resolvedProperty.toVariantProperty();
            if (!variantProperty.isValid())
                return false;

            if (isMoreThan(variantProperty.value(), 0))
                return true;
        }

        return false;
    }

    QVariant modelValue = containerItemNode.modelValue(propertyName);
    if (isMoreThan(modelValue, 0))
        return true;

    return false;
}

bool isStackedContainerAndIndexCanBeIncreased(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action is stacked container and index can be increased",
                               category()};

    if (!isStackedContainer(context))
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    const PropertyName propertyName = ModelNodeOperations::getIndexPropertyName(currentSelectedNode);

    QTC_ASSERT(currentSelectedNode.metaInfo().hasProperty(propertyName), return false);

    QmlItemNode containerItemNode(currentSelectedNode);

    QTC_ASSERT(containerItemNode.isValid(), return false);

    auto isLessThan = [](const QVariant &variant, int max) -> bool {
        if (!variant.isValid())
            return false;

        bool ok = false;
        int value = variant.toInt(&ok);
        return ok && value < max;
    };

    const int maxValue = currentSelectedNode.directSubModelNodes().size() - 1;

    if (currentSelectedNode.hasBindingProperty(propertyName)) {
        const AbstractProperty resolvedProperty = currentSelectedNode.bindingProperty(propertyName)
                                                      .resolveToProperty();
        if (resolvedProperty.isValid() && resolvedProperty.isVariantProperty()) {
            const auto variantProperty = resolvedProperty.toVariantProperty();
            if (!variantProperty.isValid())
                return false;

            if (isLessThan(variantProperty.value(), maxValue))
                return true;
        }

        return false;
    }

    QVariant modelValue = containerItemNode.modelValue(propertyName);
    if (isLessThan(modelValue, maxValue))
        return true;

    return false;
}

bool isLayout(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action is layout", category()};

    if (!inBaseState(context))
        return false;

    if (!singleSelection(context))
        return false;

    NodeMetaInfo metaInfo = context.currentSingleSelectedNode().metaInfo();

    if (!metaInfo.isValid())
        return false;

    /* Stacked containers have different semantics */
    if (isStackedContainer(context))
            return false;

    return metaInfo.isQtQuickLayoutsLayout();
}

bool isPositioner(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action is positioner", category()};

    if (!inBaseState(context))
        return false;

    if (!singleSelection(context))
        return false;

    const NodeMetaInfo metaInfo = context.currentSingleSelectedNode().metaInfo();

    return metaInfo.isQtQuickPositioner();
}

bool layoutOptionVisible(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action layout option visible", category()};

    return (selectionCanBeLayoutedAndQtQuickLayoutPossible(context)
            || singleSelectionAndInQtQuickLayout(context) || isLayout(context))
           && !DesignerMcuManager::instance().isMCUProject();
}

bool positionOptionVisible(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action position option visible", category()};

    return selectionCanBeLayouted(context) || isPositioner(context);
}

bool singleSelectedAndUiFile(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action single selected and ui file", category()};

    if (!singleSelection(context))
        return false;

    DesignDocument *designDocument = QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();

    if (!designDocument)
        return false;

    return designDocument->fileName().completeSuffix() == QLatin1String("ui.qml");
}

bool lowerAvailable(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"designer action lower available", category()};

    if (!singleSelection(selectionState))
        return false;

    ModelNode modelNode = selectionState.currentSingleSelectedNode();

    if (modelNode.isRootNode())
        return false;

    if (!modelNode.hasParentProperty())
        return false;

    if (!modelNode.parentProperty().isNodeListProperty())
        return false;

    NodeListProperty parentProperty = modelNode.parentProperty().toNodeListProperty();
    return parentProperty.indexOf(modelNode) > 0;
}

bool raiseAvailable(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"designer action raise available", category()};

    if (!singleSelection(selectionState))
        return false;

    ModelNode modelNode = selectionState.currentSingleSelectedNode();

    if (modelNode.isRootNode())
        return false;

    if (!modelNode.hasParentProperty())
        return false;

    if (!modelNode.parentProperty().isNodeListProperty())
        return false;

    NodeListProperty parentProperty = modelNode.parentProperty().toNodeListProperty();
    return parentProperty.indexOf(modelNode) < parentProperty.count() - 1;
}

bool anchorsMenuEnabled(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"designer action anchors menu enabled", category()};

    return singleSelectionItemIsNotAnchoredAndSingleSelectionNotRoot(context)
           || singleSelectionItemIsAnchored(context);
}

QIcon createResetIcon(const QStringList &basicIconAddresses)
{
    NanotraceHR::Tracer tracer{"designer action create reset icon", category()};

    using namespace Utils;
    static const IconMaskAndColor resetMask({":/utils/images/iconoverlay_reset.png",
                                             Theme::IconsStopToolBarColor});
    QList<IconMaskAndColor> iconMaskList = transform(basicIconAddresses, [=] (const QString &refAddr) {
        return IconMaskAndColor(
                    FilePath::fromString(refAddr),
                    Theme::IconsBaseColor);
    });

    QIcon finalIcon = Icon(iconMaskList).icon();
    iconMaskList.append(resetMask);
    QIcon finalOn = Icon(iconMaskList).icon();
    for (const QSize &iSize : finalIcon.availableSizes()) {
        for (const QIcon::Mode &mode : {QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected})
            finalIcon.addPixmap(finalOn.pixmap(iSize, mode, QIcon::On), mode, QIcon::On);
    }
    return finalIcon;
}

} // namespace

void DesignerActionManager::createDefaultDesignerActions()
{
    NanotraceHR::Tracer tracer{"designer action manager create default actions", category()};

    using namespace SelectionContextFunctors;
    using namespace ComponentCoreConstants;
    using namespace ModelNodeOperations;
    using namespace FormatOperation;

    const Utils::Icon prevIcon({
        {":/utils/images/prev.png", Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);

    const Utils::Icon nextIcon({
        {":/utils/images/next.png", Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);

    const Utils::Icon addIcon({
        {":/utils/images/plus.png", Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);

    addDesignerAction(new SelectionModelNodeAction(
                          selectionCategoryDisplayName,
                          selectionCategory,
                          contextIcon(DesignerIcons::SelecionIcon),
                          Priorities::SelectionCategory));

    addDesignerAction(new ConnectionsModelNodeActionGroup(
                          connectionsCategoryDisplayName,
                          connectionsCategory,
                          contextIcon(DesignerIcons::ConnectionsIcon),
                          Priorities::ConnectionsCategory));

    addDesignerAction(new ActionGroup(
                          arrangeCategoryDisplayName,
                          arrangeCategory,
                          contextIcon(DesignerIcons::ArrangeIcon),
                          Priorities::ArrangeCategory,
                          &selectionNotEmpty));

    addDesignerAction(new SeparatorDesignerAction(arrangeCategory, 10));

    addDesignerAction(new ModelNodeContextMenuAction(
                          toFrontCommandId,
                          toFrontDisplayName,
                          {},
                          arrangeCategory,
                          QKeySequence(),
                          1,
                          &toFront,
                          &raiseAvailable));

    addDesignerAction(new ModelNodeContextMenuAction(
                          raiseCommandId,
                          raiseDisplayName,
                          {},
                          arrangeCategory,
                          QKeySequence(),
                          11,
                          &raise,
                          &raiseAvailable));

    addDesignerAction(new ModelNodeContextMenuAction(
                          lowerCommandId,
                          lowerDisplayName,
                          {},
                          arrangeCategory,
                          QKeySequence(),
                          12,
                          &lower,
                          &lowerAvailable));

    addDesignerAction(new ModelNodeContextMenuAction(
                          toBackCommandId,
                          toBackDisplayName,
                          {},
                          arrangeCategory,
                          QKeySequence(),
                          2,
                          &toBack,
                          &lowerAvailable));

    addDesignerAction(new SeparatorDesignerAction(arrangeCategory, 20));

    addDesignerAction(new ModelNodeContextMenuAction(
                          reverseCommandId,
                          reverseDisplayName,
                          {},
                          arrangeCategory,
                          QKeySequence(),
                          21,
                          &reverse,
                          &multiSelectionAndHasSameParent));

    addDesignerAction(new ActionGroup(editCategoryDisplayName,
                                      editCategory,
                                      contextIcon(DesignerIcons::EditIcon),
                                      Priorities::EditCategory,
                                      &selectionNotEmpty));

    addDesignerAction(new SeparatorDesignerAction(editCategory, 30));

    addDesignerAction(new ModelNodeAction(
                          resetPositionCommandId,
                          resetPositionDisplayName,
                          createResetIcon({":/utils/images/pan.png"}),
                          resetPositionTooltip,
                          editCategory,
                          QKeySequence("Ctrl+d"),
                          32,
                          &resetPosition,
                          &selectionNotEmptyAndHasXorYProperty));

    addDesignerAction(new ModelNodeAction(
                          copyFormatCommandId,
                          copyFormatDisplayName,
                          contextIcon(DesignerIcons::CopyIcon),
                          copyFormatTooltip,
                          editCategory,
                          QKeySequence(),
                          41,
                          &copyFormat,
                          &propertiesCopyable));

    addDesignerAction(new ModelNodeAction(
                          applyFormatCommandId,
                          applyFormatDisplayName,
                          contextIcon(DesignerIcons::PasteIcon),
                          applyFormatTooltip,
                          editCategory,
                          QKeySequence(),
                          42,
                          &applyFormat,
                          &propertiesApplyable));

    addDesignerAction(new ModelNodeAction(
                          resetSizeCommandId,
                          resetSizeDisplayName,
                          createResetIcon({":/utils/images/fittoview.png"}),
                          resetSizeToolTip,
                          editCategory,
                          QKeySequence("shift+s"),
                          31,
                          &resetSize,
                          &selectionNotEmptyAndHasWidthOrHeightProperty));

    addDesignerAction(new ModelNodeAction(
                          isolateSelectionCommandId,
                          isolateSelectionDisplayName,
                          contextIcon(DesignerIcons::VisibilityIcon), // TODO: placeholder icon
                          isolateNodesToolTip,
                          rootCategory,
                          QKeySequence("shift+b"),
                          Priorities::IsolateSelection,
                          &isolateSelectedNodes,
                          &selectionNot2D3DMix));

    addDesignerAction(new ModelNodeAction(
                          showAllCommandId,
                          showAllDisplayName,
                          contextIcon(DesignerIcons::VisibilityIcon), // TODO: placeholder icon
                          showAllToolTip,
                          rootCategory,
                          QKeySequence("alt+b"),
                          Priorities::ShowAllNodes,
                          &showAllNodes,
                          &always));

    addDesignerAction(new SeparatorDesignerAction(editCategory, 40));

    addDesignerAction(new VisiblityModelNodeAction(
                          visiblityCommandId,
                          visibilityDisplayName,
                          contextIcon(DesignerIcons::VisibilityIcon),
                          rootCategory,
                          QKeySequence("Ctrl+g"),
                          Priorities::Visibility,
                          &setVisible,
                          &singleSelectedItem));

    addDesignerAction(new ActionGroup(anchorsCategoryDisplayName,
                                      anchorsCategory,
                                      contextIcon(DesignerIcons::AnchorsIcon),
                                      Priorities::AnchorsCategory,
                                      &anchorsMenuEnabled));

    addDesignerAction(new ModelNodeAction(
        anchorsFillCommandId,
        anchorsFillDisplayName,
        Utils::Icon({{":/qmldesigner/images/anchor_fill.png", Utils::Theme::IconsBaseColor}}).icon(),
        anchorsFillToolTip,
        anchorsCategory,
        QKeySequence(QKeySequence("shift+f")),
        2,
        &anchorsFill,
        &singleSelectionItemHasNoFillAnchorAndSingleSelectionNotRoot));

    addDesignerAction(new ModelNodeAction(
                          anchorsResetCommandId,
                          anchorsResetDisplayName,
                          Utils::Icon({{":/qmldesigner/images/anchor_fill.png", Utils::Theme::IconsBaseColor},
                                       {":/utils/images/iconoverlay_reset.png", Utils::Theme::IconsStopToolBarColor}}).icon(),
                          anchorsResetToolTip,
                          anchorsCategory,
                          QKeySequence(QKeySequence("Ctrl+Shift+r")),
                          1,
                          &anchorsReset,
                          &singleSelectionItemIsAnchored));

    addDesignerAction(new SeparatorDesignerAction(anchorsCategory, 10));

    addDesignerAction(new ParentAnchorAction(
                          anchorParentTopAndBottomCommandId,
                          anchorParentTopAndBottomDisplayName,
                          createResetIcon({":/qmldesigner/images/anchor_top.png",
                                           ":/qmldesigner/images/anchor_bottom.png"}),
                          {},
                          anchorsCategory,
                          QKeySequence(),
                          11,
                          AnchorLineType(AnchorLineTop | AnchorLineBottom)));

    addDesignerAction(new ParentAnchorAction(
                          anchorParentLeftAndRightCommandId,
                          anchorParentLeftAndRightDisplayName,
                          createResetIcon({":/qmldesigner/images/anchor_left.png",
                                           ":/qmldesigner/images/anchor_right.png"}),
                          {},
                          anchorsCategory,
                          QKeySequence(),
                          12,
                          AnchorLineType(AnchorLineLeft | AnchorLineRight)));

    addDesignerAction(new SeparatorDesignerAction(anchorsCategory, 20));

    addDesignerAction(new ParentAnchorAction(
                          anchorParentTopCommandId,
                          anchorParentTopDisplayName,
                          createResetIcon({":/qmldesigner/images/anchor_top.png"}),
                          {},
                          anchorsCategory,
                          QKeySequence(),
                          21,
                          AnchorLineTop));

    addDesignerAction(new ParentAnchorAction(
                          anchorParentBottomCommandId,
                          anchorParentBottomDisplayName,
                          createResetIcon({":/qmldesigner/images/anchor_bottom.png"}),
                          {},
                          anchorsCategory,
                          QKeySequence(),
                          22,
                          AnchorLineBottom));

    addDesignerAction(new ParentAnchorAction(
                          anchorParentLeftCommandId,
                          anchorParentLeftDisplayName,
                          createResetIcon({":/qmldesigner/images/anchor_left.png"}),
                          {},
                          anchorsCategory,
                          QKeySequence(),
                          23,
                          AnchorLineLeft));

    addDesignerAction(new ParentAnchorAction(
                          anchorParentRightCommandId,
                          anchorParentRightDisplayName,
                          createResetIcon({":/qmldesigner/images/anchor_right.png"}),
                          {},
                          anchorsCategory,
                          QKeySequence(),
                          24,
                          AnchorLineRight));

    addDesignerAction(new SeparatorDesignerAction(anchorsCategory, 30));

    addDesignerAction(
        new ParentAnchorAction(anchorParentVerticalCenterCommandId,
                               anchorParentVerticalCenterDisplayName,
                               createResetIcon({":/qmldesigner/images/anchor_vertical.png"}),
                               {},
                               anchorsCategory,
                               QKeySequence(),
                               31,
                               AnchorLineVerticalCenter));

    addDesignerAction(
        new ParentAnchorAction(anchorParentHorizontalCenterCommandId,
                               anchorParentHorizontalCenterDisplayName,
                               createResetIcon({":/qmldesigner/images/anchor_horizontal.png"}),
                               {},
                               anchorsCategory,
                               QKeySequence(),
                               32,
                               AnchorLineHorizontalCenter));

    addDesignerAction(new ActionGroup(
                          positionerCategoryDisplayName,
                          positionerCategory,
                          contextIcon(DesignerIcons::PositionsersIcon),
                          Priorities::PositionCategory,
                          &positionOptionVisible));

    addDesignerAction(new ActionGroup(
                          layoutCategoryDisplayName,
                          layoutCategory,
                          contextIcon(DesignerIcons::LayoutsIcon),
                          Priorities::LayoutCategory,
                          &layoutOptionVisible));

    addDesignerAction(new ActionGroup(
                          snappingCategoryDisplayName,
                          snappingCategory,
                          contextIcon(DesignerIcons::SnappingIcon),
                          Priorities::SnappingCategory,
                          &selectionEnabled,
                          &selectionEnabled));

    addDesignerAction(new GroupItemAction(
                          contextIcon(DesignerIcons::GroupSelectionIcon),
                          {},
                          Priorities::Group));

    addDesignerAction(new ActionGroup(stackedContainerCategoryDisplayName,
                                      stackedContainerCategory,
                                      addIcon.icon(),
                                      Priorities::StackedContainerCategory,
                                      &isStackedContainer,
                                      &isStackedContainer));

    addDesignerAction(new ModelNodeContextMenuAction(
                          removePositionerCommandId,
                          removePositionerDisplayName,
                          {},
                          positionerCategory,
                          QKeySequence("Ctrl+Shift+p"),
                          1,
                          &removePositioner,
                          &isPositioner,
                          &isPositioner));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutRowPositionerCommandId,
                          layoutRowPositionerDisplayName,
                          {},
                          positionerCategory,
                          QKeySequence(),
                          2,
                          &layoutRowPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutColumnPositionerCommandId,
                          layoutColumnPositionerDisplayName,
                          {},
                          positionerCategory,
                          QKeySequence(),
                          3,
                          &layoutColumnPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutGridPositionerCommandId,
                          layoutGridPositionerDisplayName,
                          {},
                          positionerCategory,
                          QKeySequence(),
                          4,
                          &layoutGridPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutFlowPositionerCommandId,
                          layoutFlowPositionerDisplayName,
                          {},
                          positionerCategory,
                          QKeySequence("Ctrl+m"),
                          5,
                          &layoutFlowPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new SeparatorDesignerAction(layoutCategory, 0));

    addDesignerAction(new ModelNodeContextMenuAction(
                          removeLayoutCommandId,
                          removeLayoutDisplayName,
                          {},
                          layoutCategory,
                          QKeySequence(),
                          1,
                          &removeLayout,
                          &isLayout,
                          &isLayout));

    addDesignerAction(new ModelNodeFormEditorAction(
                          addItemToStackedContainerCommandId,
                          addItemToStackedContainerDisplayName,
                          addIcon.icon(),
                          addItemToStackedContainerToolTip,
                          stackedContainerCategory,
                          QKeySequence("Ctrl+Shift+a"),
                          1,
                          &addItemToStackedContainer,
                          &isStackedContainer,
                          &isStackedContainer));

    addDesignerAction(new ModelNodeContextMenuAction(addTabBarToStackedContainerCommandId,
                                                     addTabBarToStackedContainerDisplayName,
                                                     {},
                                                     stackedContainerCategory,
                                                     QKeySequence("Ctrl+Shift+t"),
                                                     2,
                                                     &addTabBarToStackedContainer,
                                                     &isStackedContainerWithoutTabBar,
                                                     &isStackedContainer));

    addDesignerAction(
        new ModelNodeFormEditorAction(decreaseIndexOfStackedContainerCommandId,
                                      decreaseIndexToStackedContainerDisplayName,
                                      prevIcon.icon(),
                                      decreaseIndexOfStackedContainerToolTip,
                                      stackedContainerCategory,
                                      QKeySequence("Ctrl+Shift+Left"),
                                      3,
                                      &decreaseIndexOfStackedContainer,
                                      &isStackedContainerAndIndexCanBeDecreased,
                                      &isStackedContainerAndIndexIsVariantOrResolvableBinding));
    addDesignerAction(
        new ModelNodeFormEditorAction(increaseIndexOfStackedContainerCommandId,
                                      increaseIndexToStackedContainerDisplayName,
                                      nextIcon.icon(),
                                      increaseIndexOfStackedContainerToolTip,
                                      stackedContainerCategory,
                                      QKeySequence("Ctrl+Shift+Right"),
                                      4,
                                      &increaseIndexOfStackedContainer,
                                      &isStackedContainerAndIndexCanBeIncreased,
                                      &isStackedContainerAndIndexIsVariantOrResolvableBinding));

    addDesignerAction(
        new ModelNodeAction(layoutRowLayoutCommandId,
                            layoutRowLayoutDisplayName,
                            Utils::Icon({{":/qmldesigner/icon/designeractions/images/row.png",
                                          Utils::Theme::IconsBaseColor}})
                                .icon(),
                            layoutRowLayoutToolTip,
                            layoutCategory,
                            QKeySequence("Ctrl+u"),
                            2,
                            &layoutRowLayout,
                            &selectionCanBeLayoutedAndQtQuickLayoutPossibleAndNotMCU));

    addDesignerAction(
        new ModelNodeAction(layoutColumnLayoutCommandId,
                            layoutColumnLayoutDisplayName,
                            Utils::Icon({{":/qmldesigner/icon/designeractions/images/column.png",
                                          Utils::Theme::IconsBaseColor}})
                                .icon(),
                            layoutColumnLayoutToolTip,
                            layoutCategory,
                            QKeySequence("Ctrl+l"),
                            3,
                            &layoutColumnLayout,
                            &selectionCanBeLayoutedAndQtQuickLayoutPossibleAndNotMCU));

    addDesignerAction(new ModelNodeAction(
                          layoutGridLayoutCommandId,
                          layoutGridLayoutDisplayName,
                          Utils::Icon({{":/qmldesigner/icon/designeractions/images/grid.png",
                                        Utils::Theme::IconsBaseColor}}).icon(),
                          layoutGridLayoutToolTip,
                          layoutCategory,
                          QKeySequence("shift+g"),
                          4,
                          &layoutGridLayout,
                          &selectionCanBeLayoutedAndQtQuickLayoutPossibleAndNotMCU));

    addDesignerAction(new SeparatorDesignerAction(layoutCategory, 10));

    addDesignerAction(new FillWidthModelNodeAction(
                          layoutFillWidthCommandId,
                          layoutFillWidthDisplayName,
                          layoutCategory,
                          QKeySequence(),
                          11,
                          &setFillWidth,
                          &singleSelectionAndInQtQuickLayout,
                          &singleSelectionAndInQtQuickLayout));

    addDesignerAction(new FillHeightModelNodeAction(
                          layoutFillHeightCommandId,
                          layoutFillHeightDisplayName,
                          layoutCategory,
                          QKeySequence(),
                          12,
                          &setFillHeight,
                          &singleSelectionAndInQtQuickLayout,
                          &singleSelectionAndInQtQuickLayout));

    addDesignerAction(new SeparatorDesignerAction(rootCategory, Priorities::ModifySection));
    addDesignerAction(new SeparatorDesignerAction(rootCategory, Priorities::PositionSection));
    addDesignerAction(new SeparatorDesignerAction(rootCategory, Priorities::EventSection));
    addDesignerAction(new SeparatorDesignerAction(rootCategory, Priorities::AdditionsSection));
    addDesignerAction(new SeparatorDesignerAction(rootCategory, Priorities::ViewOprionsSection));
    addDesignerAction(new SeparatorDesignerAction(rootCategory, Priorities::CustomActionsSection));

    addDesignerAction(new ModelNodeContextMenuAction(goIntoComponentCommandId,
                                                     enterComponentDisplayName,
                                                     contextIcon(DesignerIcons::EnterComponentIcon),
                                                     rootCategory,
                                                     QKeySequence(Qt::Key_F2),
                                                     Priorities::ComponentActions + 5,
                                                     &goIntoComponentOperation,
                                                     &selectionIsEditableComponent));

    addDesignerAction(new ModelNodeContextMenuAction(jumpToCodeCommandId,
                                                     JumpToCodeDisplayName,
                                                     contextIcon(DesignerIcons::JumpToCodeIcon),
                                                     rootCategory,
                                                     QKeySequence(Qt::Key_F4),
                                                     Priorities::JumpToCode,
                                                     &jumpToCodeOperation,
                                                     &singleSelection));

    addDesignerAction(new ModelNodeContextMenuAction(
                          editAnnotationsCommandId,
                          editAnnotationsDisplayName,
                          contextIcon(DesignerIcons::AnnotationIcon),
                          rootCategory,
                          QKeySequence(),
                          Priorities::EditAnnotations,
                          &editAnnotation,
                          &singleSelection,
                          &singleSelection));

    addDesignerAction(new ModelNodeContextMenuAction(
                          addMouseAreaFillCommandId,
                          addMouseAreaFillDisplayName,
                          contextIcon(DesignerIcons::AddMouseAreaIcon),
                          rootCategory,
                          QKeySequence(),
                          Priorities::AddMouseArea,
                          &addMouseAreaFill,
                          &addMouseAreaFillCheck,
                          &singleSelection));

    if (!Core::ICore::isQtDesignStudio()) {
        addDesignerAction(new ModelNodeContextMenuAction(goToImplementationCommandId,
                                                         goToImplementationDisplayName,
                                                         {},
                                                         rootCategory,
                                                         QKeySequence(),
                                                         42,
                                                         &goImplementation,
                                                         &singleSelectedAndUiFile,
                                                         &singleSelectedAndUiFile));
    }

    addDesignerAction(new ModelNodeContextMenuAction(
                          editIn3dViewCommandId,
                          editIn3dViewDisplayName,
                          contextIcon(DesignerIcons::EditIcon),
                          rootCategory,
                          QKeySequence(),
                          Priorities::ComponentActions + 1,
                          &editIn3dView,
                          &SelectionContextFunctors::always, // If action is visible, it is usable
                          &singleSelectionView3D));

    addDesignerAction(new ModelNodeContextMenuAction(
                          makeComponentCommandId,
                          makeComponentDisplayName,
                          contextIcon(DesignerIcons::MakeComponentIcon),
                          rootCategory,
                          QKeySequence(),
                          Priorities::ComponentActions + 2,
                          &moveToComponent,
                          &singleSelection,
                          &singleSelection));

    addDesignerAction(new ModelNodeContextMenuAction(
        extractComponentCommandId,
        extractComponentDisplayName,
        contextIcon(DesignerIcons::MakeComponentIcon),
        rootCategory,
        QKeySequence(),
        Priorities::ComponentActions + 3,
        &extractComponent,
        &singleSelection,
        &isFileComponent));

    addDesignerAction(new ModelNodeContextMenuAction(
        editInEffectComposerCommandId,
        editInEffectComposerDisplayName,
        contextIcon(DesignerIcons::EditIcon),
        rootCategory,
        QKeySequence(),
        Priorities::ComponentActions + 4,
        &editInEffectComposer,
        &SelectionContextFunctors::always, // If action is visible, it is usable
        &singleSelectionEffectComposer));

    addDesignerAction(new ModelNodeContextMenuAction(
        importComponentCommandId,
        importComponentDisplayName,
        contextIcon(DesignerIcons::CreateIcon), // TODO: placeholder icon
        rootCategory,
        QKeySequence(),
        Priorities::ImportComponent,
        [&](const SelectionContext &) { m_bundleHelper->importBundleToProject(); }));

    addDesignerAction(new ModelNodeContextMenuAction(
        exportComponentCommandId,
        exportComponentDisplayName,
        contextIcon(DesignerIcons::CreateIcon), // TODO: placeholder icon
        rootCategory,
        QKeySequence(),
        Priorities::ExportComponent,
        [&](const SelectionContext &context) {
            m_bundleHelper->exportBundle(context.selectedModelNodes());
        },
        &are3DNodes,
        &are3DNodes));

    addDesignerAction(new ModelNodeContextMenuAction(
                          editMaterialCommandId,
                          editMaterialDisplayName,
                          contextIcon(DesignerIcons::EditIcon),
                          rootCategory,
                          QKeySequence(),
                          44,
                          &editMaterial,
                          &hasEditableMaterial,
                          &isModelOrMaterial));

    addDesignerAction(new ModelNodeContextMenuAction(
        mergeTemplateCommandId,
        mergeTemplateDisplayName,
        contextIcon(DesignerIcons::MergeWithTemplateIcon),
        rootCategory,
        {},
        Priorities::MergeWithTemplate,
        [&](const SelectionContext &context) {
            mergeWithTemplate(context, m_externalDependencies, m_modulesStorage);
        },
        &SelectionContextFunctors::always));

    addDesignerAction(new ModelNodeContextMenuAction(
                          addToContentLibraryCommandId,
                          addToContentLibraryDisplayName,
                          contextIcon(DesignerIcons::CreateIcon), // TODO: placeholder icon
                          rootCategory,
                          QKeySequence(),
                          Priorities::Add3DToContentLib,
                          &addNodeToContentLibrary,
                          &enableAddToContentLib,
                          &enableAddToContentLib));

    addDesignerAction(new ActionGroup(
                          "",
                          genericToolBarCategory,
                          {},
                          Priorities::GenericToolBar));

    addDesignerAction(new ChangeStyleAction());

    addDesignerAction(new EditListModelAction);

    addDesignerAction(new ModelNodeContextMenuAction(openSignalDialogCommandId,
                                                     openSignalDialogDisplayName,
                                                     {},
                                                     rootCategory,
                                                     QKeySequence(),
                                                     Priorities::SignalsDialog,
                                                     &openSignalDialog,
                                                     &singleSelectionAndHasSlotTrigger,
                                                     &singleSelectionAndHasSlotTrigger));

    addDesignerAction(new ModelNodeContextMenuAction(
                          update3DAssetCommandId,
                          update3DAssetDisplayName,
                          {},
                          rootCategory,
                          QKeySequence(),
                          Priorities::GenericToolBar,
                          &updateImported3DAsset,
                          &selectionIsImported3DAsset,
                          &selectionIsImported3DAsset));
}

void DesignerActionManager::createDefaultAddResourceHandler()
{
    NanotraceHR::Tracer tracer{"designer action manager create default add resource handlers",
                               category()};

    auto registerHandlers =
        [this](const QStringList &exts, AddResourceOperation op, const QString &category) {
            for (const QString &ext : exts)
                registerAddResourceHandler(AddResourceHandler(category, ext, op));
        };

    // Having a single image type category creates too large of a filter, so we split images into
    // categories according to their mime types
    auto transformer = [](const QByteArray& format) -> QString { return QString("*.") + format; };
    auto imageFormats = Utils::transform(QImageReader::supportedImageFormats(), transformer);
    imageFormats.push_back("*.hdr");
    imageFormats.push_back("*.ktx");

    // The filters will be displayed in reverse order to these lists in file dialog,
    // so declare most common types last
    registerHandlers(imageFormats,
                     ModelNodeOperations::addImageToProject,
                     ComponentCoreConstants::addImagesDisplayString);
    registerHandlers({"*.otf", "*.ttf"},
                     ModelNodeOperations::addFontToProject,
                     ComponentCoreConstants::addFontsDisplayString);
    registerHandlers({"*.wav", "*.mp3"},
                     ModelNodeOperations::addSoundToProject,
                     ComponentCoreConstants::addSoundsDisplayString);
    registerHandlers({"*.glsl", "*.glslv", "*.glslf", "*.vsh", "*.fsh", "*.vert", "*.frag"},
                     ModelNodeOperations::addShaderToProject,
                     ComponentCoreConstants::addShadersDisplayString);
    registerHandlers({"*.mp4"},
                     ModelNodeOperations::addVideoToProject,
                     ComponentCoreConstants::addVideosDisplayString);
}

void DesignerActionManager::createDefaultModelNodePreviewImageHandlers()
{
    NanotraceHR::Tracer tracer{
        "designer action manager create default model node preview image handlers", category()};

    registerModelNodePreviewHandler(
        ModelNodePreviewImageHandler("Image", ModelNodeOperations::previewImageDataForImageNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("BorderImage",
                                             ModelNodeOperations::previewImageDataForImageNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("SafeRendererImage",
                                             ModelNodeOperations::previewImageDataForImageNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("SafeRendererPicture",
                                             ModelNodeOperations::previewImageDataForImageNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("Texture",
                                             ModelNodeOperations::previewImageDataForImageNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("Material",
                                             ModelNodeOperations::previewImageDataForGenericNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("Model",
                                             ModelNodeOperations::previewImageDataForGenericNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("Node",
                                             ModelNodeOperations::previewImageDataForGenericNode,
                                             true));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("Item",
                                             ModelNodeOperations::previewImageDataForGenericNode,
                                             true));
}

void DesignerActionManager::addDesignerAction(ActionInterface *newAction)
{
    NanotraceHR::Tracer tracer{"designer action manager add designer action", category()};

    m_designerActions.append(QSharedPointer<ActionInterface>(newAction));

    for (auto callback : m_callBacks) {
        callback(newAction);
    }
}

void DesignerActionManager::addCreatorCommand(Core::Command *command, const QByteArray &category, int priority,
                                              const QIcon &overrideIcon)
{
    NanotraceHR::Tracer tracer{"designer action manager add creator command",
                               ComponentCoreTracing::category()};

    addDesignerAction(new CommandAction(command, category, priority, overrideIcon));
}

QList<QSharedPointer<ActionInterface> > DesignerActionManager::actionsForTargetView(const ActionInterface::TargetView &target)
{
    NanotraceHR::Tracer tracer{"designer action manager actions for target view", category()};

    QList<QSharedPointer<ActionInterface>> out;
    for (auto interface : std::as_const(m_designerActions))
        if (interface->targetView() == target)
            out << interface;

    return out;
}

QList<ActionInterface* > DesignerActionManager::designerActions() const
{
    NanotraceHR::Tracer tracer{"designer action manager get designer actions", category()};

    return Utils::transform(m_designerActions, &QSharedPointer<ActionInterface>::get);
}

ActionInterface *DesignerActionManager::actionByMenuId(const QByteArray &id)
{
    NanotraceHR::Tracer tracer{"designer action manager get action by menu id", category()};

    for (const auto &action : m_designerActions)
        if (action->menuId() == id)
            return action.data();
    return nullptr;
}

DesignerActionManager::DesignerActionManager(DesignerActionManagerView *designerActionManagerView,
                                             ExternalDependenciesInterface &externalDependencies,
                                             ModulesStorage &modulesStorage)
    : m_designerActionManagerView(designerActionManagerView)
    , m_externalDependencies(externalDependencies)
    , m_modulesStorage(modulesStorage)
    , m_bundleHelper(std::make_unique<BundleHelper>(designerActionManagerView,
                                                    QmlDesignerPlugin::instance()->mainWidget()))
{
    NanotraceHR::Tracer tracer{"designer action manager constructor", category()};

    setupIcons();
}

DesignerActionManager::~DesignerActionManager()
{
    NanotraceHR::Tracer tracer{"designer action manager destructor", category()};
}

void DesignerActionManager::setupIcons()
{
    NanotraceHR::Tracer tracer{"designer action manager setup icons", category()};

    m_designerIcons = std::make_unique<DesignerIcons>("qtds_propertyIconFont.ttf",
                                                      designerIconResourcesPath());
}

QString DesignerActionManager::designerIconResourcesPath() const
{
    NanotraceHR::Tracer tracer{"designer action manager designer icon resources path", category()};

#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/designericons.json";
#endif
    return Core::ICore::resourcePath("qmldesigner/designericons.json").toUrlishString();
}

DesignerActionToolBar::DesignerActionToolBar(QWidget *parentWidget) : Utils::StyledBar(parentWidget),
    m_toolBar(new QToolBar("ActionToolBar", this))
{
    NanotraceHR::Tracer tracer{"designer action tool bar constructor", "designer action manager"};

    QWidget *empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    m_toolBar->addWidget(empty);

    m_toolBar->setContentsMargins(0, 0, 0, 0);
    m_toolBar->setFloatable(true);
    m_toolBar->setMovable(true);
    m_toolBar->setOrientation(Qt::Horizontal);

    auto horizontalLayout = new QHBoxLayout(this);

    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);

    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);

    horizontalLayout->addWidget(m_toolBar);
}

void DesignerActionToolBar::registerAction(ActionInterface *action)
{
    NanotraceHR::Tracer tracer{"designer action tool bar register action", category()};

    m_toolBar->addAction(action->action());
}

void DesignerActionToolBar::addSeparator()
{
    NanotraceHR::Tracer tracer{"designer action tool bar add separator", category()};

    auto separatorAction = new QAction(m_toolBar);
    separatorAction->setSeparator(true);
    m_toolBar->addAction(separatorAction);
}

} //QmlDesigner
