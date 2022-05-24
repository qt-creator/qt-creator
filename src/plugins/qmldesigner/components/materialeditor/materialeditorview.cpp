/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "materialeditorview.h"

#include "materialeditorqmlbackend.h"
#include "materialeditorcontextobject.h"
#include "propertyeditorvalue.h"
#include "materialeditortransaction.h"

#include <qmldesignerconstants.h>
#include <qmltimeline.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <nodeinstanceview.h>
#include <metainfo.h>

#include <rewritingexception.h>
#include <variantproperty.h>

#include <bindingproperty.h>

#include <theme.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <designmodewidget.h>
#include <qmldesignerplugin.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QQuickWidget>
#include <QQuickItem>
#include <QScopedPointer>
#include <QStackedWidget>
#include <QShortcut>
#include <QTimer>

namespace QmlDesigner {

MaterialEditorView::MaterialEditorView(QWidget *parent)
    : AbstractView(parent)
    , m_stackedWidget(new QStackedWidget(parent))
{
    m_updateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F7), m_stackedWidget);
    connect(m_updateShortcut, &QShortcut::activated, this, &MaterialEditorView::reloadQml);

    m_stackedWidget->setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));
    m_stackedWidget->setMinimumWidth(250);
}

void MaterialEditorView::ensureMaterialLibraryNode()
{
    if (!m_hasQuick3DImport)
        return;

    m_materialLibrary = modelNodeForId(Constants::MATERIAL_LIB_ID);
    if (m_materialLibrary.isValid())
        return;

    const QList<ModelNode> materials = rootModelNode().subModelNodesOfType("QtQuick3D.Material");
    if (materials.isEmpty())
        return;

    // create material library node
    TypeName nodeType = rootModelNode().isSubclassOf("QtQuick3D.Node") ? "Quick3D.Node" : "QtQuick.Item";
    NodeMetaInfo metaInfo = model()->metaInfo(nodeType);
    m_materialLibrary = createModelNode(nodeType, metaInfo.majorVersion(), metaInfo.minorVersion());

    m_materialLibrary.setIdWithoutRefactoring(Constants::MATERIAL_LIB_ID);
    rootModelNode().defaultNodeListProperty().reparentHere(m_materialLibrary);

    // move all materials to under material library node
    for (const ModelNode &node : materials) {
        // if material has no name, set name to id
        QString matName = node.variantProperty("objectName").value().toString();
        if (matName.isEmpty()) {
            VariantProperty objNameProp = node.variantProperty("objectName");
            objNameProp.setValue(node.id());
        }

        m_materialLibrary.defaultNodeListProperty().reparentHere(node);
    }
}

MaterialEditorView::~MaterialEditorView()
{
    qDeleteAll(m_qmlBackendHash);
}

// from material editor to model
void MaterialEditorView::changeValue(const QString &name)
{
    PropertyName propertyName = name.toUtf8();

    if (propertyName.isNull() || locked() || noValidSelection() || propertyName == "id"
        || propertyName == Constants::PROPERTY_EDITOR_CLASSNAME_PROPERTY) {
        return;
    }

    if (propertyName == "objectName") {
        renameMaterial(m_selectedMaterial, m_qmlBackEnd->propertyValueForName("objectName")->value().toString());
        return;
    }

    PropertyName underscoreName(propertyName);
    underscoreName.replace('.', '_');
    PropertyEditorValue *value = m_qmlBackEnd->propertyValueForName(QString::fromLatin1(underscoreName));

    if (!value)
        return;

    if (propertyName.endsWith("__AUX")) {
        commitAuxValueToModel(propertyName, value->value());
        return;
    }

    const NodeMetaInfo metaInfo = m_selectedMaterial.metaInfo();

    QVariant castedValue;

    if (metaInfo.isValid() && metaInfo.hasProperty(propertyName)) {
        castedValue = metaInfo.propertyCastedValue(propertyName, value->value());
    } else {
        qWarning() << __FUNCTION__ << propertyName << "cannot be casted (metainfo)";
        return;
    }

    if (value->value().isValid() && !castedValue.isValid()) {
        qWarning() << __FUNCTION__ << propertyName << "not properly casted (metainfo)";
        return;
    }

    bool propertyTypeUrl = false;

    if (metaInfo.isValid() && metaInfo.hasProperty(propertyName)) {
        if (metaInfo.propertyTypeName(propertyName) == "QUrl"
                || metaInfo.propertyTypeName(propertyName) == "url") {
            // turn absolute local file paths into relative paths
            propertyTypeUrl = true;
            QString filePath = castedValue.toUrl().toString();
            QFileInfo fi(filePath);
            if (fi.exists() && fi.isAbsolute()) {
                QDir fileDir(QFileInfo(model()->fileUrl().toLocalFile()).absolutePath());
                castedValue = QUrl(fileDir.relativeFilePath(filePath));
            }
        }
    }

    if (name == "state" && castedValue.toString() == "base state")
        castedValue = "";

    if (castedValue.type() == QVariant::Color) {
        QColor color = castedValue.value<QColor>();
        QColor newColor = QColor(color.name());
        newColor.setAlpha(color.alpha());
        castedValue = QVariant(newColor);
    }

    if (!value->value().isValid() || (propertyTypeUrl && value->value().toString().isEmpty())) { // reset
        removePropertyFromModel(propertyName);
    } else {
        // QVector*D(0, 0, 0) detects as null variant though it is valid value
        if (castedValue.isValid()
            && (!castedValue.isNull() || castedValue.type() == QVariant::Vector2D
                || castedValue.type() == QVariant::Vector3D
                || castedValue.type() == QVariant::Vector4D)) {
            commitVariantValueToModel(propertyName, castedValue);
        }
    }

    requestPreviewRender();
}

