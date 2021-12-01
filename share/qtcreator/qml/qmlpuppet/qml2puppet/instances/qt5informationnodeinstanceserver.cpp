/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qt5informationnodeinstanceserver.h"

#include <QQuickItem>
#include <QQuickView>
#include <QDropEvent>
#include <QMimeData>

#include "servernodeinstance.h"
#include "childrenchangeeventfilter.h"
#include "propertyabstractcontainer.h"
#include "propertybindingcontainer.h"
#include "propertyvaluecontainer.h"
#include "instancecontainer.h"
#include "createinstancescommand.h"
#include "changefileurlcommand.h"
#include "clearscenecommand.h"
#include "reparentinstancescommand.h"
#include "update3dviewstatecommand.h"
#include "changevaluescommand.h"
#include "changebindingscommand.h"
#include "changeidscommand.h"
#include "removeinstancescommand.h"
#include "nodeinstanceclientinterface.h"
#include "removepropertiescommand.h"
#include "valueschangedcommand.h"
#include "informationchangedcommand.h"
#include "pixmapchangedcommand.h"
#include "commondefines.h"
#include "changestatecommand.h"
#include "childrenchangedcommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "createscenecommand.h"
#include "tokencommand.h"
#include "removesharedmemorycommand.h"
#include "objectnodeinstance.h"
#include "puppettocreatorcommand.h"
#include "inputeventcommand.h"
#include "view3dactioncommand.h"
#include "requestmodelnodepreviewimagecommand.h"
#include "changeauxiliarycommand.h"

#include "dummycontextobject.h"
#include "../editor3d/generalhelper.h"
#include "../editor3d/mousearea3d.h"
#include "../editor3d/camerageometry.h"
#include "../editor3d/lightgeometry.h"
#include "../editor3d/gridgeometry.h"
#include "../editor3d/selectionboxgeometry.h"
#include "../editor3d/linegeometry.h"
#include "../editor3d/icongizmoimageprovider.h"

#include <designersupportdelegate.h>
#include <qmlprivategate.h>
#include <quickitemnodeinstance.h>

#include <QVector3D>
#include <QQmlProperty>
#include <QOpenGLContext>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtGui/qevent.h>
#include <QtGui/qguiapplication.h>
#include <QProcessEnvironment>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtQuick/private/qquickrendercontrol_p.h>
#endif

#ifdef QUICK3D_MODULE
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dcamera_p.h>
#include <QtQuick3D/private/qquick3dabstractlight_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dscenerootnode_p.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "../editor3d/qt5compat/qquick3darealight_p.h"
#endif
#endif

#ifdef QUICK3D_PARTICLES_MODULE
#include <QtQuick3DParticles/private/qquick3dparticle_p.h>
#include <QtQuick3DParticles/private/qquick3dparticleaffector_p.h>
#include <QtQuick3DParticles/private/qquick3dparticleemitter_p.h>
#endif

#ifdef IMPORT_QUICK3D_ASSETS
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>
#endif

// Uncomment to display FPS counter on the lower left corner of edit 3D view
//#define FPS_COUNTER
#ifdef FPS_COUNTER
#include <QtCore/qelapsedtimer.h>
static QElapsedTimer *_fpsTimer = nullptr;
static int _frameCount = 0;
#endif

namespace QmlDesigner {

static QVariant objectToVariant(QObject *object)
{
    return QVariant::fromValue(object);
}

static QImage nonVisualComponentPreviewImage()
{
    static double ratio = qgetenv("FORMEDITOR_DEVICE_PIXEL_RATIO").toDouble();
    if (ratio == 1.) {
        static const QImage image(":/qtquickplugin/images/non-visual-component.png");
        return image;
    } else {
        static const QImage image(":/qtquickplugin/images/non-visual-component@2x.png");
        return image;
    }
}

static bool imageHasContent(const QImage &image)
{
    // Check if any image pixel contains non-zero data
    const uchar *pData = image.constBits();
    const qsizetype size = image.sizeInBytes();
    for (qsizetype i = 0; i < size; ++i) {
        if (*(pData++) != 0)
            return true;
    }
    return false;
}

static QObjectList toObjectList(const QVariant &variantList)
{
    QObjectList objList;
    if (!variantList.isNull()) {
        const auto varList = variantList.value<QVariantList>();
        for (const auto &var : varList) {
            QObject *obj = var.value<QObject *>();
            if (obj)
                objList.append(obj);
        }
    }
    return objList;
}

static QList<PropertyName> toPropertyNameList(const QVariant &variantList)
{
    QList<PropertyName> propList;
    if (!variantList.isNull()) {
        const auto varList = variantList.value<QVariantList>();
        for (const auto &var : varList) {
            PropertyName prop = var.toByteArray();
            if (!prop.isEmpty())
                propList.append(prop);
        }
    }
    return propList;
}

void Qt5InformationNodeInstanceServer::createAuxiliaryQuickView(const QUrl &url,
                                                                RenderViewData &viewData)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    viewData.window = new QQuickView(quickView()->engine(), quickView());
    viewData.window->setFormat(quickView()->format());
    DesignerSupport::createOpenGLContext(static_cast<QQuickView *>(viewData.window.data()));
#else
    viewData.renderControl = new QQuickRenderControl;
    viewData.window = new QQuickWindow(viewData.renderControl);
    viewData.renderControl->initialize();
#endif
    QQmlComponent component(engine());
    component.loadUrl(url);
    viewData.rootItem = qobject_cast<QQuickItem *>(component.create());

    if (!viewData.rootItem) {
        qWarning() << "Could not create view for: " << url.toString() << component.errors();
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    DesignerSupport::setRootItem(static_cast<QQuickView *>(viewData.window.data()), viewData.rootItem);
#else
    viewData.window->contentItem()->setSize(viewData.rootItem->size());
    viewData.window->setGeometry(0, 0, viewData.rootItem->width(), viewData.rootItem->height());
    viewData.rootItem->setParentItem(viewData.window->contentItem());
#endif
}

void Qt5InformationNodeInstanceServer::updateLockedAndHiddenStates(const QSet<ServerNodeInstance> &instances)
{
    if (!ViewConfig::isQuick3DMode())
        return;

    // We only want to update the topmost parents in the set
    for (const auto &instance : instances) {
        if (instance.isValid()) {
            const auto parentInst = instance.parent();
            if (!parentInst.isValid() || !instances.contains(parentInst)) {
                handleInstanceHidden(instance, instance.internalInstance()->isHiddenInEditor(), true);
                handleInstanceLocked(instance, instance.internalInstance()->isLockedInEditor(), true);
            }
        }
    }
}

void Qt5InformationNodeInstanceServer::handleInputEvents()
{
    if (m_editView3DData.window) {
        int angleDelta = 0;
        for (int i = 0; i < m_pendingInputEventCommands.size(); ++i) {
            const InputEventCommand &command = m_pendingInputEventCommands[i];
            if (command.type() == QEvent::Wheel) {
                if (i < m_pendingInputEventCommands.size() - 1) {
                    // Peek at next command. If that is also a wheel with same button/modifiers
                    // state, skip this event and add the angle delta to the next one.
                    auto nextCommand = m_pendingInputEventCommands[i + 1];
                    if (nextCommand.type() == QEvent::MouseMove
                            && nextCommand.button() == command.button()
                            && nextCommand.buttons() == command.buttons()
                            && nextCommand.modifiers() == command.modifiers()) {
                        angleDelta += command.angleDelta();
                        continue;
                    }
                }
                QWheelEvent *we
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
                        = new QWheelEvent(command.pos(), command.pos(), {0, 0},
                                          {0, angleDelta + command.angleDelta()},
                                          command.buttons(), command.modifiers(), Qt::NoScrollPhase,
                                          false);
#else
                        = new QWheelEvent(command.pos(), command.pos(), {0, 0}, {0, command.angleDelta()},
                                          0, Qt::Horizontal, command.buttons(), command.modifiers(),
                                          Qt::NoScrollPhase, Qt::MouseEventNotSynthesized);
#endif
                angleDelta = 0;
                QGuiApplication::sendEvent(m_editView3DData.window, we);
            } else {
                if (command.type() == QEvent::MouseMove && i < m_pendingInputEventCommands.size() - 1) {
                    // Peek at next command. If that is also a move with only difference being
                    // the position, skip this event as it is pointless
                    auto nextCommand = m_pendingInputEventCommands[i + 1];
                    if (nextCommand.type() == QEvent::MouseMove
                            && nextCommand.button() == command.button()
                            && nextCommand.buttons() == command.buttons()
                            && nextCommand.modifiers() == command.modifiers()) {
                        continue;
                    }
                }
                auto me = new QMouseEvent(command.type(), command.pos(), command.button(),
                                          command.buttons(), command.modifiers());
                // We must use sendEvent in Qt 6, as using postEvent allows the associated position
                // data stored internally in QMutableEventPoint to potentially be updated by system
                // before the event is delivered.
                QGuiApplication::sendEvent(m_editView3DData.window, me);
            }
        }

        m_pendingInputEventCommands.clear();

        render3DEditView();
    }
}

void Qt5InformationNodeInstanceServer::resolveImportSupport()
{
#ifdef IMPORT_QUICK3D_ASSETS
    QSSGAssetImportManager importManager;
    const QHash<QString, QStringList> supportedExtensions = importManager.getSupportedExtensions();
    const QHash<QString, QVariantMap> supportedOptions = importManager.getAllOptions();

    QVariantMap supportMap;

    QVariantMap extMap;
    auto itExt = supportedExtensions.constBegin();
    while (itExt != supportedExtensions.constEnd()) {
        extMap.insert(itExt.key(), itExt.value());
        ++itExt;
    }

    QVariantMap optMap;
    auto itOpt = supportedOptions.constBegin();
    while (itOpt != supportedOptions.constEnd()) {
        optMap.insert(itOpt.key(), itOpt.value());
        ++itOpt;
    }

    supportMap.insert("options", optMap);
    supportMap.insert("extensions", extMap);
    nodeInstanceClient()->handlePuppetToCreatorCommand(
                {PuppetToCreatorCommand::Import3DSupport, QVariant(supportMap)});

#endif
}

void Qt5InformationNodeInstanceServer::updateRotationBlocks(const QVector<PropertyValueContainer> &valueChanges)
{
#ifdef QUICK3D_MODULE
    auto helper = qobject_cast<QmlDesigner::Internal::GeneralHelper *>(m_3dHelper);
    if (helper) {
        QSet<QQuick3DNode *> blockedNodes;
        QSet<QQuick3DNode *> unblockedNodes;
        const PropertyName propName = "rotBlocked@Internal";
        for (const auto &container : valueChanges) {
            if (container.name() == propName) {
                ServerNodeInstance instance = instanceForId(container.instanceId());
                if (instance.isValid()) {
                    auto node = qobject_cast<QQuick3DNode *>(instance.internalObject());
                    if (node) {
                        if (container.value().toBool())
                            blockedNodes.insert(node);
                        else
                            unblockedNodes.insert(node);
                    }
                }
            }
        }
        helper->addRotationBlocks(blockedNodes);
        helper->removeRotationBlocks(unblockedNodes);
    }
#else
    Q_UNUSED(valueChanges)
#endif
}

void Qt5InformationNodeInstanceServer::removeRotationBlocks(const QVector<qint32> &instanceIds)
{
#ifdef QUICK3D_MODULE
    auto helper = qobject_cast<QmlDesigner::Internal::GeneralHelper *>(m_3dHelper);
    if (helper) {
        QSet<QQuick3DNode *> unblockedNodes;
        for (const auto &id : instanceIds) {
            ServerNodeInstance instance = instanceForId(id);
            if (instance.isValid()) {
                auto node = qobject_cast<QQuick3DNode *>(instance.internalObject());
                if (node)
                    unblockedNodes.insert(node);
            }
        }
        helper->removeRotationBlocks(unblockedNodes);
    }
#else
    Q_UNUSED(instanceIds)
#endif
}

void Qt5InformationNodeInstanceServer::createEditView3D()
{
#ifdef QUICK3D_MODULE
    qmlRegisterRevision<QQuick3DNode, 1>("MouseArea3D", 1, 0);
    qmlRegisterType<QmlDesigner::Internal::MouseArea3D>("MouseArea3D", 1, 0, "MouseArea3D");
    qmlRegisterUncreatableType<QmlDesigner::Internal::GeometryBase>("GeometryBase", 1, 0, "GeometryBase",
                                                                    "Abstract Base Class");
    qmlRegisterType<QmlDesigner::Internal::CameraGeometry>("CameraGeometry", 1, 0, "CameraGeometry");
    qmlRegisterType<QmlDesigner::Internal::LightGeometry>("LightUtils", 1, 0, "LightGeometry");
    qmlRegisterType<QmlDesigner::Internal::GridGeometry>("GridGeometry", 1, 0, "GridGeometry");
    qmlRegisterType<QmlDesigner::Internal::SelectionBoxGeometry>("SelectionBoxGeometry", 1, 0, "SelectionBoxGeometry");
    qmlRegisterType<QmlDesigner::Internal::LineGeometry>("LineGeometry", 1, 0, "LineGeometry");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qmlRegisterType<QmlDesigner::Internal::QQuick3DAreaLight>("LightUtils", 1, 0, "AreaLight");
#endif

    auto helper = new QmlDesigner::Internal::GeneralHelper();
    QObject::connect(helper, &QmlDesigner::Internal::GeneralHelper::toolStateChanged,
                     this, &Qt5InformationNodeInstanceServer::handleToolStateChanged);
    engine()->rootContext()->setContextProperty("_generalHelper", helper);
    engine()->addImageProvider(QLatin1String("IconGizmoImageProvider"),
                               new QmlDesigner::Internal::IconGizmoImageProvider);
    m_3dHelper = helper;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    createAuxiliaryQuickView(QUrl("qrc:/qtquickplugin/mockfiles/qt6/EditView3D.qml"), m_editView3DData);
#else
    createAuxiliaryQuickView(QUrl("qrc:/qtquickplugin/mockfiles/qt5/EditView3D.qml"), m_editView3DData);
#endif
    if (m_editView3DData.rootItem)
        helper->setParent(m_editView3DData.rootItem);
#endif
}

#ifdef QUICK3D_PARTICLES_MODULE
void Qt5InformationNodeInstanceServer::resetParticleSystem()
{
    if (!m_targetParticleSystem)
        return;
    m_targetParticleSystem->reset();
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 2)
    m_targetParticleSystem->setEditorTime(0);
#endif
    if (m_particleAnimationDriver)
        m_particleAnimationDriver->reset();
}

