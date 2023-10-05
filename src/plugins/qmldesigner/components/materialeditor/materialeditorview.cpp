// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "materialeditorview.h"

#include "asset.h"
#include "bindingproperty.h"
#include "auxiliarydataproperties.h"
#include "designdocument.h"
#include "designmodewidget.h"
#include "dynamicpropertiesmodel.h"
#include "externaldependenciesinterface.h"
#include "itemlibraryinfo.h"
#include "materialeditorqmlbackend.h"
#include "materialeditorcontextobject.h"
#include "materialeditordynamicpropertiesproxymodel.h"
#include "materialeditortransaction.h"
#include "metainfo.h"
#include "nodeinstanceview.h"
#include "nodelistproperty.h"
#include "nodemetainfo.h"
#include "propertyeditorqmlbackend.h"
#include "propertyeditorvalue.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "qmltimeline.h"
#include "variantproperty.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QQuickItem>
#include <QStackedWidget>
#include <QShortcut>
#include <QColorDialog>

namespace {
QSize maxSize(const std::initializer_list<QSize> &sizeList)
{
    QSize result;
    for (const QSize &size : sizeList) {
        if (size.width() > result.width())
            result.setWidth(size.width());
        if (size.height() > result.height())
            result.setHeight(size.height());
    }
    return result;
}
}