static bool isTrueFalseLiteral(const QString &expression)
{
    return (expression.compare("false", Qt::CaseInsensitive) == 0)
           || (expression.compare("true", Qt::CaseInsensitive) == 0);
}

void MaterialEditorView::changeExpression(const QString &propertyName)
{
    PropertyName name = propertyName.toUtf8();

    if (name.isNull() || locked() || noValidSelection())
        return;

    executeInTransaction("MaterialEditorView::changeExpression", [this, name] {
        PropertyName underscoreName(name);
        underscoreName.replace('.', '_');

        QmlObjectNode qmlObjectNode(m_selectedMaterial);
        PropertyEditorValue *value = m_qmlBackEnd->propertyValueForName(QString::fromLatin1(underscoreName));

        if (!value) {
            qWarning() << __FUNCTION__ << "no value for " << underscoreName;
            return;
        }

        if (m_selectedMaterial.metaInfo().isValid() && m_selectedMaterial.metaInfo().hasProperty(name)) {
            if (m_selectedMaterial.metaInfo().propertyTypeName(name) == "QColor") {
                if (QColor(value->expression().remove('"')).isValid()) {
                    qmlObjectNode.setVariantProperty(name, QColor(value->expression().remove('"')));
                    return;
                }
            } else if (m_selectedMaterial.metaInfo().propertyTypeName(name) == "bool") {
                if (isTrueFalseLiteral(value->expression())) {
                    if (value->expression().compare("true", Qt::CaseInsensitive) == 0)
                        qmlObjectNode.setVariantProperty(name, true);
                    else
                        qmlObjectNode.setVariantProperty(name, false);
                    return;
                }
            } else if (m_selectedMaterial.metaInfo().propertyTypeName(name) == "int") {
                bool ok;
                int intValue = value->expression().toInt(&ok);
                if (ok) {
                    qmlObjectNode.setVariantProperty(name, intValue);
                    return;
                }
            } else if (m_selectedMaterial.metaInfo().propertyTypeName(name) == "qreal") {
                bool ok;
                qreal realValue = value->expression().toDouble(&ok);
                if (ok) {
                    qmlObjectNode.setVariantProperty(name, realValue);
                    return;
                }
            } else if (m_selectedMaterial.metaInfo().propertyTypeName(name) == "QVariant") {
                bool ok;
                qreal realValue = value->expression().toDouble(&ok);
                if (ok) {
                    qmlObjectNode.setVariantProperty(name, realValue);
                    return;
                } else if (isTrueFalseLiteral(value->expression())) {
                    if (value->expression().compare("true", Qt::CaseInsensitive) == 0)
                        qmlObjectNode.setVariantProperty(name, true);
                    else
                        qmlObjectNode.setVariantProperty(name, false);
                    return;
                }
            }
        }

        if (value->expression().isEmpty()) {
            value->resetValue();
            return;
        }

        if (qmlObjectNode.expression(name) != value->expression() || !qmlObjectNode.propertyAffectedByCurrentState(name))
            qmlObjectNode.setBindingProperty(name, value->expression());

        requestPreviewRender();
    }); // end of transaction
}