void Qt5InformationNodeInstanceServer::handleParticleSystemSelected(QQuick3DParticleSystem* targetParticleSystem)
{
    if (!m_particleAnimationDriver || targetParticleSystem == m_targetParticleSystem)
        return;

    m_particleAnimationDriver->reset();
    // stop the previously selected from animating
    resetParticleSystem();

    m_targetParticleSystem = targetParticleSystem;

    resetParticleSystem();
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 2)
    QObject::disconnect(m_particleAnimationConnection);
    m_particleAnimationConnection = connect(m_particleAnimationDriver, &AnimationDriver::advanced, [this] () {
        if (m_targetParticleSystem)
            m_targetParticleSystem->setEditorTime(m_particleAnimationDriver->elapsed());
    });
#endif
    if (m_particleAnimationPlaying && m_targetParticleSystem->visible())
        m_particleAnimationDriver->restart();
    QObject::connect(m_targetParticleSystem, &QQuick3DNode::visibleChanged, [this] () {
        if (m_particleAnimationPlaying && m_targetParticleSystem->visible()) {
            m_particleAnimationDriver->restart();
        } else {
            m_particleAnimationDriver->reset();
            resetParticleSystem();
        }
    });

    const auto anim = animations();
    for (auto a : anim)
        a->restart();
}

static QString baseProperty(const QString &property)
{
    int index = property.indexOf('.');
    if (index > 0)
        return property.left(index);
    return property;
}

template <typename T>
static QQuick3DParticleSystem *systemProperty(QObject *object)
{
    return qobject_cast<T>(object) ? qobject_cast<T>(object)->system() : nullptr;
}

static QQuick3DParticleSystem *getSystemOrSystemProperty(QObject *selectedObject)
{
    QQuick3DParticleSystem *system = nullptr;
    system = qobject_cast<QQuick3DParticleSystem *>(selectedObject);
    if (system)
        return system;
    system = systemProperty<QQuick3DParticle *>(selectedObject);
    if (system)
        return system;
    system = systemProperty<QQuick3DParticleAffector *>(selectedObject);
    if (system)
        return system;
    system = systemProperty<QQuick3DParticleEmitter *>(selectedObject);
    if (system)
        return system;
    return nullptr;
}

static QQuick3DParticleSystem *parentParticleSystem(QObject *selectedObject)
{
    auto *ps = getSystemOrSystemProperty(selectedObject);
    if (ps)
        return ps;
    QObject *parent = selectedObject->parent();
    while (parent) {
        ps = getSystemOrSystemProperty(parent);
        if (ps)
            return ps;
        parent = parent->parent();
    }
    return nullptr;
}

void Qt5InformationNodeInstanceServer::handleParticleSystemDeselected(QObject *selectedObject)
{
    m_targetParticleSystem = nullptr;
    const auto anim = animations();
    int i = 0;
    for (auto a : anim) {
        a->stop();
        QQuickPropertyAnimation *panim = qobject_cast<QQuickPropertyAnimation *>(a);
        if (panim)
            panim->target()->setProperty(qPrintable(baseProperty(panim->property())), animationDefaultValue(i));
        i++;
    }
}
#endif

// The selection has changed in the edit view 3D. Empty list indicates selection is cleared.
void Qt5InformationNodeInstanceServer::handleSelectionChanged(const QVariant &objs)
{
#ifdef QUICK3D_PARTICLES_MODULE
    resetParticleSystem();
#endif
    QList<ServerNodeInstance> instanceList;
    const QVariantList varObjs = objs.value<QVariantList>();
    for (const auto &object : varObjs) {
        auto obj = object.value<QObject *>();
        if (obj) {
            ServerNodeInstance instance = instanceForObject(obj);
            instanceList << instance;
        }
    }
    selectInstances(instanceList);
    // Hold selection changes reflected back from designer for a bit
    m_selectionChangeTimer.start(500);
}

QVector<Qt5InformationNodeInstanceServer::InstancePropertyValueTriple>
Qt5InformationNodeInstanceServer::propertyToPropertyValueTriples(
    const ServerNodeInstance &instance,
    const PropertyName &propertyName,
    const QVariant &variant)
{
    QVector<InstancePropertyValueTriple> result;
    InstancePropertyValueTriple propTriple;

    if (variant.type() == QVariant::Vector3D) {
        auto vector3d = variant.value<QVector3D>();

        if (vector3d.isNull())
            return result;

        const PropertyName dot = propertyName.isEmpty() ? "" : ".";
        propTriple.instance = instance;
        propTriple.propertyName = propertyName + dot + PropertyName("x");
        propTriple.propertyValue = vector3d.x();
        result.append(propTriple);
        propTriple.propertyName = propertyName + dot + PropertyName("y");
        propTriple.propertyValue = vector3d.y();
        result.append(propTriple);
        propTriple.propertyName = propertyName + dot + PropertyName("z");
        propTriple.propertyValue = vector3d.z();
        result.append(propTriple);
    } else {
        propTriple.instance = instance;
        propTriple.propertyName = propertyName;
        propTriple.propertyValue = variant;
        result.append(propTriple);
    }

    return result;
}

void Qt5InformationNodeInstanceServer::modifyVariantValue(const QObjectList &objects,
    const QList<PropertyName> &propNames, ValuesModifiedCommand::TransactionOption option)
{
    struct PropNamePair {
        PropertyName origPropName;
        PropertyName targetPropName;
    };

    QList<PropNamePair> propNamePairs;

    // Position is a special case, because the position can be 'position.x 'or simply 'x'.
    // We prefer 'x'.
    for (const auto &propName : propNames) {
        if (propName != "position")
            propNamePairs.append({propName, propName});
        else
            propNamePairs.append({propName, PropertyName{}});
    }

    if (!objects.isEmpty()) {
        QVector<PropertyValueContainer> valueVector;
        for (const auto listObj : objects) {
            ServerNodeInstance instance = instanceForObject(listObj);
            if (instance.isValid()) {
                const qint32 instId = instance.instanceId();
                if (option == ValuesModifiedCommand::TransactionOption::Start)
                    instance.setModifiedFlag(true);
                else if (option == ValuesModifiedCommand::TransactionOption::End)
                    instance.setModifiedFlag(false);
                for (const auto &propNamePair : propNamePairs) {
                    // We do have to split vector3d props into foobar.x, foobar.y, foobar.z
                    const QVector<InstancePropertyValueTriple> triple
                            = propertyToPropertyValueTriples(instance, propNamePair.targetPropName,
                                                             listObj->property(propNamePair.origPropName));
                    for (const auto &property : triple) {
                        const PropertyName propName = property.propertyName;
                        const QVariant propValue = property.propertyValue;
                        valueVector.append(PropertyValueContainer(instId, propName,
                                                                  propValue, PropertyName()));
                    }
                }
            }
        }

        if (!valueVector.isEmpty()) {
            ValuesModifiedCommand command(valueVector);
            command.transactionOption = option;
            nodeInstanceClient()->valuesModified(command);
        }
    }
}

void Qt5InformationNodeInstanceServer::handleObjectPropertyCommit(const QVariant &objects,
                                                                  const QVariant &propNames)
{
    modifyVariantValue(toObjectList(objects), toPropertyNameList(propNames),
                       ValuesModifiedCommand::TransactionOption::End);
    m_changedNodes.clear();
    m_changedProperties.clear();
    m_propertyChangeTimer.stop();
}