namespace QmlDesigner {

MaterialEditorView::MaterialEditorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_stackedWidget(new QStackedWidget)
    , m_dynamicPropertiesModel(new DynamicPropertiesModel(true, this))
{
    m_updateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F7), m_stackedWidget);
    connect(m_updateShortcut, &QShortcut::activated, this, &MaterialEditorView::reloadQml);

    m_ensureMatLibTimer.callOnTimeout([this] {
        if (model() && model()->rewriterView() && !model()->rewriterView()->hasIncompleteTypeInformation()
            && model()->rewriterView()->errors().isEmpty()) {
            DesignDocument *doc = QmlDesignerPlugin::instance()->currentDesignDocument();
            if (doc && !doc->inFileComponentModelActive())
                ensureMaterialLibraryNode();
            if (m_qmlBackEnd && m_qmlBackEnd->contextObject())
                m_qmlBackEnd->contextObject()->setHasMaterialLibrary(materialLibraryNode().isValid());
            m_ensureMatLibTimer.stop();
        }
    });

    m_typeUpdateTimer.setSingleShot(true);
    m_typeUpdateTimer.setInterval(500);
    connect(&m_typeUpdateTimer, &QTimer::timeout, this, &MaterialEditorView::updatePossibleTypes);

    QmlDesignerPlugin::trackWidgetFocusTime(m_stackedWidget, Constants::EVENT_MATERIALEDITOR_TIME);

    MaterialEditorDynamicPropertiesProxyModel::registerDeclarativeType();
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

    if (auto property = metaInfo.property(propertyName)) {
        castedValue = property.castedValue(value->value());
    } else {
        qWarning() << __FUNCTION__ << propertyName << "cannot be casted (metainfo)";
        return;
    }

    if (value->value().isValid() && !castedValue.isValid()) {
        qWarning() << __FUNCTION__ << propertyName << "not properly casted (metainfo)";
        return;
    }

    bool propertyTypeUrl = false;

    if (auto property = metaInfo.property(propertyName)) {
        if (property.propertyType().isUrl()) {
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

    if (castedValue.typeId() == QVariant::Color) {
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
            && (!castedValue.isNull() || castedValue.typeId() == QVariant::Vector2D
                || castedValue.typeId() == QVariant::Vector3D
                || castedValue.typeId() == QVariant::Vector4D)) {
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

    executeInTransaction(__FUNCTION__, [this, name] {
        PropertyName underscoreName(name);
        underscoreName.replace('.', '_');

        QmlObjectNode qmlObjectNode(m_selectedMaterial);
        PropertyEditorValue *value = m_qmlBackEnd->propertyValueForName(QString::fromLatin1(underscoreName));

        if (!value) {
            qWarning() << __FUNCTION__ << "no value for " << underscoreName;
            return;
        }

        if (auto property = m_selectedMaterial.metaInfo().property(name)) {
            auto propertyType = property.propertyType();
            if (propertyType.isColor()) {
                if (QColor(value->expression().remove('"')).isValid()) {
                    qmlObjectNode.setVariantProperty(name, QColor(value->expression().remove('"')));
                    return;
                }
            } else if (propertyType.isBool()) {
                if (isTrueFalseLiteral(value->expression())) {
                    if (value->expression().compare("true", Qt::CaseInsensitive) == 0)
                        qmlObjectNode.setVariantProperty(name, true);
                    else
                        qmlObjectNode.setVariantProperty(name, false);
                    return;
                }
            } else if (propertyType.isInteger()) {
                bool ok;
                int intValue = value->expression().toInt(&ok);
                if (ok) {
                    qmlObjectNode.setVariantProperty(name, intValue);
                    return;
                }
            } else if (propertyType.isFloat()) {
                bool ok;
                qreal realValue = value->expression().toDouble(&ok);
                if (ok) {
                    qmlObjectNode.setVariantProperty(name, realValue);
                    return;
                }
            } else if (propertyType.isVariant()) {
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

    executeInTransaction(__FUNCTION__, [this, name] {
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

    executeInTransaction(__FUNCTION__, [this, name] {
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

DynamicPropertiesModel *MaterialEditorView::dynamicPropertiesModel() const
{
    return m_dynamicPropertiesModel;
}

MaterialEditorView *MaterialEditorView::instance()
{
    static MaterialEditorView *s_instance = nullptr;

    if (s_instance)
        return s_instance;

    const auto views = QmlDesignerPlugin::instance()->viewManager().views();
    for (auto *view : views) {
        MaterialEditorView *myView = qobject_cast<MaterialEditorView *>(view);
        if (myView)
            s_instance =  myView;
    }

    QTC_ASSERT(s_instance, return nullptr);
    return s_instance;
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
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
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

    executeInTransaction(__FUNCTION__, [&] {
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
        if (!model())
            break;
        executeInTransaction(__FUNCTION__, [&] {
            ModelNode matLib = materialLibraryNode();
            if (!matLib.isValid())
                return;

            NodeMetaInfo metaInfo = model()->qtQuick3DPrincipledMaterialMetaInfo();
            ModelNode newMatNode = createModelNode("QtQuick3D.PrincipledMaterial",
                                                   metaInfo.majorVersion(),
                                                   metaInfo.minorVersion());
            renameMaterial(newMatNode, "New Material");
            matLib.defaultNodeListProperty().reparentHere(newMatNode);
        });
        break;
    }

    case MaterialEditorContextObject::DeleteCurrentMaterial: {
        if (m_selectedMaterial.isValid()) {
            executeInTransaction(__FUNCTION__, [&] {
                m_selectedMaterial.destroy();
            });
        }
        break;
    }

    case MaterialEditorContextObject::OpenMaterialBrowser: {
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialBrowser", true);
        break;
    }
    }
}

void MaterialEditorView::handlePreviewEnvChanged(const QString &envAndValue)
{
    if (envAndValue.isEmpty() || m_initializingPreviewData)
        return;

    QTC_ASSERT(m_hasQuick3DImport, return);
    QTC_ASSERT(model(), return);
    QTC_ASSERT(model()->nodeInstanceView(), return);

    QStringList parts = envAndValue.split('=');
    QString env = parts[0];
    QString value;
    if (parts.size() > 1)
        value = parts[1];

    auto renderPreviews = [=](const QString &auxEnv, const QString &auxValue) {
        rootModelNode().setAuxiliaryData(materialPreviewEnvDocProperty, auxEnv);
        rootModelNode().setAuxiliaryData(materialPreviewEnvProperty, auxEnv);
        rootModelNode().setAuxiliaryData(materialPreviewEnvValueDocProperty, auxValue);
        rootModelNode().setAuxiliaryData(materialPreviewEnvValueProperty, auxValue);
        QTimer::singleShot(0, this, &MaterialEditorView::requestPreviewRender);
        emitCustomNotification("refresh_material_browser", {});
    };

    if (env == "Color") {
        m_colorDialog.clear();

        // Store color to separate property to persist selection over non-color env changes
        auto oldColorPropVal = rootModelNode().auxiliaryData(materialPreviewColorDocProperty);
        auto oldEnvPropVal = rootModelNode().auxiliaryData(materialPreviewEnvDocProperty);
        auto oldValuePropVal = rootModelNode().auxiliaryData(materialPreviewEnvValueDocProperty);
        QString oldColor = oldColorPropVal ? oldColorPropVal->toString() : "";
        QString oldEnv = oldEnvPropVal ? oldEnvPropVal->toString() : "";
        QString oldValue = oldValuePropVal ? oldValuePropVal->toString() : "";

        m_colorDialog = new QColorDialog(Core::ICore::dialogParent());
        m_colorDialog->setModal(true);
        m_colorDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_colorDialog->setCurrentColor(QColor(oldColor));
        m_colorDialog->show();

        QObject::connect(m_colorDialog, &QColorDialog::currentColorChanged,
                         m_colorDialog, [=](const QColor &color) {
            renderPreviews(env, color.name());
        });

        QObject::connect(m_colorDialog, &QColorDialog::colorSelected,
                         m_colorDialog, [=](const QColor &color) {
            renderPreviews(env, color.name());
            rootModelNode().setAuxiliaryData(materialPreviewColorDocProperty, color.name());
        });

        QObject::connect(m_colorDialog, &QColorDialog::rejected,
                         m_colorDialog, [=]() {
            renderPreviews(oldEnv, oldValue);
            initPreviewData();
        });
        return;
    }
    renderPreviews(env, value);
}

void MaterialEditorView::handlePreviewModelChanged(const QString &modelStr)
{
    if (modelStr.isEmpty() || m_initializingPreviewData)
        return;

    QTC_ASSERT(m_hasQuick3DImport, return);
    QTC_ASSERT(model(), return);
    QTC_ASSERT(model()->nodeInstanceView(), return);

    rootModelNode().setAuxiliaryData(materialPreviewModelDocProperty, modelStr);
    rootModelNode().setAuxiliaryData(materialPreviewModelProperty, modelStr);

    QTimer::singleShot(0, this, &MaterialEditorView::requestPreviewRender);
    emitCustomNotification("refresh_material_browser", {});
}

void MaterialEditorView::setupQmlBackend()
{
    QUrl qmlPaneUrl;
    QUrl qmlSpecificsUrl;
    QString specificQmlData;
    QString currentTypeName;

    if (m_selectedMaterial.isValid() && m_hasQuick3DImport && (materialLibraryNode().isValid() || m_hasMaterialRoot)) {
        qmlPaneUrl = QUrl::fromLocalFile(materialEditorResourcesPath() + "/MaterialEditorPane.qml");

        TypeName diffClassName;
        if (NodeMetaInfo metaInfo = m_selectedMaterial.metaInfo()) {
            diffClassName = metaInfo.typeName();
            for (const NodeMetaInfo &metaInfo : metaInfo.selfAndPrototypes()) {
                if (PropertyEditorQmlBackend::checkIfUrlExists(qmlSpecificsUrl))
                    break;
                qmlSpecificsUrl = PropertyEditorQmlBackend::getQmlFileUrl(metaInfo.typeName()
                                                                          + "Specifics", metaInfo);
                diffClassName = metaInfo.typeName();
            }

            if (diffClassName != m_selectedMaterial.type()) {
                specificQmlData = PropertyEditorQmlBackend::templateGeneration(metaInfo,
                                                                               model()->metaInfo(
                                                                                   diffClassName),
                                                                               m_selectedMaterial);
            }
        }
        currentTypeName = QString::fromLatin1(m_selectedMaterial.type());
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

        QObject *rootObj = currentQmlBackend->widget()->rootObject();
        QObject::connect(rootObj, SIGNAL(toolBarAction(int)),
                         this, SLOT(handleToolBarAction(int)));
        QObject::connect(rootObj, SIGNAL(previewEnvChanged(QString)),
                         this, SLOT(handlePreviewEnvChanged(QString)));
        QObject::connect(rootObj, SIGNAL(previewModelChanged(QString)),
                         this, SLOT(handlePreviewModelChanged(QString)));
    } else {
        currentQmlBackend->setup(m_selectedMaterial, currentStateName, qmlSpecificsUrl, this);
    }

    currentQmlBackend->widget()->installEventFilter(this);
    currentQmlBackend->contextObject()->setHasQuick3DImport(m_hasQuick3DImport);
    currentQmlBackend->contextObject()->setHasMaterialLibrary(materialLibraryNode().isValid());
    currentQmlBackend->contextObject()->setSpecificQmlData(specificQmlData);
    currentQmlBackend->contextObject()->setCurrentType(currentTypeName);
    currentQmlBackend->contextObject()->setIsQt6Project(externalDependencies().isQt6Project());

    m_qmlBackEnd = currentQmlBackend;

    if (m_hasMaterialRoot)
        m_dynamicPropertiesModel->setSelectedNode(m_selectedMaterial);
    else
        m_dynamicPropertiesModel->reset();

    delayedTypeUpdate();
    initPreviewData();

    m_stackedWidget->setCurrentWidget(m_qmlBackEnd->widget());
    if (m_qmlBackEnd->widget()) {
        m_stackedWidget->setMinimumSize(maxSize({m_qmlBackEnd->widget()->sizeHint(),
                                                 m_qmlBackEnd->widget()->initialSize(),
                                                 m_qmlBackEnd->widget()->minimumSizeHint(),
                                                 m_qmlBackEnd->widget()->minimumSize()}));
    } else {
        m_stackedWidget->setMinimumSize({400, 300});
    }
}

void MaterialEditorView::commitVariantValueToModel(const PropertyName &propertyName, const QVariant &value)
{
    m_locked = true;
    executeInTransaction(__FUNCTION__, [&] {
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
            m_selectedMaterial.setAuxiliaryData(AuxiliaryDataType::Document, name, value);
        else
            m_selectedMaterial.removeAuxiliaryData(AuxiliaryDataType::Document, name);
    }
    catch (const Exception &e) {
        e.showException();
    }
    m_locked = false;
}

void MaterialEditorView::removePropertyFromModel(const PropertyName &propertyName)
{
    m_locked = true;
    executeInTransaction(__FUNCTION__, [&] {
        QmlObjectNode(m_selectedMaterial).removeProperty(propertyName);
    });
    m_locked = false;
}

bool MaterialEditorView::noValidSelection() const
{
    QTC_ASSERT(m_qmlBackEnd, return true);
    return !QmlObjectNode::isValidQmlObjectNode(m_selectedMaterial);
}

void MaterialEditorView::initPreviewData()
{
    if (model() && m_qmlBackEnd) {
        auto envPropVal = rootModelNode().auxiliaryData(materialPreviewEnvDocProperty);
        auto envValuePropVal = rootModelNode().auxiliaryData(materialPreviewEnvValueDocProperty);
        auto modelStrPropVal = rootModelNode().auxiliaryData(materialPreviewModelDocProperty);
        QString env = envPropVal ? envPropVal->toString() : "";
        QString envValue = envValuePropVal ? envValuePropVal->toString() : "";
        QString modelStr = modelStrPropVal ? modelStrPropVal->toString() : "";
        // Initialize corresponding instance aux values used by puppet
        QTimer::singleShot(0, this, [this, env, envValue, modelStr]() {
            if (model()) {
                rootModelNode().setAuxiliaryData(materialPreviewEnvProperty, env);
                rootModelNode().setAuxiliaryData(materialPreviewEnvValueProperty, envValue);
                rootModelNode().setAuxiliaryData(materialPreviewModelProperty, modelStr);
            }
        });

        if (!envValue.isEmpty() && env != "Color" && env != "Basic") {
            env += '=';
            env += envValue;
        }
        if (env.isEmpty())
            env = "SkyBox=preview_studio";
        if (modelStr.isEmpty())
            modelStr = "#Sphere";
        m_initializingPreviewData = true;
        QMetaObject::invokeMethod(m_qmlBackEnd->widget()->rootObject(),
                                  "initPreviewData",
                                  Q_ARG(QVariant, env), Q_ARG(QVariant, modelStr));
        m_initializingPreviewData = false;
    }
}

void MaterialEditorView::delayedTypeUpdate()
{
     m_typeUpdateTimer.start();
}

static Import entryToImport(const ItemLibraryEntry &entry)
{
    if (entry.majorVersion() == -1 && entry.minorVersion() == -1)
        return Import::createFileImport(entry.requiredImport());

    return Import::createLibraryImport(entry.requiredImport(),
                                       QString::number(entry.majorVersion()) + QLatin1Char('.') +
                                       QString::number(entry.minorVersion()));
}

void MaterialEditorView::updatePossibleTypes()
{
    QTC_ASSERT(model(), return);

    if (!m_qmlBackEnd)
        return;

    // Ensure basic types are always first
    QStringList nonQuick3dTypes;
    QStringList allTypes;

    const QList<ItemLibraryEntry> itemLibEntries = m_itemLibraryInfo->entries();
    for (const ItemLibraryEntry &entry : itemLibEntries) {
        NodeMetaInfo metaInfo = model()->metaInfo(entry.typeName());
        bool valid = metaInfo.isValid()
                     && (metaInfo.majorVersion() >= entry.majorVersion()
                         || metaInfo.majorVersion() < 0);
        if (valid && metaInfo.isQtQuick3DMaterial()) {
            bool addImport = entry.requiredImport().isEmpty();
            if (!addImport) {
                Import import = entryToImport(entry);
                addImport = model()->hasImport(import, true, true);
            }
            if (addImport) {
                const QList<QByteArray> typeSplit = entry.typeName().split('.');
                const QString typeName = QString::fromLatin1(typeSplit.last());
                if (typeSplit.size() == 2 && typeSplit.first() == "QtQuick3D") {
                    if (!allTypes.contains(typeName))
                        allTypes.append(typeName);
                } else if (!nonQuick3dTypes.contains(typeName)) {
                    nonQuick3dTypes.append(typeName);
                }
            }
        }
    }

    allTypes.sort();
    nonQuick3dTypes.sort();
    allTypes.append(nonQuick3dTypes);

    m_qmlBackEnd->contextObject()->setPossibleTypes(allTypes);
}

void MaterialEditorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_locked = true;

    m_hasQuick3DImport = model->hasImport("QtQuick3D");
    m_hasMaterialRoot = rootModelNode().metaInfo().isQtQuick3DMaterial();

    if (m_hasMaterialRoot) {
        m_selectedMaterial = rootModelNode();
    } else if (m_hasQuick3DImport) {
        // Creating the material library node on model attach causes errors as long as the type
        // information is not complete yet, so we keep checking until type info is complete.
        m_ensureMatLibTimer.start(500);
    }

    if (m_itemLibraryInfo.data() != model->metaInfo().itemLibraryInfo()) {
        if (m_itemLibraryInfo) {
            disconnect(m_itemLibraryInfo.data(), &ItemLibraryInfo::entriesChanged,
                       this, &MaterialEditorView::delayedTypeUpdate);
        }
        m_itemLibraryInfo = model->metaInfo().itemLibraryInfo();
        if (m_itemLibraryInfo) {
            connect(m_itemLibraryInfo.data(), &ItemLibraryInfo::entriesChanged,
                    this, &MaterialEditorView::delayedTypeUpdate);
        }
    }

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
    m_dynamicPropertiesModel->reset();
    m_qmlBackEnd->materialEditorTransaction()->end();
    m_qmlBackEnd->contextObject()->setHasMaterialLibrary(false);
}

void MaterialEditorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    if (noValidSelection())
        return;

    bool changed = false;
    for (const AbstractProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());

        if (node.isRootNode())
            m_qmlBackEnd->contextObject()->setHasAliasExport(QmlObjectNode(m_selectedMaterial).isAliasExported());

        if (node == m_selectedMaterial || QmlObjectNode(m_selectedMaterial).propertyChangeForCurrentState() == node) {
            setValue(m_selectedMaterial, property.name(), QmlObjectNode(m_selectedMaterial).instanceValue(property.name()));
            changed = true;
        }

        dynamicPropertiesModel()->dispatchPropertyChanges(property);
    }
    if (changed)
        requestPreviewRender();
}

void MaterialEditorView::variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags /*propertyChange*/)
{
    if (noValidSelection())
        return;

    bool changed = false;
    for (const VariantProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());
        if (node == m_selectedMaterial || QmlObjectNode(m_selectedMaterial).propertyChangeForCurrentState() == node) {
            if (property.isDynamic())
                m_dynamicPropertiesModel->updateItem(property);
            if (m_selectedMaterial.property(property.name()).isBindingProperty())
                setValue(m_selectedMaterial, property.name(), QmlObjectNode(m_selectedMaterial).instanceValue(property.name()));
            else
                setValue(m_selectedMaterial, property.name(), QmlObjectNode(m_selectedMaterial).modelValue(property.name()));

            changed = true;
        }

        if (!changed && node.metaInfo().isQtQuick3DTexture()
            && m_selectedMaterial.bindingProperties().size() > 0) {
            // update preview when editing texture properties if the material has binding properties
            changed = true;
        }

        dynamicPropertiesModel()->dispatchPropertyChanges(property);
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
            if (property.isDynamic())
                m_dynamicPropertiesModel->updateItem(property);
            if (QmlObjectNode(m_selectedMaterial).modelNode().property(property.name()).isBindingProperty())
                setValue(m_selectedMaterial, property.name(), QmlObjectNode(m_selectedMaterial).instanceValue(property.name()));
            else
                setValue(m_selectedMaterial, property.name(), QmlObjectNode(m_selectedMaterial).modelValue(property.name()));

            changed = true;
        }

        dynamicPropertiesModel()->dispatchPropertyChanges(property);
    }
    if (changed)
        requestPreviewRender();
}

void MaterialEditorView::auxiliaryDataChanged(const ModelNode &node,
                                              AuxiliaryDataKeyView key,
                                              const QVariant &)
{

    if (noValidSelection() || !node.isSelected())
        return;

    m_qmlBackEnd->setValueforAuxiliaryProperties(m_selectedMaterial, key);
}

void MaterialEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const auto &property : propertyList)
        m_dynamicPropertiesModel->removeItem(property);
}

// request render image for the selected material node
void MaterialEditorView::requestPreviewRender()
{
    if (model() && model()->nodeInstanceView() && m_selectedMaterial.isValid())
        model()->nodeInstanceView()->previewImageDataForGenericNode(m_selectedMaterial, {});
}

bool MaterialEditorView::hasWidget() const
{
    return true;
}

WidgetInfo MaterialEditorView::widgetInfo()
{
    return createWidgetInfo(m_stackedWidget,
                            "MaterialEditor",
                            WidgetInfo::RightPane,
                            0,
                            tr("Material Editor"),
                            tr("Material Editor view"));
}

void MaterialEditorView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                              [[maybe_unused]] const QList<ModelNode> &lastSelectedNodeList)
{
    m_selectedModels.clear();

    for (const ModelNode &node : selectedNodeList) {
        if (node.metaInfo().isQtQuick3DModel())
            m_selectedModels.append(node);
    }

    m_qmlBackEnd->contextObject()->setHasModelSelection(!m_selectedModels.isEmpty());
}

void MaterialEditorView::currentStateChanged(const ModelNode &node)
{
    QmlModelState newQmlModelState(node);
    Q_ASSERT(newQmlModelState.isValid());

    resetView();
}

void MaterialEditorView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    if (!m_selectedMaterial.isValid() || !m_qmlBackEnd)
        return;

    m_locked = true;

    bool changed = false;
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
            changed = true;
        }
    }
    if (changed)
        requestPreviewRender();

    m_locked = false;
}