void MaterialEditorView::exportPropertyAsAlias(const QString &name)
{
    if (name.isNull() || locked() || noValidSelection())
        return;

    executeInTransaction("MaterialEditorView::exportPopertyAsAlias", [this, name] {
        const QString id = m_selectedMaterial.validId();
        QString upperCasePropertyName = name;
        upperCasePropertyName.replace(0, 1, upperCasePropertyName.at(0).toUpper());
        QString aliasName = id + upperCasePropertyName;
        aliasName.replace(".", ""); //remove all dots

        PropertyName propertyName = aliasName.toUtf8();
        if (rootModelNode().hasProperty(propertyName)) {
            Core::AsynchronousMessageBox::warning(tr("Cannot Export Property as Alias"),
                                                  tr("Property %1 does already exist for root component.").arg(aliasName));
            return;
        }
        rootModelNode().bindingProperty(propertyName).setDynamicTypeNameAndExpression("alias", id + "." + name);
    });
}

void MaterialEditorView::removeAliasExport(const QString &name)
{
    if (name.isNull() || locked() || noValidSelection())
        return;

    executeInTransaction("MaterialEditorView::removeAliasExport", [this, name] {
        const QString id = m_selectedMaterial.validId();

        const QList<BindingProperty> bindingProps = rootModelNode().bindingProperties();
        for (const BindingProperty &property : bindingProps) {
            if (property.expression() == (id + "." + name)) {
                rootModelNode().removeProperty(property.name());
                break;
            }
        }
    });
}

bool MaterialEditorView::locked() const
{
    return m_locked;
}

void MaterialEditorView::currentTimelineChanged(const ModelNode &)
{
    m_qmlBackEnd->contextObject()->setHasActiveTimeline(QmlTimeline::hasActiveTimeline(this));
}

void MaterialEditorView::delayedResetView()
{
    // TODO: it seems the delayed reset is not needed. Leaving it commented out for now just in case it
    // turned out to be needed. Otherwise will be removed after a small testing period.
//    if (m_timerId)
//        killTimer(m_timerId);
//    m_timerId = startTimer(50);
    resetView();
}

void MaterialEditorView::timerEvent(QTimerEvent *timerEvent)
{
    if (m_timerId == timerEvent->timerId())
        resetView();
}

void MaterialEditorView::resetView()
{
    if (!model())
        return;

    m_locked = true;

    if (m_timerId)
        killTimer(m_timerId);

    setupQmlBackend();

    if (m_qmlBackEnd)
        m_qmlBackEnd->emitSelectionChanged();

    QTimer::singleShot(0, this, &MaterialEditorView::requestPreviewRender);

    m_locked = false;

    if (m_timerId)
        m_timerId = 0;
}

// static
QString MaterialEditorView::materialEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/materialEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/materialEditorQmlSources").toString();
}

void MaterialEditorView::applyMaterialToSelectedModels(const ModelNode &material, bool add)
{
    if (m_selectedModels.isEmpty())
        return;

    QTC_ASSERT(material.isValid(), return);

    auto expToList = [](const QString &exp) {
        QString copy = exp;
        copy = copy.remove("[").remove("]");

        QStringList tmp = copy.split(',', Qt::SkipEmptyParts);
        for (QString &str : tmp)
            str = str.trimmed();

        return tmp;
    };

    auto listToExp = [](QStringList &stringList) {
        if (stringList.size() > 1)
            return QString("[" + stringList.join(",") + "]");

        if (stringList.size() == 1)
            return stringList.first();

        return QString();
    };

    executeInTransaction("MaterialEditorView::applyMaterialToSelectedModels", [&] {
        for (const ModelNode &node : std::as_const(m_selectedModels)) {
            QmlObjectNode qmlObjNode(node);
            if (add) {
                QStringList matList = expToList(qmlObjNode.expression("materials"));
                matList.append(material.id());
                QString updatedExp = listToExp(matList);
                qmlObjNode.setBindingProperty("materials", updatedExp);
            } else {
                qmlObjNode.setBindingProperty("materials", material.id());
            }
        }
    });
}