void Qt5InformationNodeInstanceServer::handleObjectPropertyChange(const QVariant &objects,
                                                                  const QVariant &propNames)
{
    QObjectList objList = toObjectList(objects);
    QList<PropertyName> propList = toPropertyNameList(propNames);

    bool nodeChanged = true;
    if (objList.size() == m_changedNodes.size()) {
        nodeChanged = false;
        for (int i = 0; i < objList.size(); ++i) {
            if (objList[i] != m_changedNodes[i]) {
                nodeChanged = true;
                break;
            }
        }
    }
    if (!nodeChanged && propList.size() == m_changedProperties.size()) {
        for (int i = 0; i < propList.size(); ++i) {
            if (propList[i] != m_changedProperties[i]) {
                nodeChanged = true;
                break;
            }
        }
    }

    if (nodeChanged) {
        if (!m_changedNodes.isEmpty()) {
            // Nodes/properties changed, commit currently pending changes
            modifyVariantValue(m_changedNodes, m_changedProperties,
                               ValuesModifiedCommand::TransactionOption::End);
            m_changedNodes.clear();
            m_changedProperties.clear();
            m_propertyChangeTimer.stop();
        }
        modifyVariantValue(objList, propList, ValuesModifiedCommand::TransactionOption::Start);
    } else if (!m_propertyChangeTimer.isActive()) {
        m_propertyChangeTimer.start();
    }
    m_changedNodes = objList;
    m_changedProperties = propList;
}

void Qt5InformationNodeInstanceServer::handleActiveSceneChange()
{
#ifdef QUICK3D_MODULE
    ServerNodeInstance sceneInstance = active3DSceneInstance();
    const QString sceneId = sceneInstance.id();

    QVariantMap toolStates;
    auto helper = qobject_cast<QmlDesigner::Internal::GeneralHelper *>(m_3dHelper);
    if (helper)
        toolStates = helper->getToolStates(sceneId);
    toolStates.insert("sceneInstanceId", QVariant::fromValue(sceneInstance.instanceId()));

    nodeInstanceClient()->handlePuppetToCreatorCommand({PuppetToCreatorCommand::ActiveSceneChanged,
                                                        toolStates});
    m_selectionChangeTimer.start(0);
#endif
}

void Qt5InformationNodeInstanceServer::handleToolStateChanged(const QString &sceneId,
                                                              const QString &tool,
                                                              const QVariant &toolState)
{
    QVariantList data;
    data << sceneId;
    data << tool;
    data << toolState;
    nodeInstanceClient()->handlePuppetToCreatorCommand({PuppetToCreatorCommand::Edit3DToolState,
                                                        QVariant::fromValue(data)});
}

void Qt5InformationNodeInstanceServer::handleView3DSizeChange()
{
    QObject *view3D = sender();
    if (view3D == m_active3DView)
        updateView3DRect(view3D);
}

void Qt5InformationNodeInstanceServer::handleView3DDestroyed(QObject *obj)
{
#ifdef QUICK3D_MODULE
    auto view = qobject_cast<QQuick3DViewport *>(obj);
    m_view3Ds.remove(obj);
    removeNode3D(view->scene());
    if (view && view == m_active3DView)
        m_active3DView = nullptr;
#else
    Q_UNUSED(obj)
#endif
}

void Qt5InformationNodeInstanceServer::handleNode3DDestroyed(QObject *obj)
{
#ifdef QUICK3D_MODULE
    if (qobject_cast<QQuick3DCamera *>(obj)) {
        QMetaObject::invokeMethod(m_editView3DData.rootItem, "releaseCameraGizmo",
                                  Q_ARG(QVariant, objectToVariant(obj)));
    } else if (qobject_cast<QQuick3DAbstractLight *>(obj)) {
        QMetaObject::invokeMethod(m_editView3DData.rootItem, "releaseLightGizmo",
                                  Q_ARG(QVariant, objectToVariant(obj)));
#ifdef QUICK3D_PARTICLES_MODULE
    } else if (qobject_cast<QQuick3DParticleSystem *>(obj)) {
        QMetaObject::invokeMethod(m_editView3DData.rootItem, "releaseParticleSystemGizmo",
                                  Q_ARG(QVariant, objectToVariant(obj)));
#endif
    }
    removeNode3D(obj);
#else
    Q_UNUSED(obj)
#endif
}

void Qt5InformationNodeInstanceServer::updateView3DRect(QObject *view3D)
{
    QRectF viewPortrect(0., 0., 1000., 1000.);
    if (view3D) {
        viewPortrect = QRectF(0., 0., view3D->property("width").toDouble(),
                              view3D->property("height").toDouble());
    }
    QQmlProperty viewPortProperty(m_editView3DData.rootItem, "viewPortRect", context());
    viewPortProperty.write(viewPortrect);
}

void Qt5InformationNodeInstanceServer::updateActiveSceneToEditView3D()
{
#ifdef QUICK3D_MODULE
    if (!m_editView3DSetupDone)
        return;

    QVariant activeSceneVar = objectToVariant(m_active3DScene);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Active scene change handling on qml side is async, so a deleted importScene would crash
    // editView when it updates next. Disable/enable edit view update synchronously to avoid this.
    QMetaObject::invokeMethod(m_editView3DData.rootItem, "enableEditViewUpdate",
                              Q_ARG(QVariant, activeSceneVar));
#endif
    ServerNodeInstance sceneInstance = active3DSceneInstance();
    const QString sceneId = sceneInstance.id();

    // QML item id is updated with separate call, so delay this update until we have it
    if (m_active3DScene && sceneId.isEmpty()) {
        m_active3DSceneUpdatePending = true;
        return;
    } else {
        m_active3DSceneUpdatePending = false;
    }

    QMetaObject::invokeMethod(m_editView3DData.rootItem, "setActiveScene", Qt::QueuedConnection,
                              Q_ARG(QVariant, activeSceneVar),
                              Q_ARG(QVariant, QVariant::fromValue(sceneId)));

    updateView3DRect(m_active3DView);

    auto helper = qobject_cast<QmlDesigner::Internal::GeneralHelper *>(m_3dHelper);
    if (helper)
        helper->storeToolState(helper->globalStateId(), helper->lastSceneIdKey(), QVariant(sceneId), 0);
#endif
}

void Qt5InformationNodeInstanceServer::removeNode3D(QObject *node)
{
    m_3DSceneMap.remove(node);
    const auto oldMap = m_3DSceneMap;
    auto it = oldMap.constBegin();
    while (it != oldMap.constEnd()) {
        if (it.value() == node) {
            m_3DSceneMap.remove(it.key(), node);
            break;
        }
        ++it;
    }
    if (node == m_active3DScene) {
        m_active3DScene = nullptr;
        m_active3DView = nullptr;
        updateActiveSceneToEditView3D();
    }
}

void Qt5InformationNodeInstanceServer::resolveSceneRoots()
{
#ifdef QUICK3D_MODULE
    if (!m_editView3DSetupDone)
        return;

    const auto oldMap = m_3DSceneMap;
    m_3DSceneMap.clear();
    auto it = oldMap.begin();
    bool updateActiveScene = !m_active3DScene;
    while (it != oldMap.end()) {
        QObject *node = it.value();
        QObject *newRoot = find3DSceneRoot(node);
        QObject *oldRoot = it.key();
        if (!m_active3DScene || (newRoot != oldRoot && m_active3DScene == oldRoot)) {
            m_active3DScene = newRoot;
            updateActiveScene = true;
        }
        m_3DSceneMap.insert(newRoot, node);

        if (newRoot != oldRoot) {
            if (qobject_cast<QQuick3DCamera *>(node)) {
                QMetaObject::invokeMethod(m_editView3DData.rootItem, "updateCameraGizmoScene",
                                          Q_ARG(QVariant, objectToVariant(newRoot)),
                                          Q_ARG(QVariant, objectToVariant(node)));
            } else if (qobject_cast<QQuick3DAbstractLight *>(node)) {
                QMetaObject::invokeMethod(m_editView3DData.rootItem, "updateLightGizmoScene",
                                          Q_ARG(QVariant, objectToVariant(newRoot)),
                                          Q_ARG(QVariant, objectToVariant(node)));
#ifdef QUICK3D_PARTICLES_MODULE
            } else if (qobject_cast<QQuick3DParticleSystem *>(node)) {
                QMetaObject::invokeMethod(m_editView3DData.rootItem, "updateParticleSystemGizmoScene",
                                          Q_ARG(QVariant, objectToVariant(newRoot)),
                                          Q_ARG(QVariant, objectToVariant(node)));
#endif
            }
        }
        ++it;
    }
    if (updateActiveScene) {
        m_active3DView = findView3DForSceneRoot(m_active3DScene);
        updateActiveSceneToEditView3D();
    }
#endif
}

ServerNodeInstance Qt5InformationNodeInstanceServer::active3DSceneInstance() const
{
    ServerNodeInstance sceneInstance;
    if (hasInstanceForObject(m_active3DScene))
        sceneInstance = instanceForObject(m_active3DScene);
    else if (hasInstanceForObject(m_active3DView))
        sceneInstance = instanceForObject(m_active3DView);
    return sceneInstance;
}

void Qt5InformationNodeInstanceServer::updateNodesRecursive(QQuickItem *item)
{
    const auto childItems = item->childItems();
    for (QQuickItem *childItem : childItems)
        updateNodesRecursive(childItem);

    if (Internal::QuickItemNodeInstance::unifiedRenderPathOrQt6()) {
        if (item->flags() & QQuickItem::ItemHasContents)
            item->update();
    } else {
        DesignerSupport::updateDirtyNode(item);
    }
}

QQuickItem *Qt5InformationNodeInstanceServer::getContentItemForRendering(QQuickItem *rootItem)
{
    QQuickItem *contentItem = QQmlProperty::read(rootItem, "contentItem").value<QQuickItem *>();
    if (contentItem) {
        if (!Internal::QuickItemNodeInstance::unifiedRenderPathOrQt6())
            designerSupport()->refFromEffectItem(contentItem, false);
        QmlDesigner::Internal::QmlPrivateGate::disableNativeTextRendering(contentItem);
    }
    return contentItem;
}

void Qt5InformationNodeInstanceServer::render3DEditView(int count)
{
    m_need3DEditViewRender = qMax(count, m_need3DEditViewRender);
    if (!m_render3DEditViewTimer.isActive())
        m_render3DEditViewTimer.start(0);
}

