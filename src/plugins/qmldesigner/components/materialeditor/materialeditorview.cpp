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
#include "materialeditordynamicpropertiesproxymodel.h"
#include "propertyeditorvalue.h"
#include "materialeditortransaction.h"
#include "assetslibrarywidget.h"

#include <bindingproperty.h>
#include <dynamicpropertiesmodel.h>
#include <metainfo.h>
#include <nodeinstanceview.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <rewritingexception.h>
#include <variantproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmltimeline.h>

#include <theme.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <designmodewidget.h>
#include <qmldesignerplugin.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <propertyeditorqmlbackend.h>

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
#include <QColorDialog>

namespace QmlDesigner {

MaterialEditorView::MaterialEditorView(QWidget *parent)
    : AbstractView(parent)
    , m_stackedWidget(new QStackedWidget(parent))
    , m_dynamicPropertiesModel(new Internal::DynamicPropertiesModel(true, this))
{
    m_updateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F7), m_stackedWidget);
    connect(m_updateShortcut, &QShortcut::activated, this, &MaterialEditorView::reloadQml);

    m_ensureMatLibTimer.callOnTimeout([this] {
        if (model() && model()->rewriterView() && !model()->rewriterView()->hasIncompleteTypeInformation()
            && model()->rewriterView()->errors().isEmpty()) {
            executeInTransaction("MaterialEditorView::MaterialEditorView", [this] {
                ensureMaterialLibraryNode();
            });
            m_ensureMatLibTimer.stop();
        }
    });

    m_typeUpdateTimer.setSingleShot(true);
    m_typeUpdateTimer.setInterval(500);
    connect(&m_typeUpdateTimer, &QTimer::timeout, this, &MaterialEditorView::updatePossibleTypes);

    m_stackedWidget->setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));
    m_stackedWidget->setMinimumWidth(250);
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