void MaterialEditorView::nodeTypeChanged(const ModelNode &node, const TypeName &typeName, int, int)
{
     if (node == m_selectedMaterial) {
         m_qmlBackEnd->contextObject()->setCurrentType(QString::fromLatin1(typeName));
         resetView();
     }
}

void MaterialEditorView::rootNodeTypeChanged(const QString &type, int, int)
{
    if (rootModelNode() == m_selectedMaterial) {
        m_qmlBackEnd->contextObject()->setCurrentType(type);
        resetView();
    }
}

void MaterialEditorView::modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap)
{
    if (node == m_selectedMaterial)
        m_qmlBackEnd->updateMaterialPreview(pixmap);
}

void MaterialEditorView::importsChanged([[maybe_unused]] const Imports &addedImports,
                                        [[maybe_unused]] const Imports &removedImports)
{
    m_hasQuick3DImport = model()->hasImport("QtQuick3D");
    m_qmlBackEnd->contextObject()->setHasQuick3DImport(m_hasQuick3DImport);

    if (m_hasQuick3DImport)
        m_ensureMatLibTimer.start(500);

    resetView();
}

void MaterialEditorView::renameMaterial(ModelNode &material, const QString &newName)
{
    QTC_ASSERT(material.isValid(), return);

    QVariant objName = material.variantProperty("objectName").value();
    if (objName.isValid() && objName.toString() == newName)
        return;

    executeInTransaction(__FUNCTION__, [&] {
        material.setIdWithRefactoring(model()->generateIdFromName(newName, "material"));

        VariantProperty objNameProp = material.variantProperty("objectName");
        objNameProp.setValue(newName);
    });
}