// render the 3D edit view and send the result to creator process
void Qt5InformationNodeInstanceServer::doRender3DEditView()
{
    if (m_editView3DSetupDone) {
        if (!m_editView3DData.contentItem)
            m_editView3DData.contentItem = getContentItemForRendering(m_editView3DData.rootItem);

        QImage renderImage;

        updateNodesRecursive(m_editView3DData.contentItem);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
            renderImage = m_editView3DData.window->grabWindow();
        } else {
            // Fake render loop signaling to update things like QML items as 3D textures
            m_editView3DData.window->beforeSynchronizing();
            m_editView3DData.window->beforeRendering();

            QSizeF size = qobject_cast<QQuickItem *>(m_editView3DData.contentItem)->size();
            QRectF renderRect(QPointF(0., 0.), size);
            renderImage = designerSupport()->renderImageForItem(m_editView3DData.contentItem,
                                                                renderRect, size.toSize());

            m_editView3DData.window->afterRendering();
        }
#else
        renderImage = grabRenderControl(m_editView3DData);
#endif

        // There's no instance related to image, so instance id is -1.
        // Key number is selected so that it is unlikely to conflict other ImageContainer use.
        auto imgContainer = ImageContainer(-1, renderImage, 2100000000);

        // If we have only one or no render queued, send the result to the creator side.
        // Otherwise, we'll hold on that until we have rendered all pending frames to ensure sent
        // results are correct.
        if (m_need3DEditViewRender <= 1) {
            nodeInstanceClient()->handlePuppetToCreatorCommand({PuppetToCreatorCommand::Render3DView,
                                                                QVariant::fromValue(imgContainer)});
#ifdef QUICK3D_PARTICLES_MODULE
            if (m_need3DEditViewRender == 0 && ViewConfig::isParticleViewMode()
                    && m_particleAnimationDriver && m_particleAnimationDriver->isAnimating()) {
                m_need3DEditViewRender = 1;
            }
#endif
        }

        if (m_need3DEditViewRender > 0) {
            // We queue another render even if the requested render count was one, because another
            // render is needed to ensure gizmo geometries are properly updated.
            // Note that while in theory this seems that we shouldn't need to send the image to
            // creator side when m_need3DEditViewRender is one, we need to do it to ensure
            // smooth operation when objects are moved via drag, which triggers new renders
            // continueously.
            m_render3DEditViewTimer.start(17); // 16.67ms = ~60fps, rounds up to 17
            --m_need3DEditViewRender;
        }

#ifdef FPS_COUNTER
        // Force constant rendering for accurate fps count
        if (!m_render3DEditViewTimer.isActive())
            m_render3DEditViewTimer.start(0);
        ++_frameCount;
        if (_fpsTimer->elapsed() > 1000) {
            QQmlProperty fpsProperty(m_editView3DData.rootItem, "fps", context());
            fpsProperty.write(_frameCount);
            _frameCount = 0;
            _fpsTimer->start();
        }
#endif
    }
}

void Qt5InformationNodeInstanceServer::renderModelNodeImageView()
{
    if (!m_renderModelNodeImageViewTimer.isActive())
        m_renderModelNodeImageViewTimer.start(0);
}

void Qt5InformationNodeInstanceServer::doRenderModelNodeImageView()
{
    // This crashes on Qt 6.0.x due to QtQuick3D issue, so the preview generation is disabled
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) || QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
    ServerNodeInstance instance;
    if (m_modelNodePreviewImageCommand.renderItemId() >= 0)
        instance = instanceForId(m_modelNodePreviewImageCommand.renderItemId());
    else
        instance = instanceForId(m_modelNodePreviewImageCommand.instanceId());

    if (instance.isSubclassOf("QQuick3DObject"))
        doRenderModelNode3DImageView();
    else if (instance.isSubclassOf("QQuickItem"))
        doRenderModelNode2DImageView();
#endif
}

void Qt5InformationNodeInstanceServer::doRenderModelNode3DImageView()
{
#ifdef QUICK3D_MODULE
    m_modelNode3DImageViewAsyncData.cleanup();
    if (m_modelNode3DImageViewData.rootItem) {
        QMetaObject::invokeMethod(m_modelNode3DImageViewData.rootItem, "destroyView");
        if (!m_modelNode3DImageViewData.contentItem)
            m_modelNode3DImageViewData.contentItem = getContentItemForRendering(m_modelNode3DImageViewData.rootItem);

        if (m_modelNodePreviewImageCache.contains(m_modelNodePreviewImageCommand.componentPath())) {
            m_modelNode3DImageViewAsyncData.renderImage
                    = m_modelNodePreviewImageCache[m_modelNodePreviewImageCommand.componentPath()];
            modelNode3DImageViewSendImageToCreator();
        } else {
            ServerNodeInstance instance = instanceForId(m_modelNodePreviewImageCommand.instanceId());
            if (!m_modelNodePreviewImageCommand.componentPath().isEmpty()
                    && instance.isSubclassOf("QQuick3DNode")) {
                // Create a new instance for Node components, as using Nodes in multiple
                // import scenes simultaneously isn't supported. And even if it was, we still
                // wouldn't want the children of the Node to appear in the preview.
                QQmlComponent component(engine());
                component.loadUrl(QUrl::fromLocalFile(m_modelNodePreviewImageCommand.componentPath()));
                m_modelNode3DImageViewAsyncData.instanceObj = qobject_cast<QQuick3DObject *>(component.create());
                if (!m_modelNode3DImageViewAsyncData.instanceObj) {
                    qWarning() << "Could not create preview component: " << component.errors();
                    m_modelNode3DImageViewAsyncData.cleanup();
                    return;
                }
                m_modelNode3DImageViewAsyncData.createdFromComponent = true;
            } else {
                m_modelNode3DImageViewAsyncData.instanceObj = instance.internalObject();
            }
            QSize renderSize = m_modelNodePreviewImageCommand.size();
            if (Internal::QuickItemNodeInstance::unifiedRenderPathOrQt6()) {
                // Requested size is already adjusted for target pixel ratio, so we have to adjust
                // back if ratio is not default for our window.
                double ratio = m_modelNode3DImageViewData.window->devicePixelRatio();
                renderSize.setWidth(qRound(qreal(renderSize.width()) / ratio));
                renderSize.setHeight(qRound(qreal(renderSize.height()) / ratio));
            }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            m_modelNode3DImageViewData.bufferDirty = m_modelNode3DImageViewData.bufferDirty
                    || m_modelNode3DImageViewData.rootItem->width() != renderSize.width()
                    || m_modelNode3DImageViewData.rootItem->height() != renderSize.height();
#endif

            m_modelNode3DImageViewData.window->resize(renderSize);
            m_modelNode3DImageViewData.rootItem->setSize(renderSize);

            QMetaObject::invokeMethod(
                        m_modelNode3DImageViewData.rootItem, "createViewForObject",
                        Q_ARG(QVariant, objectToVariant(m_modelNode3DImageViewAsyncData.instanceObj)));

            // Selection box geometry updates have an asynchronous step, so we need to do rendering
            // in asynchronous steps as well, since we are adjusting the selection box geometry
            // while finding correct zoom level.
            m_modelNode3DImageViewAsyncData.timer.start();
        }
    }
#endif
}

void Qt5InformationNodeInstanceServer::modelNode3DImageViewSendImageToCreator()
{
    if (!m_modelNode3DImageViewAsyncData.renderImage.isNull()) {
        // Key number is selected so that it is unlikely to conflict other ImageContainer use.
        ImageContainer imgContainer(m_modelNodePreviewImageCommand.instanceId(), {}, 2100000001);
        imgContainer.setImage(m_modelNode3DImageViewAsyncData.renderImage);

        // send the rendered image to creator process
        nodeInstanceClient()->handlePuppetToCreatorCommand(
                    {PuppetToCreatorCommand::RenderModelNodePreviewImage,
                     QVariant::fromValue(imgContainer)});

        m_modelNode3DImageViewAsyncData.cleanup();
    }
}

void Qt5InformationNodeInstanceServer::modelNode3DImageViewRenderStep()
{
    ++m_modelNode3DImageViewAsyncData.count;

    updateNodesRecursive(m_modelNode3DImageViewData.contentItem);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
        m_modelNode3DImageViewAsyncData.renderImage = m_modelNode3DImageViewData.window->grabWindow();
    } else {
        // Fake render loop signaling to update things like QML items as 3D textures
        m_modelNode3DImageViewData.window->beforeSynchronizing();
        m_modelNode3DImageViewData.window->beforeRendering();

        QSizeF size = qobject_cast<QQuickItem *>(m_modelNode3DImageViewData.contentItem)->size();
        QRectF renderRect(QPointF(0., 0.), size);
        m_modelNode3DImageViewAsyncData.renderImage
                = designerSupport()->renderImageForItem(m_modelNode3DImageViewData.contentItem,
                                                        renderRect, size.toSize());
        m_modelNode3DImageViewData.window->afterRendering();
    }
#else
    m_modelNode3DImageViewAsyncData.renderImage = grabRenderControl(m_modelNode3DImageViewData);
#endif
    QMetaObject::invokeMethod(m_modelNode3DImageViewData.rootItem, "afterRender");
    const bool ready = QQmlProperty::read(m_modelNode3DImageViewData.rootItem, "ready").value<bool>();
    if (ready || m_modelNode3DImageViewAsyncData.count >= 10) {
        QMetaObject::invokeMethod(m_modelNode3DImageViewData.rootItem, "destroyView");
        if (m_modelNode3DImageViewAsyncData.createdFromComponent) {
            // If component changes, puppet will need a reset anyway, so we can cache the image
            m_modelNodePreviewImageCache.insert(m_modelNodePreviewImageCommand.componentPath(),
                                                m_modelNode3DImageViewAsyncData.renderImage);
        }
        modelNode3DImageViewSendImageToCreator();
    } else {
        m_modelNode3DImageViewAsyncData.timer.start();
    }
}

static QRectF itemBoundingRect(QQuickItem *item)
{
    QRectF itemRect;
    if (item) {
        itemRect = item->boundingRect();
        if (item->clip()) {
            return itemRect;
        } else {
            const auto childItems = item->childItems();
            for (const auto &childItem : childItems) {
                QRectF mappedRect = childItem->mapRectToItem(item, itemBoundingRect(childItem));
                // Sanity check for size
                if (mappedRect.isValid() && (mappedRect.width() < 10000) && (mappedRect.height() < 10000))
                    itemRect = itemRect.united(mappedRect);
            }
        }
    }
    return itemRect;
}