void MaterialEditorView::handleToolBarAction(int action)
{
    QTC_ASSERT(m_hasQuick3DImport, return);

    switch (action) {
    case MaterialEditorContextObject::ApplyToSelected: {
        applyMaterialToSelectedModels(m_selectedMaterial);
        break;
    }

    case MaterialEditorContextObject::ApplyToSelectedAdd: {
        applyMaterialToSelectedModels(m_selectedMaterial, true);
        break;
    }

    case MaterialEditorContextObject::AddNewMaterial: {
        ensureMaterialLibraryNode();

        executeInTransaction("MaterialEditorView:handleToolBarAction", [&] {
            NodeMetaInfo metaInfo = model()->metaInfo("QtQuick3D.DefaultMaterial");
            ModelNode newMatNode = createModelNode("QtQuick3D.DefaultMaterial", metaInfo.majorVersion(),
                                                                                metaInfo.minorVersion());
            renameMaterial(newMatNode, "New Material");

            m_materialLibrary.defaultNodeListProperty().reparentHere(newMatNode);
        });
        break;
    }

    case MaterialEditorContextObject::DeleteCurrentMaterial: {
        if (m_selectedMaterial.isValid())
            m_selectedMaterial.destroy();
        break;
    }

    case MaterialEditorContextObject::OpenMaterialBrowser: {
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialBrowser", true);
        break;
    }
    }
}

void MaterialEditorView::setupQmlBackend()
{
    QUrl qmlPaneUrl;
    QUrl qmlSpecificsUrl;

    if (m_selectedMaterial.isValid() && m_hasQuick3DImport) {
        qmlPaneUrl = QUrl::fromLocalFile(materialEditorResourcesPath() + "/MaterialEditorPane.qml");

        NodeMetaInfo metaInfo = m_selectedMaterial.metaInfo();
        QDir importDir(metaInfo.importDirectoryPath() + Constants::QML_DESIGNER_SUBFOLDER);
        QString typeName = QString::fromUtf8(metaInfo.typeName().split('.').constLast());
        qmlSpecificsUrl = QUrl::fromLocalFile(importDir.absoluteFilePath(typeName + "Specifics.qml"));
    } else {
        qmlPaneUrl = QUrl::fromLocalFile(materialEditorResourcesPath() + "/EmptyMaterialEditorPane.qml");
    }

    MaterialEditorQmlBackend *currentQmlBackend = m_qmlBackendHash.value(qmlPaneUrl.toString());

    QString currentStateName = currentState().isBaseState() ? currentState().name() : "invalid state";

    if (!currentQmlBackend) {
        currentQmlBackend = new MaterialEditorQmlBackend(this);

        m_stackedWidget->addWidget(currentQmlBackend->widget());
        m_qmlBackendHash.insert(qmlPaneUrl.toString(), currentQmlBackend);

        currentQmlBackend->setup(m_selectedMaterial, currentStateName, qmlSpecificsUrl, this);

        currentQmlBackend->setSource(qmlPaneUrl);

        QObject::connect(currentQmlBackend->widget()->rootObject(), SIGNAL(toolBarAction(int)),
                         this, SLOT(handleToolBarAction(int)));
    } else {
        currentQmlBackend->setup(m_selectedMaterial, currentStateName, qmlSpecificsUrl, this);
    }

    currentQmlBackend->contextObject()->setHasQuick3DImport(m_hasQuick3DImport);

    m_stackedWidget->setCurrentWidget(currentQmlBackend->widget());

    m_qmlBackEnd = currentQmlBackend;
}

void MaterialEditorView::commitVariantValueToModel(const PropertyName &propertyName, const QVariant &value)
{
    m_locked = true;
    executeInTransaction("MaterialEditorView:commitVariantValueToModel", [&] {
        QmlObjectNode(m_selectedMaterial).setVariantProperty(propertyName, value);
    });
    m_locked = false;
}

void MaterialEditorView::commitAuxValueToModel(const PropertyName &propertyName, const QVariant &value)
{
    m_locked = true;

    PropertyName name = propertyName;
    name.chop(5);

    try {
        if (value.isValid())
            m_selectedMaterial.setAuxiliaryData(name, value);
        else
            m_selectedMaterial.removeAuxiliaryData(name);
    }
    catch (const Exception &e) {
        e.showException();
    }
    m_locked = false;
}

