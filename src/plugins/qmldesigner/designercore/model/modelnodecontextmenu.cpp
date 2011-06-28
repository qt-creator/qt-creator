#include "modelnodecontextmenu.h"
#include <QApplication>
#include <coreplugin/editormanager/editormanager.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <modelnode.h>
#include <qmlitemnode.h>
#include <variantproperty.h>
#include <nodeproperty.h>
#include <rewritertransaction.h>
#include <designdocumentcontroller.h>


namespace QmlDesigner {

static inline QString captionForModelNode(const ModelNode &modelNode)
{
    if (modelNode.id().isEmpty())
        return modelNode.simplifiedTypeName();

    return modelNode.id();
}

static inline bool contains(const QmlItemNode &node, const QPoint &p)
{
    return node.instanceSceneTransform().mapRect(node.instanceBoundingRect()).contains(p);
}

static inline bool checkIfNodeIsAView(const ModelNode &node)
{
    return node.metaInfo().isValid() &&
            (node.metaInfo().isSubclassOf("QtQuick.ListView", -1, -1) ||
             node.metaInfo().isSubclassOf("QtQuick.GridView", -1, -1) ||
             node.metaInfo().isSubclassOf("QtQuick.PathView", -1, -1));
}

static inline QList<QmlItemNode> siblingsForNode(const QmlItemNode &itemNode)
{
    QList<QmlItemNode> siblingList;

    if (itemNode.isValid() && itemNode.modelNode().parentProperty().isValid()) {
        QList<ModelNode> modelNodes = itemNode.modelNode().parentProperty().parentModelNode().allDirectSubModelNodes();
        foreach (const ModelNode &node, modelNodes) {
            QmlItemNode childItemNode = node;
            if (childItemNode.isValid())
                siblingList.append(childItemNode);
        }
    }

    return siblingList;
}

static signed int getMaxZValue(const QList<QmlItemNode> &siblingList)
{
    signed int maximum = INT_MIN;
    foreach (const QmlItemNode &node, siblingList) {
        signed int z  = node.instanceValue("z").toInt();
        if (z > maximum)
            maximum = z;
    }
    return maximum;
}

static signed int getMinZValue(const QList<QmlItemNode> &siblingList)
{
    signed int minimum = INT_MAX;
    foreach (const QmlItemNode &node, siblingList) {
        signed int z  = node.instanceValue("z").toInt();
        if (z < minimum)
            minimum = z;
    }
    return minimum;
}


static inline bool modelNodeIsComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return false;

    if (node.metaInfo().isComponent())
        return true;

    if (node.nodeSourceType() == ModelNode::NodeWithComponentSource)
        return true;
    if (checkIfNodeIsAView(node) &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().metaInfo().isComponent())
            return true;
        if (node.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource)
            return true;
    }

    return false;
}

static inline bool isFileComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return false;

    if (node.metaInfo().isComponent())
        return true;

    if (checkIfNodeIsAView(node) &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().metaInfo().isComponent())
            return true;
    }

    return false;
}

static inline void openFileForComponent(const ModelNode &node)
{
    if (node.metaInfo().isComponent()) {
        Core::EditorManager::instance()->openEditor(node.metaInfo().componentFileName());
    } else if (checkIfNodeIsAView(node) &&
               node.hasNodeProperty("delegate") &&
               node.nodeProperty("delegate").modelNode().metaInfo().isComponent()) {
        Core::EditorManager::instance()->openEditor(node.nodeProperty("delegate").modelNode().metaInfo().componentFileName());
    }
}

static inline void openInlineComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return;

    if (!DesignDocumentController::instance())
        return;

    if (node.nodeSourceType() == ModelNode::NodeWithComponentSource)
        DesignDocumentController::instance()->changeCurrentModelTo(node);
    if (checkIfNodeIsAView(node) &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource)
            DesignDocumentController::instance()->changeCurrentModelTo(node.nodeProperty("delegate").modelNode());
    }
}

ModelNodeContextMenu::ModelNodeContextMenu(QmlModelView *view) : m_view(view)
{
}