void Qt5InformationNodeInstanceServer::doRenderModelNode2DImageView()
{
    if (m_modelNode2DImageViewData.rootItem) {
        if (!m_modelNode2DImageViewData.contentItem)
            m_modelNode2DImageViewData.contentItem = getContentItemForRendering(m_modelNode2DImageViewData.rootItem);

        // Key number is the same as in 3D case as they produce image for same purpose
        auto imgContainer = ImageContainer(m_modelNodePreviewImageCommand.instanceId(), {}, 2100000001);
        QImage renderImage;
        if (m_modelNodePreviewImageCache.contains(m_modelNodePreviewImageCommand.componentPath())) {
            renderImage = m_modelNodePreviewImageCache[m_modelNodePreviewImageCommand.componentPath()];
        } else {
            QQuickItem *instanceItem = nullptr;

            if (!m_modelNodePreviewImageCommand.componentPath().isEmpty()) {
                QQmlComponent component(engine());
                component.loadUrl(QUrl::fromLocalFile(m_modelNodePreviewImageCommand.componentPath()));
                instanceItem = qobject_cast<QQuickItem *>(component.create());
                if (!instanceItem) {
                    qWarning() << "Could not create preview component: " << component.errors();
                    return;
                }
            } else {
                qWarning() << "2D image preview is not supported for non-components.";
                return;
            }

            instanceItem->setParentItem(m_modelNode2DImageViewData.contentItem);

            // Some component may expect to always be shown at certain size, so their layouts may
            // not support scaling, so let's always render at the default size if item has one and
            // scale the resulting image instead.
            QSize finalSize = m_modelNodePreviewImageCommand.size();
            QRectF renderRect = itemBoundingRect(instanceItem);
            QSize renderSize = renderRect.size().toSize();
            if (renderSize.isEmpty()) {
                renderSize = finalSize;
                renderRect = QRectF(QPointF(0., 0.), QSizeF(renderSize));
            }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            m_modelNode2DImageViewData.bufferDirty = m_modelNode2DImageViewData.bufferDirty
                    || m_modelNode2DImageViewData.rootItem->width() != renderSize.width()
                    || m_modelNode2DImageViewData.rootItem->height() != renderSize.height();
#endif

            m_modelNode2DImageViewData.window->resize(renderSize);
            m_modelNode2DImageViewData.rootItem->setSize(renderSize);
            m_modelNode2DImageViewData.contentItem->setPosition(QPointF(-renderRect.x(), -renderRect.y()));

            updateNodesRecursive(m_modelNode2DImageViewData.contentItem);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
                    renderImage = m_modelNode2DImageViewData.window->grabWindow();
                } else {
                    renderImage = designerSupport()->renderImageForItem(m_modelNode2DImageViewData.contentItem,
                                                                        renderRect, renderSize);
                }
#else
                renderImage = grabRenderControl(m_modelNode2DImageViewData);
#endif

            if (!imageHasContent(renderImage))
                renderImage = nonVisualComponentPreviewImage();

            if (renderSize != finalSize)
                renderImage = renderImage.scaled(finalSize, Qt::KeepAspectRatio);

            delete instanceItem;

            // If component changes, puppet will need a reset anyway, so we can cache the image
            m_modelNodePreviewImageCache.insert(m_modelNodePreviewImageCommand.componentPath(), renderImage);
        }

        if (!renderImage.isNull()) {
            imgContainer.setImage(renderImage);

            // send the rendered image to creator process
            nodeInstanceClient()->handlePuppetToCreatorCommand({PuppetToCreatorCommand::RenderModelNodePreviewImage,
                                                                QVariant::fromValue(imgContainer)});
        }
    }
}

Qt5InformationNodeInstanceServer::Qt5InformationNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt5NodeInstanceServer(nodeInstanceClient)
{
    m_propertyChangeTimer.setInterval(100);
    m_propertyChangeTimer.setSingleShot(true);
    m_selectionChangeTimer.setSingleShot(true);
    m_render3DEditViewTimer.setSingleShot(true);
    m_inputEventTimer.setSingleShot(true);
    m_renderModelNodeImageViewTimer.setSingleShot(true);
    m_modelNode3DImageViewAsyncData.timer.setSingleShot(true);

#ifdef FPS_COUNTER
    if (!_fpsTimer) {
        _fpsTimer = new QElapsedTimer;
        _fpsTimer->start();
    }
#endif
#ifdef QUICK3D_PARTICLES_MODULE
    if (ViewConfig::isParticleViewMode()) {
        m_particleAnimationDriver = new AnimationDriver(this);
        m_particleAnimationDriver->setInterval(17);
    }
#endif
}

Qt5InformationNodeInstanceServer::~Qt5InformationNodeInstanceServer()
{
    m_editView3DSetupDone = false;

    m_propertyChangeTimer.stop();
    m_selectionChangeTimer.stop();
    m_render3DEditViewTimer.stop();
    m_inputEventTimer.stop();
    m_renderModelNodeImageViewTimer.stop();
    m_modelNode3DImageViewAsyncData.timer.stop();

    if (m_editView3DData.rootItem)
        m_editView3DData.rootItem->disconnect(this);

    for (auto view : qAsConst(m_view3Ds))
        view->disconnect();
    for (auto node : qAsConst(m_3DSceneMap))
        node->disconnect();

    if (m_editView3DData.rootItem)
        QMetaObject::invokeMethod(m_editView3DData.rootItem, "aboutToShutDown", Qt::DirectConnection);

    if (!Internal::QuickItemNodeInstance::unifiedRenderPathOrQt6()) {
        if (m_editView3DData.contentItem)
            designerSupport()->derefFromEffectItem(m_editView3DData.contentItem);
        if (m_modelNode3DImageViewData.contentItem)
            designerSupport()->derefFromEffectItem(m_modelNode3DImageViewData.contentItem);
        if (m_modelNode2DImageViewData.contentItem)
            designerSupport()->derefFromEffectItem(m_modelNode2DImageViewData.contentItem);
    }
}

void Qt5InformationNodeInstanceServer::sendTokenBack()
{
    foreach (const TokenCommand &command, m_tokenList)
        nodeInstanceClient()->token(command);

    m_tokenList.clear();
}

void Qt5InformationNodeInstanceServer::token(const TokenCommand &command)
{
    m_tokenList.append(command);
    startRenderTimer();
}

bool Qt5InformationNodeInstanceServer::isDirtyRecursiveForNonInstanceItems(QQuickItem *item) const
{
    static DesignerSupport::DirtyType informationsDirty = DesignerSupport::DirtyType(DesignerSupport::TransformUpdateMask
                                                                              | DesignerSupport::ContentUpdateMask
                                                                              | DesignerSupport::Visible
                                                                              | DesignerSupport::ZValue
                                                                              | DesignerSupport::OpacityValue);

    if (DesignerSupport::isDirty(item, informationsDirty))
        return true;

    foreach (QQuickItem *childItem, item->childItems()) {
        if (!hasInstanceForObject(childItem)) {
            if (DesignerSupport::isDirty(childItem, informationsDirty))
                return true;
            else if (isDirtyRecursiveForNonInstanceItems(childItem))
                return true;
        }
    }

    return false;
}

bool Qt5InformationNodeInstanceServer::isDirtyRecursiveForParentInstances(QQuickItem *item) const
{
    if (DesignerSupport::isDirty(item,  DesignerSupport::TransformUpdateMask))
        return true;

    QQuickItem *parentItem = item->parentItem();

    if (parentItem) {
        if (hasInstanceForObject(parentItem))
            return false;

        return isDirtyRecursiveForParentInstances(parentItem);
    }

    return false;
}

/* This method allows changing the selection from the puppet */
void Qt5InformationNodeInstanceServer::selectInstances(const QList<ServerNodeInstance> &instanceList)
{
    nodeInstanceClient()->selectionChanged(createChangeSelectionCommand(instanceList));
}

/* This method allows changing property values from the puppet
 * For performance reasons (and the undo stack) properties should always be modifed in 'bulks'.
 */
void Qt5InformationNodeInstanceServer::modifyProperties(
    const QVector<NodeInstanceServer::InstancePropertyValueTriple> &properties)
{
    nodeInstanceClient()->valuesModified(createValuesModifiedCommand(properties));
}

QList<ServerNodeInstance> Qt5InformationNodeInstanceServer::createInstances(
        const QVector<InstanceContainer> &container)
{
    const auto createdInstances = NodeInstanceServer::createInstances(container);

    if (m_editView3DSetupDone) {
        add3DViewPorts(createdInstances);
        add3DScenes(createdInstances);
        createCameraAndLightGizmos(createdInstances);
    }

    render3DEditView();

    return createdInstances;
}

void Qt5InformationNodeInstanceServer::initializeAuxiliaryViews()
{
#ifdef QUICK3D_MODULE
    if (ViewConfig::isQuick3DMode())
        createEditView3D();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    createAuxiliaryQuickView(QUrl("qrc:/qtquickplugin/mockfiles/qt6/ModelNode3DImageView.qml"),
                             m_modelNode3DImageViewData);
#else
    createAuxiliaryQuickView(QUrl("qrc:/qtquickplugin/mockfiles/qt5/ModelNode3DImageView.qml"),
                             m_modelNode3DImageViewData);
#endif
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    createAuxiliaryQuickView(QUrl("qrc:/qtquickplugin/mockfiles/qt6/ModelNode2DImageView.qml"),
                             m_modelNode2DImageViewData);
#else
    createAuxiliaryQuickView(QUrl("qrc:/qtquickplugin/mockfiles/qt5/ModelNode2DImageView.qml"),
                             m_modelNode2DImageViewData);
#endif
    m_modelNode2DImageViewData.window->setDefaultAlphaBuffer(true);
    m_modelNode2DImageViewData.window->setColor(Qt::transparent);
}

void Qt5InformationNodeInstanceServer::handleObjectPropertyChangeTimeout()
{
    modifyVariantValue(m_changedNodes, m_changedProperties,
                       ValuesModifiedCommand::TransactionOption::None);
}

void Qt5InformationNodeInstanceServer::handleSelectionChangeTimeout()
{
    changeSelection(m_lastSelectionChangeCommand);
}

void Qt5InformationNodeInstanceServer::createCameraAndLightGizmos(
        const QList<ServerNodeInstance> &instanceList) const
{
    QHash<QObject *, QObjectList> cameras;
    QHash<QObject *, QObjectList> lights;
    QHash<QObject *, QObjectList> particleSystems;

    for (const ServerNodeInstance &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DCamera"))
            cameras[find3DSceneRoot(instance)] << instance.internalObject();
        else if (instance.isSubclassOf("QQuick3DAbstractLight"))
            lights[find3DSceneRoot(instance)] << instance.internalObject();
        else if (instance.isSubclassOf("QQuick3DParticleSystem"))
            particleSystems[find3DSceneRoot(instance)] << instance.internalObject();

    }

    auto cameraIt = cameras.constBegin();
    while (cameraIt != cameras.constEnd()) {
        const auto cameraObjs = cameraIt.value();
        for (auto &obj : cameraObjs) {
            QMetaObject::invokeMethod(m_editView3DData.rootItem, "addCameraGizmo",
                                      Q_ARG(QVariant, objectToVariant(cameraIt.key())),
                                      Q_ARG(QVariant, objectToVariant(obj)));
        }
        ++cameraIt;
    }
    auto lightIt = lights.constBegin();
    while (lightIt != lights.constEnd()) {
        const auto lightObjs = lightIt.value();
        for (auto &obj : lightObjs) {
            QMetaObject::invokeMethod(m_editView3DData.rootItem, "addLightGizmo",
                                      Q_ARG(QVariant, objectToVariant(lightIt.key())),
                                      Q_ARG(QVariant, objectToVariant(obj)));
        }
        ++lightIt;
    }
    auto particleIt = particleSystems.constBegin();
    while (particleIt != particleSystems.constEnd()) {
        const auto particleObjs = particleIt.value();
        for (auto &obj : particleObjs) {
            QMetaObject::invokeMethod(m_editView3DData.rootItem, "addParticleSystemGizmo",
                                      Q_ARG(QVariant, objectToVariant(particleIt.key())),
                                      Q_ARG(QVariant, objectToVariant(obj)));
        }
        ++particleIt;
    }
}