void MaterialEditorView::removePropertyFromModel(const PropertyName &propertyName)
{
    m_locked = true;
    executeInTransaction("MaterialEditorView:removePropertyFromModel", [&] {
        QmlObjectNode(m_selectedMaterial).removeProperty(propertyName);
    });
    m_locked = false;
}

bool MaterialEditorView::noValidSelection() const
{
    QTC_ASSERT(m_qmlBackEnd, return true);
    return !QmlObjectNode::isValidQmlObjectNode(m_selectedMaterial);
}

void MaterialEditorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_locked = true;

    m_hasQuick3DImport = model->hasImport("QtQuick3D");

    ensureMaterialLibraryNode();

    if (!m_setupCompleted) {
        reloadQml();
        m_setupCompleted = true;
    }
    resetView();

    m_locked = false;
}

void MaterialEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    m_qmlBackEnd->materialEditorTransaction()->end();


}

void MaterialEditorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    if (noValidSelection())
        return;

    for (const AbstractProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());

        if (node.isRootNode())
            m_qmlBackEnd->contextObject()->setHasAliasExport(QmlObjectNode(m_selectedMaterial).isAliasExported());

        if (node == m_selectedMaterial || QmlObjectNode(m_selectedMaterial).propertyChangeForCurrentState() == node) {
            setValue(m_selectedMaterial, property.name(), QmlObjectNode(m_selectedMaterial).instanceValue(property.name()));
        }
    }
}

void MaterialEditorView::variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags /*propertyChange*/)
{
    if (noValidSelection())
        return;

    bool changed = false;
    for (const VariantProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());

        if (node == m_selectedMaterial || QmlObjectNode(m_selectedMaterial).propertyChangeForCurrentState() == node) {
            if (m_selectedMaterial.property(property.name()).isBindingProperty())
                setValue(m_selectedMaterial, property.name(), QmlObjectNode(m_selectedMaterial).instanceValue(property.name()));
            else
                setValue(m_selectedMaterial, property.name(), QmlObjectNode(m_selectedMaterial).modelValue(property.name()));

            changed = true;
        }
    }
    if (changed)
        requestPreviewRender();
}

void MaterialEditorView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags /*propertyChange*/)
{
    if (noValidSelection())
        return;

    bool changed = false;
    for (const BindingProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());

        if (property.isAliasExport())
            m_qmlBackEnd->contextObject()->setHasAliasExport(QmlObjectNode(m_selectedMaterial).isAliasExported());

        if (node == m_selectedMaterial || QmlObjectNode(m_selectedMaterial).propertyChangeForCurrentState() == node) {
            if (QmlObjectNode(m_selectedMaterial).modelNode().property(property.name()).isBindingProperty())
                setValue(m_selectedMaterial, property.name(), QmlObjectNode(m_selectedMaterial).instanceValue(property.name()));
            else
                setValue(m_selectedMaterial, property.name(), QmlObjectNode(m_selectedMaterial).modelValue(property.name()));

            changed = true;
        }
    }
    if (changed)
        requestPreviewRender();
}

void MaterialEditorView::auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &)
{

    if (noValidSelection() || !node.isSelected())
        return;

    m_qmlBackEnd->setValueforAuxiliaryProperties(m_selectedMaterial, name);
}

// request render image for the selected material node
void MaterialEditorView::requestPreviewRender()
{
    if (m_selectedMaterial.isValid())
        model()->nodeInstanceView()->previewImageDataForGenericNode(m_selectedMaterial, {});
}

bool MaterialEditorView::hasWidget() const
{
    return true;
}

WidgetInfo MaterialEditorView::widgetInfo()
{
    return createWidgetInfo(m_stackedWidget, nullptr, "MaterialEditor", WidgetInfo::RightPane, 0, tr("Material Editor"));
}

void MaterialEditorView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                              const QList<ModelNode> &lastSelectedNodeList)
{
    m_selectedModels.clear();

    for (const ModelNode &node : selectedNodeList) {
        if (node.isSubclassOf("QtQuick3D.Model"))
            m_selectedModels.append(node);
    }

    m_qmlBackEnd->contextObject()->setHasModelSelection(!m_selectedModels.isEmpty());
}

void MaterialEditorView::currentStateChanged(const ModelNode &node)
{
    QmlModelState newQmlModelState(node);
    Q_ASSERT(newQmlModelState.isValid());
    delayedResetView();
}