void MaterialEditorView::duplicateMaterial(const ModelNode &material)
{
    QTC_ASSERT(material.isValid(), return);

    if (!model())
        return;

    TypeName matType = material.type();
    QmlObjectNode sourceMat(material);
    ModelNode duplicateMatNode;
    QList<AbstractProperty> dynamicProps;

    executeInTransaction(__FUNCTION__, [&] {
        ModelNode matLib = materialLibraryNode();
        if (!matLib.isValid())
            return;

        // create the duplicate material
        NodeMetaInfo metaInfo = model()->metaInfo(matType);
        QmlObjectNode duplicateMat = createModelNode(matType, metaInfo.majorVersion(), metaInfo.minorVersion());

        duplicateMatNode = duplicateMat.modelNode();

        // set name and id
        QString newName = sourceMat.modelNode().variantProperty("objectName").value().toString() + " copy";
        VariantProperty objNameProp = duplicateMatNode.variantProperty("objectName");
        objNameProp.setValue(newName);
        duplicateMatNode.setIdWithoutRefactoring(model()->generateIdFromName(newName, "material"));

        // sync properties. Only the base state is duplicated.
        const QList<AbstractProperty> props = material.properties();
        for (const AbstractProperty &prop : props) {
            if (prop.name() == "objectName" || prop.name() == "data")
                continue;

            if (prop.isVariantProperty()) {
                if (prop.isDynamic()) {
                    dynamicProps.append(prop);
                } else {
                    VariantProperty variantProp = duplicateMatNode.variantProperty(prop.name());
                    variantProp.setValue(prop.toVariantProperty().value());
                }
            } else if (prop.isBindingProperty()) {
                if (prop.isDynamic()) {
                    dynamicProps.append(prop);
                } else {
                    BindingProperty bindingProp = duplicateMatNode.bindingProperty(prop.name());
                    bindingProp.setExpression(prop.toBindingProperty().expression());
                }
            }
        }

        matLib.defaultNodeListProperty().reparentHere(duplicateMat);
    });

    // For some reason, creating dynamic properties in the same transaction doesn't work, so
    // let's do it in separate transaction.
    // TODO: Fix the issue and merge transactions (QDS-8094)
    if (!dynamicProps.isEmpty()) {
        executeInTransaction(__FUNCTION__, [&] {
            for (const AbstractProperty &prop : std::as_const(dynamicProps)) {
                if (prop.isVariantProperty()) {
                    VariantProperty variantProp = duplicateMatNode.variantProperty(prop.name());
                    variantProp.setDynamicTypeNameAndValue(prop.dynamicTypeName(),
                                                           prop.toVariantProperty().value());
                } else if (prop.isBindingProperty()) {
                    BindingProperty bindingProp = duplicateMatNode.bindingProperty(prop.name());
                    bindingProp.setDynamicTypeNameAndExpression(prop.dynamicTypeName(),
                                                                prop.toBindingProperty().expression());
                }
            }
        });
    }
}