void Qt5InformationNodeInstanceServer::add3DViewPorts(const QList<ServerNodeInstance> &instanceList)
{
    for (const ServerNodeInstance &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DViewport")) {
            QObject *obj = instance.internalObject();
            if (!m_view3Ds.contains(obj))  {
                m_view3Ds << obj;
                QObject::connect(obj, SIGNAL(widthChanged()), this, SLOT(handleView3DSizeChange()));
                QObject::connect(obj, SIGNAL(heightChanged()), this, SLOT(handleView3DSizeChange()));
                QObject::connect(obj, &QObject::destroyed,
                                 this, &Qt5InformationNodeInstanceServer::handleView3DDestroyed);
            }
        }
    }
}

void Qt5InformationNodeInstanceServer::add3DScenes(const QList<ServerNodeInstance> &instanceList)
{
    for (const ServerNodeInstance &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DNode")) {
            QObject *sceneRoot = find3DSceneRoot(instance);
            QObject *obj = instance.internalObject();
            if (!m_3DSceneMap.contains(sceneRoot, obj)) {
                m_3DSceneMap.insert(sceneRoot, obj);
                QObject::connect(obj, &QObject::destroyed,
                                 this, &Qt5InformationNodeInstanceServer::handleNode3DDestroyed);
            }
        }
    }
}

QObject *Qt5InformationNodeInstanceServer::findView3DForInstance(const ServerNodeInstance &instance) const
{
#ifdef QUICK3D_MODULE
    if (!instance.isValid())
        return {};

    // View3D of an instance is one of the following, in order of priority:
    // - Any direct ancestor View3D of the instance
    // - Any View3D that specifies the instance's scene as importScene
    ServerNodeInstance checkInstance = instance;
    while (checkInstance.isValid()) {
        if (checkInstance.isSubclassOf("QQuick3DViewport"))
            return checkInstance.internalObject();
        else
            checkInstance = checkInstance.parent();
    }

    // If no ancestor View3D was found, check if the scene root is specified as importScene in
    // some View3D.
    QObject *sceneRoot = find3DSceneRoot(instance);
    for (const auto &view3D : qAsConst(m_view3Ds)) {
        auto view = qobject_cast<QQuick3DViewport *>(view3D);
        if (view && sceneRoot == view->importScene())
            return view3D;
    }
#else
    Q_UNUSED(instance)
#endif
    return {};
}

QObject *Qt5InformationNodeInstanceServer::findView3DForSceneRoot(QObject *sceneRoot) const
{
#ifdef QUICK3D_MODULE
    if (!sceneRoot)
        return {};

    if (hasInstanceForObject(sceneRoot)) {
        return findView3DForInstance(instanceForObject(sceneRoot));
    } else {
        // No instance, so the scene root must be scene property of one of the views
        for (const auto &view3D : qAsConst(m_view3Ds)) {
            auto view = qobject_cast<QQuick3DViewport *>(view3D);
            if (view && sceneRoot == view->scene())
                return view3D;
        }
    }
#else
    Q_UNUSED(sceneRoot)
#endif
    return {};
}

QObject *Qt5InformationNodeInstanceServer::find3DSceneRoot(const ServerNodeInstance &instance) const
{
#ifdef QUICK3D_MODULE
    // The root of a 3D scene is any QQuick3DNode that doesn't have QQuick3DNode as parent.
    // One exception is QQuick3DSceneRootNode that has only a single child QQuick3DNode (not
    // a subclass of one, but exactly QQuick3DNode). In that case we consider the single child node
    // to be the scene root (as QQuick3DSceneRootNode is not visible in the navigator scene graph).

    if (!instance.isValid())
        return nullptr;

    QQuick3DNode *childNode = nullptr;
    auto countChildNodes = [&childNode](QQuick3DViewport *view) -> int {
        QQuick3DNode *sceneNode = view->scene();
        QList<QQuick3DObject *> children = sceneNode->childItems();
        int nodeCount = 0;
        for (const auto &child : children) {
            auto nodeChild = qobject_cast<QQuick3DNode *>(child);
            if (nodeChild) {
                ++nodeCount;
                childNode = nodeChild;
            }
        }
        return nodeCount;
    };

    // In case of View3D is selected, the root scene is whatever is contained in View3D, or
    // importScene, in case there is no content in View3D
    QObject *obj = instance.internalObject();
    auto view = qobject_cast<QQuick3DViewport *>(obj);
    if (view) {
        int nodeCount = countChildNodes(view);
        if (nodeCount == 0)
            return view->importScene();
        else if (nodeCount == 1)
            return childNode;
        else
            return view->scene();
    }

    ServerNodeInstance checkInstance = instance;
    bool foundNode = checkInstance.isSubclassOf("QQuick3DNode");
    while (checkInstance.isValid()) {
        ServerNodeInstance parentInstance = checkInstance.parent();
        if (parentInstance.isSubclassOf("QQuick3DViewport")) {
            view = qobject_cast<QQuick3DViewport *>(parentInstance.internalObject());
            int nodeCount = countChildNodes(view);
            if (nodeCount == 1)
                return childNode;
            else
                return view->scene();
        } else if (parentInstance.isSubclassOf("QQuick3DNode")) {
            foundNode = true;
            checkInstance = parentInstance;
        } else {
            if (!foundNode) {
                // We haven't found any node yet, continue the search
                checkInstance = parentInstance;
            } else {
                return checkInstance.internalObject();
            }
        }
    }
#else
    Q_UNUSED(instance)
#endif
    return nullptr;
}

QObject *Qt5InformationNodeInstanceServer::find3DSceneRoot(QObject *obj) const
{
#ifdef QUICK3D_MODULE
    if (hasInstanceForObject(obj))
        return find3DSceneRoot(instanceForObject(obj));

    // If there is no instance, obj could be a scene in a View3D
    for (const auto &viewObj : qAsConst(m_view3Ds)) {
        const auto view = qobject_cast<QQuick3DViewport *>(viewObj);
        if (view && view->scene() == obj)
            return obj;
    }
#else
    Q_UNUSED(obj)
#endif
    // Some other non-instance object, assume it's not part of any scene
    return nullptr;
}

void Qt5InformationNodeInstanceServer::setup3DEditView(const QList<ServerNodeInstance> &instanceList,
                                                       const QHash<QString, QVariantMap> &toolStates)
{
#ifdef QUICK3D_MODULE
    if (!m_editView3DData.rootItem)
        return;

    ServerNodeInstance root = rootNodeInstance();

    add3DViewPorts(instanceList);
    add3DScenes(instanceList);

    QObject::connect(m_editView3DData.rootItem, SIGNAL(selectionChanged(QVariant)),
                     this, SLOT(handleSelectionChanged(QVariant)));
    QObject::connect(m_editView3DData.rootItem, SIGNAL(commitObjectProperty(QVariant, QVariant)),
                     this, SLOT(handleObjectPropertyCommit(QVariant, QVariant)));
    QObject::connect(m_editView3DData.rootItem, SIGNAL(changeObjectProperty(QVariant, QVariant)),
                     this, SLOT(handleObjectPropertyChange(QVariant, QVariant)));
    QObject::connect(m_editView3DData.rootItem, SIGNAL(notifyActiveSceneChange()),
                     this, SLOT(handleActiveSceneChange()));
    QObject::connect(&m_propertyChangeTimer, &QTimer::timeout,
                     this, &Qt5InformationNodeInstanceServer::handleObjectPropertyChangeTimeout);
    QObject::connect(&m_selectionChangeTimer, &QTimer::timeout,
                     this, &Qt5InformationNodeInstanceServer::handleSelectionChangeTimeout);
    QObject::connect(&m_render3DEditViewTimer, &QTimer::timeout,
                     this, &Qt5InformationNodeInstanceServer::doRender3DEditView);
    QObject::connect(&m_inputEventTimer, &QTimer::timeout,
                     this, &Qt5InformationNodeInstanceServer::handleInputEvents);
    QObject::connect(&m_modelNode3DImageViewAsyncData.timer, &QTimer::timeout,
                     this, &Qt5InformationNodeInstanceServer::modelNode3DImageViewRenderStep);

    QString lastSceneId;
    auto helper = qobject_cast<QmlDesigner::Internal::GeneralHelper *>(m_3dHelper);
    if (helper) {
        auto it = toolStates.constBegin();
        while (it != toolStates.constEnd()) {
            helper->initToolStates(it.key(), it.value());
            ++it;
        }
        if (toolStates.contains(helper->globalStateId())) {
            if (toolStates[helper->globalStateId()].contains(helper->rootSizeKey())) {
                QSize size = toolStates[helper->globalStateId()][helper->rootSizeKey()].value<QSize>();
                m_editView3DData.rootItem->setSize(size);
                m_editView3DData.window->setGeometry(0, 0, size.width(), size.height());
            }
            if (toolStates[helper->globalStateId()].contains(helper->lastSceneIdKey()))
                lastSceneId = toolStates[helper->globalStateId()][helper->lastSceneIdKey()].toString();
        }
    }

    // Find a scene to show
    m_active3DScene = nullptr;
    m_active3DView = nullptr;
    if (!m_3DSceneMap.isEmpty()) {
        // Restore the previous scene if possible
        if (!lastSceneId.isEmpty()) {
            const auto keys = m_3DSceneMap.uniqueKeys();
            for (const auto key : keys) {
                m_active3DScene = key;
                m_active3DView = findView3DForSceneRoot(m_active3DScene);
                ServerNodeInstance sceneInstance = active3DSceneInstance();
                if (lastSceneId == sceneInstance.id())
                    break;
            }
        } else {
            m_active3DScene = m_3DSceneMap.begin().key();
            m_active3DView = findView3DForSceneRoot(m_active3DScene);
        }
    }

    m_editView3DSetupDone = true;

    if (toolStates.contains({})) {
        // Update tool state to an existing no-scene state before updating the active scene to
        // ensure the previous state is inherited properly in all cases.
        QMetaObject::invokeMethod(m_editView3DData.rootItem, "updateToolStates", Qt::QueuedConnection,
                                  Q_ARG(QVariant, toolStates[{}]),
                                  Q_ARG(QVariant, QVariant::fromValue(false)));
    }

    updateActiveSceneToEditView3D();

    createCameraAndLightGizmos(instanceList);

    // Queue two renders to make sure icon gizmos update properly
    render3DEditView(2);
#else
    Q_UNUSED(instanceList)
    Q_UNUSED(toolStates)
#endif
}