void MaterialEditorView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    if (!m_selectedMaterial.isValid() || !m_qmlBackEnd)
        return;

    m_locked = true;

    for (const QPair<ModelNode, PropertyName> &propertyPair : propertyList) {
        const ModelNode modelNode = propertyPair.first;
        const QmlObjectNode qmlObjectNode(modelNode);
        const PropertyName propertyName = propertyPair.second;

        if (qmlObjectNode.isValid() && modelNode == m_selectedMaterial && qmlObjectNode.currentState().isValid()) {
            const AbstractProperty property = modelNode.property(propertyName);
            if (!modelNode.hasProperty(propertyName) || modelNode.property(property.name()).isBindingProperty())
                setValue(modelNode, property.name(), qmlObjectNode.instanceValue(property.name()));
            else
                setValue(modelNode, property.name(), qmlObjectNode.modelValue(property.name()));
        }
    }

    m_locked = false;
}

void MaterialEditorView::nodeTypeChanged(const ModelNode &node, const TypeName &, int, int)
{
     if (node == m_selectedMaterial)
         delayedResetView();
}

void MaterialEditorView::modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap)
{
    if (node == m_selectedMaterial)
        m_qmlBackEnd->updateMaterialPreview(pixmap);
}

void MaterialEditorView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    m_hasQuick3DImport = model()->hasImport("QtQuick3D");
    m_qmlBackEnd->contextObject()->setHasQuick3DImport(m_hasQuick3DImport);

    ensureMaterialLibraryNode(); // create the material lib if Quick3D import is added
    resetView();
}

void MaterialEditorView::renameMaterial(ModelNode &material, const QString &newName)
{
    QTC_ASSERT(material.isValid(), return);

    executeInTransaction("MaterialEditorView:renameMaterial", [&] {
        material.setIdWithRefactoring(generateIdFromName(newName));

        VariantProperty objNameProp = material.variantProperty("objectName");
        objNameProp.setValue(newName);
    });
}

void MaterialEditorView::customNotification(const AbstractView *view, const QString &identifier,
                                            const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    if (identifier == "selected_material_changed") {
        m_selectedMaterial = nodeList.first();
        QTimer::singleShot(0, this, &MaterialEditorView::resetView);
    } else if (identifier == "apply_to_selected_triggered") {
        applyMaterialToSelectedModels(nodeList.first(), data.first().toBool());
    } else if (identifier == "rename_material") {
        if (m_selectedMaterial == nodeList.first())
            renameMaterial(m_selectedMaterial, data.first().toString());
    } else if (identifier == "add_new_material") {
        handleToolBarAction(MaterialEditorContextObject::AddNewMaterial);
    }
}

// from model to material editor
void MaterialEditorView::setValue(const QmlObjectNode &qmlObjectNode, const PropertyName &name, const QVariant &value)
{
    m_locked = true;
    m_qmlBackEnd->setValue(qmlObjectNode, name, value);
    requestPreviewRender();
    m_locked = false;
}

void MaterialEditorView::reloadQml()
{
    m_qmlBackendHash.clear();
    while (QWidget *widget = m_stackedWidget->widget(0)) {
        m_stackedWidget->removeWidget(widget);
        delete widget;
    }
    m_qmlBackEnd = nullptr;

    resetView();
}

// generate a unique camelCase id from a name
QString MaterialEditorView::generateIdFromName(const QString &name)
{
    QString newId;
    if (name.isEmpty()) {
        newId = "material";
    } else {
        // convert to camel case
        QStringList nameWords = name.split(" ");
        nameWords[0] = nameWords[0].at(0).toLower() + nameWords[0].mid(1);
        for (int i = 1; i < nameWords.size(); ++i)
            nameWords[i] = nameWords[i].at(0).toUpper() + nameWords[i].mid(1);
        newId = nameWords.join("");

        // if id starts with a number prepend an underscore
        if (newId.at(0).isDigit())
            newId.prepend('_');
    }

    QRegularExpression rgx("\\d+$"); // matches a number at the end of a string
    while (hasId(newId)) { // id exists
        QRegularExpressionMatch match = rgx.match(newId);
        if (match.hasMatch()) { // ends with a number, increment it
            QString numStr = match.captured();
            int num = numStr.toInt() + 1;
            newId = newId.mid(0, match.capturedStart()) + QString::number(num);
        } else {
            newId.append('1');
        }
    }

    return newId;
}

} // namespace QmlDesigner