void MaterialEditorView::customNotification([[maybe_unused]] const AbstractView *view,
                                            const QString &identifier,
                                            const QList<ModelNode> &nodeList,
                                            const QList<QVariant> &data)
{
    if (identifier == "selected_material_changed") {
        if (!m_hasMaterialRoot) {
            m_selectedMaterial = nodeList.first();
            m_dynamicPropertiesModel->setSelectedNode(m_selectedMaterial);
            QTimer::singleShot(0, this, &MaterialEditorView::resetView);
        }
    } else if (identifier == "apply_to_selected_triggered") {
        applyMaterialToSelectedModels(nodeList.first(), data.first().toBool());
    } else if (identifier == "rename_material") {
        if (m_selectedMaterial == nodeList.first())
            renameMaterial(m_selectedMaterial, data.first().toString());
    } else if (identifier == "add_new_material") {
        handleToolBarAction(MaterialEditorContextObject::AddNewMaterial);
    } else if (identifier == "duplicate_material") {
        duplicateMaterial(nodeList.first());
    }
}

void MaterialEditorView::nodeReparented(const ModelNode &node,
                                        [[maybe_unused]] const NodeAbstractProperty &newPropertyParent,
                                        [[maybe_unused]] const NodeAbstractProperty &oldPropertyParent,
                                        [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    if (node.id() == Constants::MATERIAL_LIB_ID && m_qmlBackEnd && m_qmlBackEnd->contextObject())
        m_qmlBackEnd->contextObject()->setHasMaterialLibrary(true);
}

void MaterialEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (removedNode.id() == Constants::MATERIAL_LIB_ID && m_qmlBackEnd && m_qmlBackEnd->contextObject())
        m_qmlBackEnd->contextObject()->setHasMaterialLibrary(false);
}