void ModelNodeContextMenu::execute(const QPoint &pos, bool selectionMenuBool)
{
    QMenu* menu = new QMenu();

    bool singleSelected = false;
    bool selectionIsEmpty = m_view->selectedModelNodes().isEmpty();
    ModelNode currentSingleNode;
    if (m_view->selectedModelNodes().count()== 1) {
        singleSelected = true;
        currentSingleNode = m_view->selectedModelNodes().first();
    }

    if (selectionMenuBool) {
        QMenu *selectionMenu = new QMenu(QApplication::translate("ModelNodeContextMenu", "Selecion"), menu);
        menu->addMenu(selectionMenu);
        ModelNode parentNode;
        if (singleSelected) {
            //ModelNodeAction *selectionAction;
            //selectionAction = createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "DeSelect: ") + captionForModelNode(currentSingleNode), selectionMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::DeSelectModelNode);
            //selectionMenu->addAction(selectionAction);
            if (!currentSingleNode.isRootNode()) {
                parentNode = currentSingleNode.parentProperty().parentModelNode();
                selectionMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "Select parent: ") + captionForModelNode(parentNode),
                                                               selectionMenu, QList<ModelNode>() << parentNode, ModelNodeAction::SelectModelNode));
            }

            selectionMenu->addSeparator();
        }
        foreach (const ModelNode &node, m_view->allModelNodes()) {
            if (node != currentSingleNode && node != parentNode && contains(node, m_scenePos) && !node.isRootNode())
                selectionMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "Select: ") + captionForModelNode(node), selectionMenu, QList<ModelNode>() << node, ModelNodeAction::SelectModelNode));
        }
    }

    QMenu *stackMenu = new QMenu(QApplication::translate("ModelNodeContextMenu", "Stack (z)"), menu);
    menu->addMenu(stackMenu);

    stackMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "To Front"), stackMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::ToFront, singleSelected));
    stackMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "To Back"), stackMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::ToBack, singleSelected));
    stackMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "Raise"), stackMenu, QList<ModelNode>() << m_view->selectedModelNodes(), ModelNodeAction::Raise));
    stackMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "Lower"), stackMenu, QList<ModelNode>() << m_view->selectedModelNodes(), ModelNodeAction::Lower));
    stackMenu->addSeparator();
    stackMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "Reset z property"), stackMenu, QList<ModelNode>() << m_view->selectedModelNodes(), ModelNodeAction::ResetZ));

    QMenu *editMenu = new QMenu(QApplication::translate("ModelNodeContextMenu", "Edit"), menu);
    menu->addMenu(editMenu);
    if (!selectionIsEmpty) {
        //editMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "Change Id"), editMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::SetId, singleSelected));
        editMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "Reset Position"), editMenu, m_view->selectedModelNodes(), ModelNodeAction::ResetPosition));
        editMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "Reset Size"), editMenu, m_view->selectedModelNodes(), ModelNodeAction::ResetSize));
        editMenu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "Visibility"), editMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::ModelNodeVisibility, singleSelected));

    } else {
        editMenu->setEnabled(false);
    }

    menu->addSeparator();
    bool enterComponent = false;
    if (singleSelected) {
        enterComponent = modelNodeIsComponent(currentSingleNode);
    }
    menu->addAction(createModelNodeAction(QApplication::translate("ModelNodeContextMenu", "Go into Component"), editMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::EnterComponent, enterComponent));

    menu->exec(pos);
    menu->deleteLater();
}

void ModelNodeContextMenu::setScenePos(const QPoint &pos)
{
    m_scenePos = pos;
}


ModelNodeAction* ModelNodeContextMenu::createModelNodeAction(const QString &description, QMenu *menu, const QList<ModelNode> &modelNodeList, ModelNodeAction::ModelNodeActionType type, bool enabled)
{
    ModelNodeAction* action = new ModelNodeAction(description, menu, m_view, modelNodeList, type);
    action->setEnabled(enabled);
    return action;
}


ModelNodeAction::ModelNodeAction( const QString & text, QObject *parent, QmlModelView *view,  const QList<ModelNode> &modelNodeList, ModelNodeActionType type) :
    QAction(text, parent), m_view(view), m_modelNodeList(modelNodeList), m_type(type)
{
    if (type == ModelNodeVisibility) {
        setCheckable(true);
        QmlItemNode itemNode = QmlItemNode(m_modelNodeList.first());
        if (itemNode.isValid())
            setChecked(itemNode.instanceValue("visible").toBool());
        else
            setEnabled(false);
    }
    connect(this, SIGNAL(triggered(bool)), this, SLOT(actionTriggered(bool)));
}