void Qt5InformationNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;
    if (!inFunction) {
        inFunction = true;

        DesignerSupport::polishItems(quickWindow());

        QSet<ServerNodeInstance> informationChangedInstanceSet;
        QVector<InstancePropertyPair> propertyChangedList;

        if (quickWindow()) {
            foreach (QQuickItem *item, allItems()) {
                if (item && hasInstanceForObject(item)) {
                    ServerNodeInstance instance = instanceForObject(item);

                    if (isDirtyRecursiveForNonInstanceItems(item))
                        informationChangedInstanceSet.insert(instance);
                    else if (isDirtyRecursiveForParentInstances(item))
                        informationChangedInstanceSet.insert(instance);

                    if (DesignerSupport::isDirty(item, DesignerSupport::ParentChanged)) {
                        m_parentChangedSet.insert(instance);
                        informationChangedInstanceSet.insert(instance);
                    }
                }
            }

            foreach (const InstancePropertyPair& property, changedPropertyList()) {
                const ServerNodeInstance instance = property.first;
                if (instance.isValid()) {
                    if (property.second.contains("anchors"))
                        informationChangedInstanceSet.insert(instance);

                    propertyChangedList.append(property);
                }
            }

            resetAllItems();
            clearChangedPropertyList();

            sendTokenBack();

            if (!informationChangedInstanceSet.isEmpty())
                nodeInstanceClient()->informationChanged(
                    createAllInformationChangedCommand(QtHelpers::toList(informationChangedInstanceSet)));

            if (!propertyChangedList.isEmpty())
                nodeInstanceClient()->valuesChanged(createValuesChangedCommand(propertyChangedList));

            if (!m_parentChangedSet.isEmpty()) {
                sendChildrenChangedCommand(QtHelpers::toList(m_parentChangedSet));
                updateLockedAndHiddenStates(m_parentChangedSet);
                m_parentChangedSet.clear();
            }

            if (!m_completedComponentList.isEmpty()) {
                nodeInstanceClient()->componentCompleted(createComponentCompletedCommand(m_completedComponentList));
                m_completedComponentList.clear();
            }

            slowDownRenderTimer();
            nodeInstanceClient()->flush();
            nodeInstanceClient()->synchronizeWithClientProcess();
        }

        inFunction = false;
    }
}

void Qt5InformationNodeInstanceServer::reparentInstances(const ReparentInstancesCommand &command)
{
    foreach (const ReparentContainer &container, command.reparentInstances()) {
        if (hasInstanceForId(container.instanceId())) {
            ServerNodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                m_parentChangedSet.insert(instance);
            }
        }
    }

    Qt5NodeInstanceServer::reparentInstances(command);

    if (m_editView3DSetupDone)
        resolveSceneRoots();

    // Make sure selection is in sync after all reparentings are done
    m_selectionChangeTimer.start(0);
}

void Qt5InformationNodeInstanceServer::clearScene(const ClearSceneCommand &command)
{
    Qt5NodeInstanceServer::clearScene(command);

    m_parentChangedSet.clear();
    m_completedComponentList.clear();
}

void Qt5InformationNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    Qt5NodeInstanceServer::createScene(command);

    QList<ServerNodeInstance> instanceList;
    for (const InstanceContainer &container : std::as_const(command.instances)) {
        if (hasInstanceForId(container.instanceId())) {
            ServerNodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid())
                instanceList.append(instance);
        }
    }

    nodeInstanceClient()->informationChanged(createAllInformationChangedCommand(instanceList, true));
    nodeInstanceClient()->valuesChanged(createValuesChangedCommand(instanceList));
    sendChildrenChangedCommand(instanceList);
    nodeInstanceClient()->componentCompleted(createComponentCompletedCommand(instanceList));

    if (ViewConfig::isQuick3DMode()) {
        setup3DEditView(instanceList, command.edit3dToolStates);
        updateRotationBlocks(command.auxiliaryChanges);
    }

    QObject::connect(&m_renderModelNodeImageViewTimer, &QTimer::timeout,
                     this, &Qt5InformationNodeInstanceServer::doRenderModelNodeImageView);
#ifdef IMPORT_QUICK3D_ASSETS
    QTimer::singleShot(0, this, &Qt5InformationNodeInstanceServer::resolveImportSupport);
#endif
}

void Qt5InformationNodeInstanceServer::sendChildrenChangedCommand(const QList<ServerNodeInstance> &childList)
{
    QSet<ServerNodeInstance> parentSet;
    QList<ServerNodeInstance> noParentList;

    foreach (const ServerNodeInstance &child, childList) {
        if (child.isValid()) {
            if (!child.hasParent()) {
                noParentList.append(child);
            } else {
                ServerNodeInstance parent = child.parent();
                if (parent.isValid()) {
                    parentSet.insert(parent);
                } else {
                    noParentList.append(child);
                }
            }
        }
    }

    foreach (const ServerNodeInstance &parent, parentSet)
        nodeInstanceClient()->childrenChanged(createChildrenChangedCommand(parent, parent.childItems()));

    if (!noParentList.isEmpty())
        nodeInstanceClient()->childrenChanged(createChildrenChangedCommand(ServerNodeInstance(), noParentList));

}

void Qt5InformationNodeInstanceServer::completeComponent(const CompleteComponentCommand &command)
{
    Qt5NodeInstanceServer::completeComponent(command);

    QList<ServerNodeInstance> instanceList;
    foreach (qint32 instanceId, command.instances()) {
        if (hasInstanceForId(instanceId)) {
            ServerNodeInstance instance = instanceForId(instanceId);
            if (instance.isValid()) {
                instanceList.append(instance);
            }
        }
    }

    m_completedComponentList.append(instanceList);

    nodeInstanceClient()->valuesChanged(createValuesChangedCommand(instanceList));
    nodeInstanceClient()->informationChanged(createAllInformationChangedCommand(instanceList, true));
}

void QmlDesigner::Qt5InformationNodeInstanceServer::removeSharedMemory(const QmlDesigner::RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Values")
        ValuesChangedCommand::removeSharedMemorys(command.keyNumbers());
}

void Qt5InformationNodeInstanceServer::changeSelection(const ChangeSelectionCommand &command)
{
    if (!m_editView3DSetupDone)
        return;

    m_lastSelectionChangeCommand = command;
    if (m_selectionChangeTimer.isActive()) {
        // If selection was recently changed by puppet, hold updating the selection for a bit to
        // avoid selection flicker, especially in multiselect cases.
        // Add additional time in case more commands are still coming through
        m_selectionChangeTimer.start(500);
        return;
    }

    // Find a scene root of the selection to update active scene shown
    const QVector<qint32> instanceIds = command.instanceIds();
    QVariantList selectedObjs;
    QObject *firstSceneRoot = nullptr;
    ServerNodeInstance firstInstance;
    for (qint32 id : instanceIds) {
        if (hasInstanceForId(id)) {
            ServerNodeInstance instance = instanceForId(id);
            QObject *sceneRoot = find3DSceneRoot(instance);
            if (!firstSceneRoot && sceneRoot) {
                firstSceneRoot = sceneRoot;
                firstInstance = instance;
            }
            QObject *object = nullptr;
            if (firstSceneRoot && sceneRoot == firstSceneRoot && instance.isSubclassOf("QQuick3DNode"))
                object = instance.internalObject();

#ifdef QUICK3D_PARTICLES_MODULE
            auto particlesystem = qobject_cast<QQuick3DParticleSystem *>(instance.internalObject());
            if (particlesystem) {
                handleParticleSystemSelected(particlesystem);
            } else {
                particlesystem = parentParticleSystem(instance.internalObject());
                if (particlesystem) {
                    if (particlesystem != m_targetParticleSystem)
                        handleParticleSystemSelected(particlesystem);
                } else {
                    handleParticleSystemDeselected(instance.internalObject());
                }
            }
#endif
            auto isSelectableAsRoot = [&]() -> bool {
#ifdef QUICK3D_MODULE
                if (qobject_cast<QQuick3DModel *>(object)
                    || qobject_cast<QQuick3DCamera *>(object)
                    || qobject_cast<QQuick3DAbstractLight *>(object)
#ifdef QUICK3D_PARTICLES_MODULE
                    || qobject_cast<QQuick3DParticleSystem *>(object)
#endif
                ) {
                    return true;
                }
                // Node is a component if it has node children that have no instances
                auto node = qobject_cast<QQuick3DNode *>(object);
                if (node) {
                    const auto childItems = node->childItems();
                    for (const auto &childItem : childItems) {
                        if (qobject_cast<QQuick3DNode *>(childItem) && !hasInstanceForObject(childItem))
                            return true;
                    }
                }
#endif
                return false;
            };
            if (object && (firstSceneRoot != object || isSelectableAsRoot()))
                selectedObjs << objectToVariant(object);
        }
    }

    if (firstSceneRoot && m_active3DScene != firstSceneRoot) {
        m_active3DScene = firstSceneRoot;
        m_active3DView = findView3DForInstance(firstInstance);
        updateActiveSceneToEditView3D();
    }

    // Ensure the UI has enough selection box items. If it doesn't yet have them, which can be the
    // case when the first selection processed is a multiselection, we wait a bit as
    // using the new boxes immediately leads to visual glitches.
    int boxCount = m_editView3DData.rootItem->property("selectionBoxes").value<QVariantList>().size();
    if (boxCount < selectedObjs.size()) {
        QMetaObject::invokeMethod(m_editView3DData.rootItem, "ensureSelectionBoxes",
                                  Q_ARG(QVariant, QVariant::fromValue(selectedObjs.size())));
        m_selectionChangeTimer.start(0);
    } else {
        QMetaObject::invokeMethod(m_editView3DData.rootItem, "selectObjects",
                                  Q_ARG(QVariant, QVariant::fromValue(selectedObjs)));
    }

    render3DEditView(2);
}

void Qt5InformationNodeInstanceServer::changePropertyValues(const ChangeValuesCommand &command)
{
    bool hasDynamicProperties = false;
    const QVector<PropertyValueContainer> values = command.valueChanges();
    for (const PropertyValueContainer &container : values) {
        if (!container.isReflected()) {
            hasDynamicProperties |= container.isDynamic();
            setInstancePropertyVariant(container);
        }
    }

    if (hasDynamicProperties)
        refreshBindings();

    startRenderTimer();

    render3DEditView();
}

void Qt5InformationNodeInstanceServer::removeInstances(const RemoveInstancesCommand &command)
{
    int nodeCount = m_3DSceneMap.size();

    removeRotationBlocks(command.instanceIds());

    Qt5NodeInstanceServer::removeInstances(command);

    if (nodeCount != m_3DSceneMap.size()) {
        // Some nodes were removed, which can cause scene root to change for nodes under View3D
        // objects, so re-resolve scene roots.
        resolveSceneRoots();
    }

    if (m_editView3DSetupDone && (!m_active3DScene || !m_active3DView)) {
        if (!m_active3DScene && !m_3DSceneMap.isEmpty())
            m_active3DScene = m_3DSceneMap.begin().key();
        m_active3DView = findView3DForSceneRoot(m_active3DScene);
        updateActiveSceneToEditView3D();
    }
    render3DEditView();
}