void QmlDesigner::MaterialEditorView::highlightSupportedProperties(bool highlight)
{
    if (!m_selectedMaterial.isValid())
        return;

    DesignerPropertyMap &propMap = m_qmlBackEnd->backendValuesPropertyMap();
    const QStringList propNames = propMap.keys();
    NodeMetaInfo metaInfo = m_selectedMaterial.metaInfo();
    QTC_ASSERT(metaInfo.isValid(), return);

    for (const QString &propName : propNames) {
        if (metaInfo.property(propName.toUtf8()).propertyType().isQtQuick3DTexture()) {
            QObject *propEditorValObj = propMap.value(propName).value<QObject *>();
            PropertyEditorValue *propEditorVal = qobject_cast<PropertyEditorValue *>(propEditorValObj);
            propEditorVal->setHasActiveDrag(highlight);
        }
    }
}

void MaterialEditorView::dragStarted(QMimeData *mimeData)
{
    if (mimeData->hasFormat(Constants::MIME_TYPE_ASSETS)) {
        const QString assetPath = QString::fromUtf8(mimeData->data(Constants::MIME_TYPE_ASSETS)).split(',')[0];
        Asset asset(assetPath);

        if (!asset.isValidTextureSource()) // currently only image assets have dnd-supported properties
            return;

        highlightSupportedProperties();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_TEXTURE)
            || mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_TEXTURE)) {
        highlightSupportedProperties();
    }
}

void MaterialEditorView::dragEnded()
{
    highlightSupportedProperties(false);
}

// from model to material editor
void MaterialEditorView::setValue(const QmlObjectNode &qmlObjectNode, const PropertyName &name, const QVariant &value)
{
    m_locked = true;
    m_qmlBackEnd->setValue(qmlObjectNode, name, value);
    m_locked = false;
}

bool MaterialEditorView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        if (m_qmlBackEnd && m_qmlBackEnd->widget() == obj)
            QMetaObject::invokeMethod(m_qmlBackEnd->widget()->rootObject(), "closeContextMenu");
    }
    return AbstractView::eventFilter(obj, event);
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

} // namespace QmlDesigner