void ModelNodeAction::actionTriggered(bool b)
{
    switch (m_type) {
    case ModelNodeAction::SelectModelNode: select(); break;
    case ModelNodeAction::DeSelectModelNode: deSelect(); break;
    case ModelNodeAction::CutSelection: cut(); break;
    case ModelNodeAction::CopySelection: copy(); break;
    case ModelNodeAction::DeleteSelection: deleteSelection(); break;
    case ModelNodeAction::ToFront: toFront(); break;
    case ModelNodeAction::ToBack: toBack(); break;
    case ModelNodeAction::Raise: raise(); break;
    case ModelNodeAction::Lower: lower(); break;
    case ModelNodeAction::Paste: paste(); break;
    case ModelNodeAction::Undo: undo(); break;
    case ModelNodeAction::Redo: redo(); break;
    case ModelNodeAction::ModelNodeVisibility: setVisible(b); break;
    case ModelNodeAction::ResetSize: resetSize(); break;
    case ModelNodeAction::ResetPosition: resetPosition(); break;
    case ModelNodeAction::EnterComponent: enterComponent(); break;
    case ModelNodeAction::SetId: setId(); break;
    case ModelNodeAction::ResetZ: resetZ(); break;
    }
}

void ModelNodeAction::select()
{
    if (m_view)
        m_view->setSelectedModelNodes(m_modelNodeList);
}

void ModelNodeAction::deSelect()
{
    if (m_view) {
        QList<ModelNode> selectedNodes = m_view->selectedModelNodes();
        foreach (const ModelNode &node, m_modelNodeList) {
            if (selectedNodes.contains(node))
                selectedNodes.removeAll(node);
        }
        m_view->setSelectedModelNodes(selectedNodes);
    }
}

void ModelNodeAction::cut()
{
}


void ModelNodeAction::copy()
{
}

void ModelNodeAction::deleteSelection()
{
}

void ModelNodeAction::toFront()
{
    if (!m_view)
        return;

    QmlItemNode node = m_modelNodeList.first();
    if (node.isValid()) {
        signed int maximumZ = getMaxZValue(siblingsForNode(node));
        maximumZ++;
        node.setVariantProperty("z", maximumZ);
    }
}


void ModelNodeAction::toBack()
{
    if (!m_view)
        return;

    QmlItemNode node = m_modelNodeList.first();
    if (node.isValid()) {
        signed int minimumZ = getMinZValue(siblingsForNode(node));
        minimumZ--;
        node.setVariantProperty("z", minimumZ);
    }
}

void ModelNodeAction::raise()
{
    if (!m_view)
        return;

    RewriterTransaction(m_view);
    foreach (ModelNode modelNode, m_modelNodeList) {
        QmlItemNode node = modelNode;
        if (node.isValid()) {
            signed int z  = node.instanceValue("z").toInt();
            z++;
            node.setVariantProperty("z", z);
        }
    }
}

void ModelNodeAction::lower()
{
    if (!m_view)
        return;

    RewriterTransaction(m_view);
    foreach (ModelNode modelNode, m_modelNodeList) {
        QmlItemNode node = modelNode;
        if (node.isValid()) {
            signed int z  = node.instanceValue("z").toInt();
            z--;
            node.setVariantProperty("z", z);
        }
    }
}

void ModelNodeAction::paste()
{
}

void ModelNodeAction::undo()
{
}

void ModelNodeAction::redo()
{
}

void ModelNodeAction::setVisible(bool b)
{
    m_modelNodeList.first().variantProperty("visible") = b;
}


void ModelNodeAction::resetSize()
{
    if (!m_view)
        return;

    RewriterTransaction(m_view);
    foreach (ModelNode node, m_modelNodeList) {
        node.removeProperty("width");
        node.removeProperty("height");
    }
}

void ModelNodeAction::resetPosition()
{
    if (!m_view)
        return;

    RewriterTransaction(m_view);
    foreach (ModelNode node, m_modelNodeList) {
        node.removeProperty("x");
        node.removeProperty("y");
    }
}

void ModelNodeAction::enterComponent()
{
    const ModelNode node = m_modelNodeList.first();
    if (node.isValid()) {
    if (isFileComponent(node))
        openFileForComponent(node);
    else
        openInlineComponent(node);
    }
}

void ModelNodeAction::setId()
{
}

void ModelNodeAction::resetZ()
{
    if (!m_view)
        return;

    RewriterTransaction(m_view);
    foreach (ModelNode node, m_modelNodeList) {
        node.removeProperty("z");
    }
}

}