Internal::DynamicPropertiesModel *MaterialEditorView::dynamicPropertiesModel() const
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
        if (!model())
            break;
        executeInTransaction("MaterialEditorView:handleToolBarAction", [&] {
            ModelNode matLib = materialLibraryNode();
            if (!matLib.isValid())
                return;

            NodeMetaInfo metaInfo = model()->metaInfo("QtQuick3D.DefaultMaterial");
            ModelNode newMatNode = createModelNode("QtQuick3D.DefaultMaterial", metaInfo.majorVersion(),
                                                                                metaInfo.minorVersion());
            renameMaterial(newMatNode, "New Material");
            matLib.defaultNodeListProperty().reparentHere(newMatNode);
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

    PropertyName matPrevEnvAuxProp("matPrevEnv");
    PropertyName matPrevEnvValueAuxProp("matPrevEnvValue");

    auto renderPreviews = [=](const QString &auxEnv, const QString &auxValue) {
        rootModelNode().setAuxiliaryData(matPrevEnvAuxProp, auxEnv);
        rootModelNode().setAuxiliaryData(matPrevEnvValueAuxProp, auxValue);
        QTimer::singleShot(0, this, &MaterialEditorView::requestPreviewRender);
        emitCustomNotification("refresh_material_browser", {});
    };

    if (env == "Color") {
        m_colorDialog.clear();

        // Store color to separate property to persist selection over non-color env changes
        PropertyName colorAuxProp("matPrevColor");
        QString oldColor = rootModelNode().auxiliaryData(colorAuxProp).toString();
        QString oldEnv = rootModelNode().auxiliaryData(matPrevEnvAuxProp).toString();
        QString oldValue = rootModelNode().auxiliaryData(matPrevEnvValueAuxProp).toString();

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
            rootModelNode().setAuxiliaryData(colorAuxProp, color.name());
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

    rootModelNode().setAuxiliaryData("matPrevModel", modelStr);

    QTimer::singleShot(0, this, &MaterialEditorView::requestPreviewRender);
    emitCustomNotification("refresh_material_browser", {});
}

void MaterialEditorView::setupQmlBackend()
{
    QUrl qmlPaneUrl;
    QUrl qmlSpecificsUrl;
    QString specificQmlData;
    QString currentTypeName;

    if (m_selectedMaterial.isValid() && m_hasQuick3DImport) {
        qmlPaneUrl = QUrl::fromLocalFile(materialEditorResourcesPath() + "/MaterialEditorPane.qml");

        TypeName diffClassName;
        NodeMetaInfo metaInfo = m_selectedMaterial.metaInfo();
        if (metaInfo.isValid()) {
            diffClassName = metaInfo.typeName();
            const QList<NodeMetaInfo> hierarchy = metaInfo.classHierarchy();
            for (const NodeMetaInfo &metaInfo : hierarchy) {
                if (PropertyEditorQmlBackend::checkIfUrlExists(qmlSpecificsUrl))
                    break;
                qmlSpecificsUrl = PropertyEditorQmlBackend::getQmlFileUrl(metaInfo.typeName()
                                                                          + "Specifics", metaInfo);
                diffClassName = metaInfo.typeName();
            }
        }
        if (metaInfo.isValid() && diffClassName != m_selectedMaterial.type()) {
            specificQmlData = PropertyEditorQmlBackend::templateGeneration(
                        metaInfo, model()->metaInfo(diffClassName), m_selectedMaterial);
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
    currentQmlBackend->contextObject()->setHasMaterialRoot(m_hasMaterialRoot);
    currentQmlBackend->contextObject()->setSpecificQmlData(specificQmlData);
    currentQmlBackend->contextObject()->setCurrentType(currentTypeName);

    m_qmlBackEnd = currentQmlBackend;

    if (m_hasMaterialRoot)
        m_dynamicPropertiesModel->setSelectedNode(m_selectedMaterial);
    else
        m_dynamicPropertiesModel->reset();

    delayedTypeUpdate();
    initPreviewData();

    m_stackedWidget->setCurrentWidget(m_qmlBackEnd->widget());
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

void MaterialEditorView::initPreviewData()
{
    if (model() && m_qmlBackEnd) {
        QString env = rootModelNode().auxiliaryData("matPrevEnv").toString();
        QString envValue = rootModelNode().auxiliaryData("matPrevEnvValue").toString();
        QString modelStr = rootModelNode().auxiliaryData("matPrevModel").toString();
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
    static const QStringList basicTypes {"DefaultMaterial", "PrincipledMaterial", "CustomMaterial"};
    QStringList allTypes = basicTypes;

    const QList<ItemLibraryEntry> itemLibEntries = m_itemLibraryInfo->entries();
    for (const ItemLibraryEntry &entry : itemLibEntries) {
        NodeMetaInfo metaInfo = model()->metaInfo(entry.typeName());
        bool valid = metaInfo.isValid()
                     && (metaInfo.majorVersion() >= entry.majorVersion()
                         || metaInfo.majorVersion() < 0);
        if (valid && metaInfo.isSubclassOf("QtQuick3D.Material")) {
            bool addImport = entry.requiredImport().isEmpty();
            if (!addImport) {
                Import import = entryToImport(entry);
                addImport = model()->hasImport(import, true, true);
            }
            if (addImport) {
                QString typeName = QString::fromLatin1(entry.typeName().split('.').last());
                if (!allTypes.contains(typeName))
                    allTypes.append(typeName);
            }
        }
    }
    m_qmlBackEnd->contextObject()->setPossibleTypes(allTypes);
}

void MaterialEditorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_locked = true;

    m_hasQuick3DImport = model->hasImport("QtQuick3D");
    m_hasMaterialRoot = rootModelNode().isSubclassOf("QtQuick3D.Material");

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
                m_dynamicPropertiesModel->variantPropertyChanged(property);
            if (m_selectedMaterial.property(property.name()).isBindingProperty())
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
                m_dynamicPropertiesModel->bindingPropertyChanged(property);
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

void MaterialEditorView::auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &)
{

    if (noValidSelection() || !node.isSelected())
        return;

    m_qmlBackEnd->setValueforAuxiliaryProperties(m_selectedMaterial, name);
}

void MaterialEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const auto &property : propertyList) {
        if (property.isBindingProperty())
            m_dynamicPropertiesModel->bindingRemoved(property.toBindingProperty());
        else if (property.isVariantProperty())
            m_dynamicPropertiesModel->variantRemoved(property.toVariantProperty());
    }
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
                            tr("Material Editor"));
}

void MaterialEditorView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                              const QList<ModelNode> &lastSelectedNodeList)
{
    Q_UNUSED(lastSelectedNodeList)

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
         delayedResetView();
     }
}