void Qt5InformationNodeInstanceServer::inputEvent(const InputEventCommand &command)
{
    if (m_editView3DData.window) {
        m_pendingInputEventCommands.append(command);
        if (!m_inputEventTimer.isActive())
            m_inputEventTimer.start(0);
    }
}

void Qt5InformationNodeInstanceServer::view3DAction(const View3DActionCommand &command)
{
    if (!m_editView3DSetupDone)
        return;

    QVariantMap updatedState;
    int renderCount = 1;

    switch (command.type()) {
    case View3DActionCommand::MoveTool:
        updatedState.insert("transformMode", 0);
        break;
    case View3DActionCommand::RotateTool:
        updatedState.insert("transformMode", 1);
        break;
    case View3DActionCommand::ScaleTool:
        updatedState.insert("transformMode", 2);
        break;
    case View3DActionCommand::FitToView:
        QMetaObject::invokeMethod(m_editView3DData.rootItem, "fitToView");
        break;
    case View3DActionCommand::SelectionModeToggle:
        updatedState.insert("selectionMode", command.isEnabled() ? 1 : 0);
        break;
    case View3DActionCommand::CameraToggle:
        updatedState.insert("usePerspective", command.isEnabled());
        // It can take a couple frames to properly update icon gizmo positions
        renderCount = 2;
        break;
    case View3DActionCommand::OrientationToggle:
        updatedState.insert("globalOrientation", command.isEnabled());
        break;
    case View3DActionCommand::EditLightToggle:
        updatedState.insert("showEditLight", command.isEnabled());
        break;
    case View3DActionCommand::ShowGrid:
        updatedState.insert("showGrid", command.isEnabled());
        break;
#ifdef QUICK3D_PARTICLES_MODULE
    case View3DActionCommand::ParticlesPlay:
        m_particleAnimationPlaying = command.isEnabled();
        updatedState.insert("particlePlay", command.isEnabled());
        if (m_particleAnimationPlaying) {
            m_particleAnimationDriver->reset();
            m_particleAnimationDriver->restart();
            m_particleAnimationDriver->setSeekerEnabled(false);
            m_particleAnimationDriver->setSeekerPosition(0);
        } else {
            m_particleAnimationDriver->reset();
            m_particleAnimationDriver->setSeekerEnabled(true);
        }
        break;
    case View3DActionCommand::ParticlesRestart:
        resetParticleSystem();
        m_particleAnimationPlaying = true;
        m_particleAnimationDriver->restart();
        m_particleAnimationDriver->setSeekerEnabled(false);
        m_particleAnimationDriver->setSeekerPosition(0);
        break;
    case View3DActionCommand::ParticlesSeek:
        m_particleAnimationDriver->setSeekerPosition(static_cast<const View3DSeekActionCommand &>(command).position());
        break;
#endif
    default:
        break;
    }

    if (!updatedState.isEmpty()) {
        QMetaObject::invokeMethod(m_editView3DData.rootItem, "updateToolStates",
                                  Q_ARG(QVariant, updatedState),
                                  Q_ARG(QVariant, QVariant::fromValue(false)));
    }

    render3DEditView(renderCount);
}

void Qt5InformationNodeInstanceServer::requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command)
{
    m_modelNode3DImageViewAsyncData.timer.stop();
    m_modelNodePreviewImageCommand = command;
    renderModelNodeImageView();
}

void Qt5InformationNodeInstanceServer::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
{
    updateRotationBlocks(command.auxiliaryChanges);
    Qt5NodeInstanceServer::changeAuxiliaryValues(command);
    render3DEditView();
}

void Qt5InformationNodeInstanceServer::changePropertyBindings(const ChangeBindingsCommand &command)
{
    Qt5NodeInstanceServer::changePropertyBindings(command);
    render3DEditView();
}

void Qt5InformationNodeInstanceServer::changeIds(const ChangeIdsCommand &command)
{
    Qt5NodeInstanceServer::changeIds(command);

#ifdef QUICK3D_MODULE
    if (m_editView3DSetupDone) {
        ServerNodeInstance sceneInstance = active3DSceneInstance();
        if (m_active3DSceneUpdatePending) {
            const QString sceneId = sceneInstance.id();
            if (!sceneId.isEmpty())
                updateActiveSceneToEditView3D();
        } else {
            qint32 sceneInstanceId = sceneInstance.instanceId();
            for (const auto &id : command.ids) {
                if (sceneInstanceId == id.instanceId()) {
                    QMetaObject::invokeMethod(m_editView3DData.rootItem, "handleActiveSceneIdChange",
                                              Qt::QueuedConnection,
                                              Q_ARG(QVariant, QVariant(sceneInstance.id())));
                    render3DEditView();
                    break;
                }
            }
        }
    }
#endif
}

void Qt5InformationNodeInstanceServer::changeState(const ChangeStateCommand &command)
{
    Qt5NodeInstanceServer::changeState(command);

    render3DEditView();
}

void Qt5InformationNodeInstanceServer::removeProperties(const RemovePropertiesCommand &command)
{
    Qt5NodeInstanceServer::removeProperties(command);

    render3DEditView();
}

void Qt5InformationNodeInstanceServer::handleInstanceLocked(const ServerNodeInstance &instance,
                                                            bool enable, bool checkAncestors)
{
#ifdef QUICK3D_MODULE
    if (!ViewConfig::isQuick3DMode())
        return;

    bool edit3dLocked = enable;
    if (!edit3dLocked || checkAncestors) {
        auto parentInst = instance.parent();
        while (!edit3dLocked && parentInst.isValid()) {
            edit3dLocked = parentInst.internalInstance()->isLockedInEditor();
            parentInst = parentInst.parent();
        }
    }

    QObject *obj = instance.internalObject();
    auto node = qobject_cast<QQuick3DNode *>(obj);
    if (node)
        node->setProperty("_edit3dLocked", edit3dLocked);
    const auto children = obj->children();
    for (auto child : children) {
        if (hasInstanceForObject(child)) {
            const ServerNodeInstance childInstance = instanceForObject(child);
            if (childInstance.isValid()) {
                auto objInstance = childInstance.internalInstance();
                // Don't override explicit lock on children
                handleInstanceLocked(childInstance, edit3dLocked || objInstance->isLockedInEditor(), false);
            }
        }
    }
#else
    Q_UNUSED(instance);
    Q_UNUSED(enable);
    Q_UNUSED(checkAncestors);
#endif
}

void Qt5InformationNodeInstanceServer::handleInstanceHidden(const ServerNodeInstance &instance,
                                                            bool enable, bool checkAncestors)
{
#ifdef QUICK3D_MODULE
    if (!ViewConfig::isQuick3DMode())
        return;

    bool edit3dHidden = enable;
    if (!edit3dHidden || checkAncestors) {
        // We do not care about hidden status of non-3D ancestor nodes, as the 3D scene
        // can be considered a separate visual entity in the whole scene.
        auto parentInst = instance.parent();
        while (!edit3dHidden && parentInst.isValid() && qobject_cast<QQuick3DNode *>(parentInst.internalObject())) {
            edit3dHidden = parentInst.internalInstance()->isHiddenInEditor();
            parentInst = parentInst.parent();
        }
    }

    auto node = qobject_cast<QQuick3DNode *>(instance.internalObject());
    if (node) {
        bool isInstanceHidden = false;
        auto getQuick3DInstanceAndHidden = [this, &isInstanceHidden](QQuick3DObject *obj) -> ServerNodeInstance {
            if (hasInstanceForObject(obj)) {
                const ServerNodeInstance instance = instanceForObject(obj);
                if (instance.isValid() && qobject_cast<QQuick3DNode *>(instance.internalObject())) {
                    auto objInstance = instance.internalInstance();
                    isInstanceHidden = objInstance->isHiddenInEditor();
                    return instance;
                }
            }
            return {};
        };
        // Always make sure the hide status is correct on the node tree from this point on,
        // as changes in the node tree (reparenting, adding new nodes) can make the previously set
        // hide status based on ancestor unreliable.
        node->setProperty("_edit3dHidden", edit3dHidden);
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 1)
        if (auto model = qobject_cast<QQuick3DModel *>(node))
            model->setPickable(!edit3dHidden); // allow 3D objects to receive mouse clicks
#endif
        const auto childItems = node->childItems();
        for (auto childItem : childItems) {
            const ServerNodeInstance quick3dInstance = getQuick3DInstanceAndHidden(childItem);
            if (quick3dInstance.isValid()) {
                // Don't override explicit hide in children
                handleInstanceHidden(quick3dInstance, edit3dHidden || isInstanceHidden, false);
            } else {
                // Children of components do not have instances, but will still need to be pickable
                std::function<void(QQuick3DNode *)> checkChildren;
                checkChildren = [&](QQuick3DNode *checkNode) {
                    const auto childItems = checkNode->childItems();
                    for (auto child : childItems) {
                        if (auto childNode = qobject_cast<QQuick3DNode *>(child))
                            checkChildren(childNode);
                    }
                    if (auto checkModel = qobject_cast<QQuick3DModel *>(checkNode)) {
                        QVariant value;
                        if (!edit3dHidden)
                            value = QVariant::fromValue(node);
                        // Specify the actual pick target with dynamic property
                        checkModel->setProperty("_pickTarget", value);
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 1)
                        checkModel->setPickable(!edit3dHidden);
#endif
                    }
                };
                if (auto childNode = qobject_cast<QQuick3DNode *>(childItem))
                    checkChildren(childNode);
            }
        }
    }
#else
    Q_UNUSED(instance);
    Q_UNUSED(enable);
    Q_UNUSED(checkAncestors);
#endif
}

bool Qt5InformationNodeInstanceServer::isInformationServer() const
{
    return true;
}

// update 3D view size when it changes in creator side
void Qt5InformationNodeInstanceServer::update3DViewState(const Update3dViewStateCommand &command)
{
#ifdef QUICK3D_MODULE
    if (command.type() == Update3dViewStateCommand::SizeChange) {
        if (m_editView3DSetupDone) {
            m_editView3DData.rootItem->setSize(command.size());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            m_editView3DData.window->contentItem()->setSize(m_editView3DData.rootItem->size());
            m_editView3DData.window->setGeometry(0, 0, m_editView3DData.rootItem->width(),
                                                 m_editView3DData.rootItem->height());
            m_editView3DData.bufferDirty = true;
#endif
            auto helper = qobject_cast<QmlDesigner::Internal::GeneralHelper *>(m_3dHelper);
            if (helper)
                helper->storeToolState(helper->globalStateId(), helper->rootSizeKey(), QVariant(command.size()), 0);
            // Queue two renders to make sure all gizmos update properly
            render3DEditView(2);
        }
    }
#else
    Q_UNUSED(command)
#endif
}

} // namespace QmlDesigner