void MaterialEditorView::rootNodeTypeChanged(const QString &type, int, int)
{
    if (rootModelNode() == m_selectedMaterial) {
        m_qmlBackEnd->contextObject()->setCurrentType(type);
        delayedResetView();
    }
}

void MaterialEditorView::modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap)
{
    if (node == m_selectedMaterial)
        m_qmlBackEnd->updateMaterialPreview(pixmap);
}

void MaterialEditorView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    Q_UNUSED(addedImports)
    Q_UNUSED(removedImports)

    m_hasQuick3DImport = model()->hasImport("QtQuick3D");
    m_qmlBackEnd->contextObject()->setHasQuick3DImport(m_hasQuick3DImport);

    if (m_hasQuick3DImport)
        m_ensureMatLibTimer.start(500);

    resetView();
}

void MaterialEditorView::renameMaterial(ModelNode &material, const QString &newName)
{
    QTC_ASSERT(material.isValid(), return);

    executeInTransaction("MaterialEditorView:renameMaterial", [&] {
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

    executeInTransaction(__FUNCTION__, [&] {
        ModelNode matLib = materialLibraryNode();
        if (!matLib.isValid())
            return;

        // create the duplicate material
        NodeMetaInfo metaInfo = model()->metaInfo(matType);
        QmlObjectNode duplicateMat = createModelNode(matType, metaInfo.majorVersion(), metaInfo.minorVersion());

        // set name and id
        QString newName = sourceMat.modelNode().variantProperty("objectName").value().toString() + " copy";
        duplicateMat.modelNode().variantProperty("objectName").setValue(newName);
        duplicateMat.modelNode().setIdWithoutRefactoring(model()->generateIdFromName(newName, "material"));

        // sync properties
        const QList<AbstractProperty> props = material.properties();
        for (const AbstractProperty &prop : props) {
            if (prop.name() == "objectName")
                continue;

            if (prop.isVariantProperty())
                duplicateMat.setVariantProperty(prop.name(), prop.toVariantProperty().value());
            else if (prop.isBindingProperty())
                duplicateMat.setBindingProperty(prop.name(), prop.toBindingProperty().expression());
        }

        matLib.defaultNodeListProperty().reparentHere(duplicateMat);
    });
}

void MaterialEditorView::customNotification(const AbstractView *view, const QString &identifier,
                                            const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    Q_UNUSED(view)

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

void QmlDesigner::MaterialEditorView::highlightSupportedProperties(bool highlight)
{
    if (!m_selectedMaterial.isValid())
        return;

    DesignerPropertyMap &propMap = m_qmlBackEnd->backendValuesPropertyMap();
    const QStringList propNames = propMap.keys();
    NodeMetaInfo metaInfo = m_selectedMaterial.metaInfo();
    QTC_ASSERT(metaInfo.isValid(), return);

    for (const QString &propName : propNames) {
        if (metaInfo.propertyTypeName(propName.toLatin1()) == "QtQuick3D.Texture") {
            QObject *propEditorValObj = propMap.value(propName).value<QObject *>();
            PropertyEditorValue *propEditorVal = qobject_cast<PropertyEditorValue *>(propEditorValObj);
            propEditorVal->setHasActiveDrag(highlight);
        }
    }
}

void MaterialEditorView::dragStarted(QMimeData *mimeData)
{
    if (!mimeData->hasFormat(Constants::MIME_TYPE_ASSETS))
        return;

    const QString assetPath = QString::fromUtf8(mimeData->data(Constants::MIME_TYPE_ASSETS)).split(',')[0];
    QString assetType = AssetsLibraryWidget::getAssetTypeAndData(assetPath).first;

    if (assetType != Constants::MIME_TYPE_ASSET_IMAGE) // currently only image assets have dnd-supported properties
        return;

    highlightSupportedProperties();
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
    return QObject::eventFilter(obj, event);
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
