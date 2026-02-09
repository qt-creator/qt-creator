// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorcontextobject.h"

#include "compatibleproperties.h"
#include "propertyeditortracing.h"
#include "propertyeditorutils.h"

#include <abstractview.h>
#include <easingcurvedialog.h>
#include <nodemetainfo.h>
#include <qml3dnode.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlmodelnodeproxy.h>
#include <qmlobjectnode.h>
#include <qmltimeline.h>

#include <utils3d.h>

#include <qmldesigner/settings/designersettings.h>

#include <coreplugin/messagebox.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QCursor>
#include <QFontDatabase>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QQmlContext>
#include <QQuickWidget>
#include <QWindow>

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

namespace QmlDesigner {

using namespace Qt::StringLiterals;
using QmlDesigner::PropertyEditorTracing::category;

static Q_LOGGING_CATEGORY(urlSpecifics, "qtc.propertyeditor.specifics", QtWarningMsg)

PropertyEditorContextObject::PropertyEditorContextObject(QObject *parent)
    : QObject(parent)
    , m_isBaseState(false)
    , m_selectionChanged(false)
    , m_backendValues(nullptr)
    , m_qmlComponent(nullptr)
    , m_qmlContext(nullptr)
{
    NanotraceHR::Tracer tracer{"property editor context object constructor", category()};
}

QString PropertyEditorContextObject::convertColorToString(const QVariant &color)
{
    NanotraceHR::Tracer tracer{"property editor context object convert color to string", category()};

    QString colorString;
    QColor theColor;
    if (color.canConvert(QMetaType(QMetaType::QColor))) {
        theColor = color.value<QColor>();
    } else if (color.canConvert(QMetaType(QMetaType::QVector3D))) {
        auto vec = color.value<QVector3D>();
        theColor = QColor::fromRgbF(vec.x(), vec.y(), vec.z());
    }

    colorString = theColor.name();

    if (theColor.alpha() != 255) {
        QString hexAlpha = QString("%1").arg(theColor.alpha(), 2, 16, QLatin1Char('0'));
        colorString.remove(0, 1);
        colorString.prepend(hexAlpha);
        colorString.prepend(QStringLiteral("#"));
    }
    return colorString;
}

QColor PropertyEditorContextObject::colorFromString(const QString &colorString)
{
    NanotraceHR::Tracer tracer{"property editor context object color from string", category()};

    return QColor::fromString(colorString);
}

QString PropertyEditorContextObject::translateFunction()
{
    NanotraceHR::Tracer tracer{"property editor context object translate function", category()};

    switch (designerSettings().typeOfQsTrFunction()) {
        case 0: return QLatin1String("qsTr");
        case 1: return QLatin1String("qsTrId");
        case 2: return QLatin1String("qsTranslate");
        default:
            break;
    }
    return QLatin1String("qsTr");
}

QStringList PropertyEditorContextObject::autoComplete(const QString &text, int pos, bool explicitComplete, bool filter)
{
    NanotraceHR::Tracer tracer{"property editor context object auto complete", category()};

    if (m_model && m_model->rewriterView())
        return  Utils::filtered(m_model->rewriterView()->autoComplete(text, pos, explicitComplete), [filter](const QString &string) {
            return !filter || (!string.isEmpty() && string.at(0).isUpper()); });

    return {};
}

void PropertyEditorContextObject::toggleExportAlias()
{
    NanotraceHR::Tracer tracer{"property editor context object toggle export alias", category()};

    QTC_ASSERT(m_model && m_model->rewriterView(), return);

    /* Ideally we should not missuse the rewriterView
     * If we add more code here we have to forward the property editor view */
    RewriterView *rewriterView = m_model->rewriterView();

    QTC_ASSERT(!rewriterView->selectedModelNodes().isEmpty(), return);

    const ModelNode selectedNode = rewriterView->selectedModelNodes().constFirst();

    if (QmlObjectNode::isValidQmlObjectNode(selectedNode)) {
        QmlObjectNode objectNode(selectedNode);

        PropertyName modelNodeId = selectedNode.id().toUtf8();
        ModelNode rootModelNode = rewriterView->rootModelNode();

        rewriterView->executeInTransaction("PropertyEditorContextObject:toggleExportAlias",
                                           [&objectNode, &rootModelNode, modelNodeId]() {
                                               if (!objectNode.isAliasExported())
                                                   objectNode.ensureAliasExport();
                                               else if (rootModelNode.hasProperty(modelNodeId))
                                                   rootModelNode.removeProperty(modelNodeId);
                                           });
    }
}

void PropertyEditorContextObject::goIntoComponent()
{
    NanotraceHR::Tracer tracer{"property editor context object go into component", category()};

    QTC_ASSERT(m_model && m_model->rewriterView(), return);

    /* Ideally we should not missuse the rewriterView
     * If we add more code here we have to forward the property editor view */
    RewriterView *rewriterView = m_model->rewriterView();

    QTC_ASSERT(!rewriterView->selectedModelNodes().isEmpty(), return);

    const ModelNode selectedNode = rewriterView->selectedModelNodes().constFirst();

    DocumentManager::goIntoComponent(selectedNode);
}

void PropertyEditorContextObject::changeTypeName(const QString &typeName)
{
    NanotraceHR::Tracer tracer{"property editor context object change type name", category()};

    QTC_ASSERT(m_model && m_model->rewriterView(), return);

    /* Ideally we should not missuse the rewriterView
     * If we add more code here we have to forward the property editor view */
    RewriterView *rewriterView = m_model->rewriterView();

    QTC_ASSERT(!m_editorNodes.isEmpty(), return);

    auto changeNodeTypeName = [&](ModelNode &selectedNode) {
        // Check if the requested type is the same as already set
        if (selectedNode.simplifiedTypeName() == typeName)
            return;

        NodeMetaInfo metaInfo = m_model->metaInfo(typeName.toLatin1());
        if (!metaInfo.isValid()) {
            Core::AsynchronousMessageBox::warning(tr("Invalid Type"),
                                                  tr("%1 is an invalid type.").arg(typeName));
            return;
        }

        // Create a list of properties available for the new type
        auto propertiesAndSignals = Utils::transform<PropertyNameList>(
            PropertyEditorUtils::filteredProperties(metaInfo), &PropertyMetaInfo::name);
        // Add signals to the list

        const PropertyNameList &signalNames = metaInfo.signalNames();
        for (const PropertyName &signal : signalNames) {
            if (signal.isEmpty())
                continue;

            PropertyName name = signal;
            QChar firstChar = QChar(signal.at(0)).toUpper().toLatin1();
            name[0] = firstChar.toLatin1();
            name.prepend("on");
            propertiesAndSignals.append(name);
        }

        // Add dynamic properties and respective change signals
        const QList<AbstractProperty> &nodeProperties = selectedNode.properties();
        for (const AbstractProperty &property : nodeProperties) {
            if (!property.isDynamic())
                continue;

            // Add dynamic property
            propertiesAndSignals.append(property.name().toByteArray());
            // Add its change signal
            PropertyName name = property.name().toByteArray();
            QChar firstChar = QChar(property.name().at(0)).toUpper().toLatin1();
            name[0] = firstChar.toLatin1();
            name.prepend("on");
            name.append("Changed");
            propertiesAndSignals.append(name);
        }

        // Compare current properties and signals with the once available for change type
        QList<PropertyName> incompatibleProperties;
        for (const AbstractProperty &property : nodeProperties) {
            if (!propertiesAndSignals.contains(property.name()))
                incompatibleProperties.append(property.name().toByteArray());
        }

        CompatibleProperties compatibleProps(selectedNode.metaInfo(), metaInfo);
        compatibleProps.createCompatibilityMap(incompatibleProperties);

        Utils::sort(incompatibleProperties);

        // Create a dialog showing incompatible properties and signals
        if (!incompatibleProperties.empty()) {
            QString detailedText = QString("<b>Incompatible properties:</b><br>");

            for (const auto &p : std::as_const(incompatibleProperties))
                detailedText.append("- " + QString::fromUtf8(p) + "<br>");

            detailedText.chop(QString("<br>").size());

            QMessageBox msgBox;
            msgBox.setTextFormat(Qt::RichText);
            msgBox.setIcon(QMessageBox::Question);
            msgBox.setWindowTitle("Change Type");
            msgBox.setText(QString("Changing the type from %1 to %2 can't be done without removing "
                                   "incompatible properties.<br><br>%3")
                               .arg(selectedNode.simplifiedTypeName(), typeName, detailedText));
            msgBox.setInformativeText(
                "Do you want to continue by removing incompatible properties?");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Ok);

            if (msgBox.exec() == QMessageBox::Cancel)
                return;

            for (const auto &p : std::as_const(incompatibleProperties))
                selectedNode.removeProperty(p);
        }

        compatibleProps.copyMappedProperties(selectedNode);
#ifdef QDS_USE_PROJECTSTORAGE
        if (selectedNode.isRootNode())
            rewriterView->changeRootNodeType(typeName.toUtf8(), -1, -1);
        else
            selectedNode.changeType(typeName.toUtf8(), -1, -1);
#else
        if (selectedNode.isRootNode())
            rewriterView->changeRootNodeType(metaInfo.typeName(),
                                             metaInfo.majorVersion(),
                                             metaInfo.minorVersion());
        else
            selectedNode.changeType(metaInfo.typeName(),
                                    metaInfo.majorVersion(),
                                    metaInfo.minorVersion());
#endif
        compatibleProps.applyCompatibleProperties(selectedNode);
    };

    try {
        auto transaction = RewriterTransaction(rewriterView, "PropertyEditorContextObject:changeTypeName");

        ModelNodes selectedNodes = m_editorNodes;
        for (ModelNode &selectedNode : selectedNodes)
            changeNodeTypeName(selectedNode);

        transaction.commit();
    } catch (const Exception &e) {
        e.showException();
    }
}

void PropertyEditorContextObject::insertKeyframe(const QString &propertyName)
{
    QTC_ASSERT(m_model && m_model->rewriterView(), return);

    if (isBlocked(propertyName))
        return;

    /* Ideally we should not missuse the rewriterView
     * If we add more code here we have to forward the property editor view */
    RewriterView *rewriterView = m_model->rewriterView();

    QTC_ASSERT(!rewriterView->selectedModelNodes().isEmpty(), return);

    ModelNode selectedNode = rewriterView->selectedModelNodes().constFirst();

    QmlTimeline timeline = rewriterView->currentTimelineNode();

    QTC_ASSERT(timeline.isValid(), return );
    QTC_ASSERT(selectedNode.isValid(), return );

    rewriterView->executeInTransaction("PropertyEditorContextObject::insertKeyframe", [&]{
        timeline.insertKeyframe(selectedNode, propertyName.toUtf8());
    });
}

QString PropertyEditorContextObject::activeDragSuffix() const
{
    NanotraceHR::Tracer tracer{"property editor context object active drag suffix", category()};

    return m_activeDragSuffix;
}

void PropertyEditorContextObject::setActiveDragSuffix(const QString &suffix)
{
    NanotraceHR::Tracer tracer{"property editor context object set active drag suffix", category()};

    if (m_activeDragSuffix != suffix) {
        m_activeDragSuffix = suffix;
        emit activeDragSuffixChanged();
    }
}

int PropertyEditorContextObject::majorVersion() const
{
    NanotraceHR::Tracer tracer{"property editor context object major version", category()};

    return m_majorVersion;
}

int PropertyEditorContextObject::majorQtQuickVersion() const
{
    NanotraceHR::Tracer tracer{"property editor context object major Qt Quick version", category()};

    return m_majorQtQuickVersion;
}

int PropertyEditorContextObject::minorQtQuickVersion() const
{
    NanotraceHR::Tracer tracer{"property editor context object minor Qt Quick version", category()};

    return m_minorQtQuickVersion;
}

void PropertyEditorContextObject::setMajorVersion(int majorVersion)
{
    NanotraceHR::Tracer tracer{"property editor context object set major version", category()};

    if (m_majorVersion == majorVersion)
        return;

    m_majorVersion = majorVersion;

    emit majorVersionChanged();
}

void PropertyEditorContextObject::setMajorQtQuickVersion(int majorVersion)
{
    NanotraceHR::Tracer tracer{"property editor context object set major Qt Quick version",
                               category()};

    if (m_majorQtQuickVersion == majorVersion)
        return;

    m_majorQtQuickVersion = majorVersion;

    emit majorQtQuickVersionChanged();

}

void PropertyEditorContextObject::setMinorQtQuickVersion(int minorVersion)
{
    NanotraceHR::Tracer tracer{"property editor context object set minor Qt Quick version",
                               category()};

    if (m_minorQtQuickVersion == minorVersion)
        return;

    m_minorQtQuickVersion = minorVersion;

    emit minorQtQuickVersionChanged();
}

int PropertyEditorContextObject::minorVersion() const
{
    NanotraceHR::Tracer tracer{"property editor context object minor version", category()};

    return m_minorVersion;
}

void PropertyEditorContextObject::setMinorVersion(int minorVersion)
{
    if (m_minorVersion == minorVersion)
        return;

    m_minorVersion = minorVersion;

    emit minorVersionChanged();
}

void PropertyEditorContextObject::setEditorInstancesCount(int n)
{
    NanotraceHR::Tracer tracer{"property editor context object set editor instances count",
                               category()};

    if (m_editorInstancesCount == n)
        return;

    m_editorInstancesCount = n;
    emit editorInstancesCountChanged();
}

int PropertyEditorContextObject::editorInstancesCount() const
{
    NanotraceHR::Tracer tracer{"property editor context object editor instances count", category()};

    return m_editorInstancesCount;
}

bool PropertyEditorContextObject::hasActiveTimeline() const
{
    NanotraceHR::Tracer tracer{"property editor context object has active timeline", category()};
    return m_setHasActiveTimeline;
}

void PropertyEditorContextObject::setHasActiveTimeline(bool b)
{
    NanotraceHR::Tracer tracer{"property editor context object set has active timeline", category()};

    if (b == m_setHasActiveTimeline)
        return;

    m_setHasActiveTimeline = b;
    emit hasActiveTimelineChanged();
}

void PropertyEditorContextObject::insertInQmlContext(QQmlContext *context)
{
    NanotraceHR::Tracer tracer{"property editor context object insert in QML context", category()};

    m_qmlContext = context;
    m_qmlContext->setContextObject(this);
}

QQmlComponent *PropertyEditorContextObject::specificQmlComponent()
{
    NanotraceHR::Tracer tracer{"property editor context object specific QML component", category()};

    if (m_qmlComponent)
        return m_qmlComponent;

    m_qmlComponent = new QQmlComponent(m_qmlContext->engine(), this);

    m_qmlComponent->setData(m_specificQmlData.toUtf8(), QUrl::fromLocalFile("specifics.qml"));

    const bool showError = qEnvironmentVariableIsSet(Constants::ENVIRONMENT_SHOW_QML_ERRORS);
    if (showError && !m_specificQmlData.isEmpty() && !m_qmlComponent->errors().isEmpty()) {
        const QString errMsg = m_qmlComponent->errors().constFirst().toString();
        Core::AsynchronousMessageBox::warning(tr("Invalid QML source"), errMsg);
    }

    return m_qmlComponent;
}

bool PropertyEditorContextObject::hasMultiSelection() const
{
    NanotraceHR::Tracer tracer{"property editor context object has multi selection", category()};

    return m_hasMultiSelection;
}

void PropertyEditorContextObject::setHasMultiSelection(bool b)
{
    NanotraceHR::Tracer tracer{"property editor context object set has multi selection", category()};
    if (b == m_hasMultiSelection)
        return;

    m_hasMultiSelection = b;
    emit hasMultiSelectionChanged();
}

bool PropertyEditorContextObject::isMultiPropertyEditorPluginEnabled() const
{
    NanotraceHR::Tracer tracer{
        "property editor context object is multi property editor plugin enabled", category()};

    const auto plugins = ExtensionSystem::PluginManager::plugins();
    auto found = std::ranges::find(plugins, "multipropertyeditor"_L1, &ExtensionSystem::PluginSpec::id);

    if (found != plugins.end())
        return (*found)->isEffectivelyEnabled();

    return false;
}

bool PropertyEditorContextObject::isSelectionLocked() const
{
    NanotraceHR::Tracer tracer{"property editor context object is selection locked", category()};

    return m_isSelectionLocked;
}

void PropertyEditorContextObject::setIsSelectionLocked(bool lock)
{
    NanotraceHR::Tracer tracer{"property editor context object set is selection locked", category()};

    if (lock == m_isSelectionLocked)
        return;

    m_isSelectionLocked = lock;
    emit isSelectionLockedChanged();
}

void PropertyEditorContextObject::setInsightEnabled(bool value)
{
    NanotraceHR::Tracer tracer{"property editor context object set insight enabled", category()};

    if (value != m_insightEnabled) {
        m_insightEnabled = value;
        emit insightEnabledChanged();
    }
}

void PropertyEditorContextObject::setInsightCategories(const QStringList &categories)
{
    NanotraceHR::Tracer tracer{"property editor context object set insight categories", category()};

    m_insightCategories = categories;
    emit insightCategoriesChanged();
}

bool PropertyEditorContextObject::hasQuick3DImport() const
{
    NanotraceHR::Tracer tracer{"property editor context object has quick 3D import", category()};

    return m_hasQuick3DImport;
}

void PropertyEditorContextObject::setEditorNodes(const ModelNodes &nodes)
{
    m_editorNodes = nodes;
}

void PropertyEditorContextObject::setHasQuick3DImport(bool value)
{
    NanotraceHR::Tracer tracer{"property editor context object set has quick 3D import", category()};

    if (value == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = value;
    emit hasQuick3DImportChanged();
}

bool PropertyEditorContextObject::hasMaterialLibrary() const
{
    NanotraceHR::Tracer tracer{"property editor context object has material library", category()};

    return m_hasMaterialLibrary;
}

void PropertyEditorContextObject::setHasMaterialLibrary(bool value)
{
    NanotraceHR::Tracer tracer{"property editor context object set has material library", category()};

    if (value == m_hasMaterialLibrary)
        return;

    m_hasMaterialLibrary = value;
    emit hasMaterialLibraryChanged();
}

bool PropertyEditorContextObject::isQt6Project() const
{
    NanotraceHR::Tracer tracer{"property editor context object is Qt6 project", category()};

    return m_isQt6Project;
}

void PropertyEditorContextObject::setIsQt6Project(bool value)
{
    NanotraceHR::Tracer tracer{"property editor context object set is Qt6 project", category()};

    if (m_isQt6Project == value)
        return;

    m_isQt6Project = value;
    emit isQt6ProjectChanged();
}

bool PropertyEditorContextObject::has3DModelSelected() const
{
    NanotraceHR::Tracer tracer{"property editor context object has 3D model selected", category()};

    return m_has3DModelSelected;
}

bool PropertyEditorContextObject::has3DScene() const
{
    NanotraceHR::Tracer tracer{"property editor context object has 3D scene", category()};

    return m_has3DScene;
}

void PropertyEditorContextObject::setHas3DScene(bool value)
{
    NanotraceHR::Tracer tracer{"property editor context object set has 3D scene", category()};

    if (value == m_has3DScene)
        return;

    m_has3DScene = value;
    emit has3DSceneChanged();
}

void PropertyEditorContextObject::setHas3DModelSelected(bool value)
{
    NanotraceHR::Tracer tracer{"property editor context object set has 3D model selected", category()};

    if (value == m_has3DModelSelected)
        return;

    m_has3DModelSelected = value;
    emit has3DModelSelectedChanged();
}

void PropertyEditorContextObject::setSpecificsUrl(const QUrl &newSpecificsUrl)
{
    NanotraceHR::Tracer tracer{"property editor context object set specifics URL", category()};

    if (newSpecificsUrl == m_specificsUrl)
        return;

    qCInfo(urlSpecifics) << Q_FUNC_INFO << newSpecificsUrl;

    m_specificsUrl = newSpecificsUrl;
    emit specificsUrlChanged();
}

void PropertyEditorContextObject::setSpecificQmlData(const QString &newSpecificQmlData)
{
    NanotraceHR::Tracer tracer{"property editor context object set specific QML data", category()};

    if (m_specificQmlData == newSpecificQmlData)
        return;

    m_specificQmlData = newSpecificQmlData;

    delete m_qmlComponent;
    m_qmlComponent = nullptr;

    emit specificQmlComponentChanged();
    emit specificQmlDataChanged();
}

void PropertyEditorContextObject::setStateName(const QString &newStateName)
{
    NanotraceHR::Tracer tracer{"property editor context object set state name", category()};

    if (newStateName == m_stateName)
        return;

    m_stateName = newStateName;
    emit stateNameChanged();
}

void PropertyEditorContextObject::setAllStateNames(const QStringList &allStates)
{
    NanotraceHR::Tracer tracer{"property editor context object set all state names", category()};

    if (allStates == m_allStateNames)
        return;

    m_allStateNames = allStates;
    emit allStateNamesChanged();
}

void PropertyEditorContextObject::setIsBaseState(bool newIsBaseState)
{
    NanotraceHR::Tracer tracer{"property editor context object set is base state", category()};

    if (newIsBaseState == m_isBaseState)
        return;

    m_isBaseState = newIsBaseState;
    emit isBaseStateChanged();
}

void PropertyEditorContextObject::setSelectionChanged(bool newSelectionChanged)
{
    NanotraceHR::Tracer tracer{"property editor context object set selection changed", category()};

    if (newSelectionChanged == m_selectionChanged)
        return;

    m_selectionChanged = newSelectionChanged;
    emit selectionChangedChanged();
}

void PropertyEditorContextObject::setBackendValues(QQmlPropertyMap *newBackendValues)
{
    NanotraceHR::Tracer tracer{"property editor context object set backend values", category()};

    if (newBackendValues == m_backendValues)
        return;

    m_backendValues = newBackendValues;
    emit backendValuesChanged();
}

void PropertyEditorContextObject::setModel(Model *model)
{
    NanotraceHR::Tracer tracer{"property editor context object set model", category()};

    m_model = model;
    setHas3DScene(Utils3D::active3DSceneId(model) != -1);
}

void PropertyEditorContextObject::triggerSelectionChanged()
{
    NanotraceHR::Tracer tracer{"property editor context object trigger selection changed", category()};

    setSelectionChanged(!m_selectionChanged);
}

void PropertyEditorContextObject::setHasAliasExport(bool hasAliasExport)
{
    NanotraceHR::Tracer tracer{"property editor context object set has alias export", category()};

    if (m_aliasExport == hasAliasExport)
        return;

    m_aliasExport = hasAliasExport;
    emit hasAliasExportChanged();
}

void PropertyEditorContextObject::setQuickWidget(QQuickWidget *newQuickWidget)
{
    NanotraceHR::Tracer tracer{"property editor context object set quick widget", category()};

    m_quickWidget = newQuickWidget;
}

void PropertyEditorContextObject::hideCursor()
{
    if (QApplication::overrideCursor())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    if (QWidget *w = QApplication::activeWindow())
        m_lastPos = QCursor::pos(w->screen());
}

void PropertyEditorContextObject::restoreCursor()
{
    NanotraceHR::Tracer tracer{"property editor context object restore cursor", category()};

    if (!QApplication::overrideCursor())
        return;

    QApplication::restoreOverrideCursor();

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

void PropertyEditorContextObject::holdCursorInPlace()
{
    NanotraceHR::Tracer tracer{"property editor context object hold cursor in place", category()};

    if (!QApplication::overrideCursor())
        return;

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

int PropertyEditorContextObject::devicePixelRatio()
{
    NanotraceHR::Tracer tracer{"property editor context object device pixel ratio", category()};

    if (QWidget *w = QApplication::activeWindow())
        return w->devicePixelRatio();

    return 1;
}

QStringList PropertyEditorContextObject::styleNamesForFamily(const QString &family)
{
    NanotraceHR::Tracer tracer{"property editor context object style names for family", category()};

    return QFontDatabase::styles(family);
}

QStringList PropertyEditorContextObject::allStatesForId(const QString &id)
{
    NanotraceHR::Tracer tracer{"property editor context object all states for id", category()};

    if (m_model && m_model->rewriterView()) {
        const QmlObjectNode node = m_model->rewriterView()->modelNodeForId(id);
        if (node.isValid())
            return node.allStateNames();
    }

      return {};
}

bool PropertyEditorContextObject::isBlocked(const QString &propName) const
{
    NanotraceHR::Tracer tracer{"property editor context object is blocked", category()};

    if (m_model && m_model->rewriterView()) {
        const QList<ModelNode> nodes = m_model->rewriterView()->selectedModelNodes();
        for (const auto &node : nodes) {
              if (Qml3DNode qml3DNode{node}; qml3DNode.isBlocked(propName.toUtf8()))
                return true;
        }
    }
    return false;
}

void PropertyEditorContextObject::verifyInsightImport()
{
    NanotraceHR::Tracer tracer{"property editor context object verify insight import", category()};

    Import import = Import::createLibraryImport("QtInsightTracker", "1.0");

    if (!m_model->hasImport(import))
        m_model->changeImports({import}, {});
}

QRect PropertyEditorContextObject::screenRect() const
{
    NanotraceHR::Tracer tracer{"property editor context object screen rect", category()};

    if (m_quickWidget && m_quickWidget->screen())
        return m_quickWidget->screen()->availableGeometry();
    return  {};
}

QPoint PropertyEditorContextObject::globalPos(const QPoint &point) const
{
    NanotraceHR::Tracer tracer{"property editor context object global pos", category()};

    if (m_quickWidget)
        return m_quickWidget->mapToGlobal(point);
    return point;
}

void PropertyEditorContextObject::handleToolBarAction(int action)
{
    NanotraceHR::Tracer tracer{"property editor context object handle toolbar action", category()};

    emit toolBarAction(action);
}

void PropertyEditorContextObject::saveExpandedState(const QString &sectionName, bool expanded)
{
    NanotraceHR::Tracer tracer{"property editor context object save expanded state", category()};

    s_expandedStateHash.insert(sectionName, expanded);
}

bool PropertyEditorContextObject::loadExpandedState(const QString &sectionName, bool defaultValue) const
{
    NanotraceHR::Tracer tracer{"property editor context object load expanded state", category()};

    return s_expandedStateHash.value(sectionName, defaultValue);
}

void EasingCurveEditor::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"property editor context object register declarative type", category()};

    qmlRegisterType<EasingCurveEditor>("HelperWidgets", 2, 0, "EasingCurveEditor");
}

void EasingCurveEditor::runDialog()
{
    NanotraceHR::Tracer tracer{"property editor context object run dialog", category()};

    if (m_modelNode.isValid())
        EasingCurveDialog::runDialog({ m_modelNode }, Core::ICore::dialogParent());
}

void EasingCurveEditor::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    NanotraceHR::Tracer tracer{"property editor context object set model node backend", category()};

    if (!modelNodeBackend.isNull() && modelNodeBackend.isValid()) {
        m_modelNodeBackend = modelNodeBackend;

        const auto modelNodeBackendObject = m_modelNodeBackend.value<QObject*>();

        const auto backendObjectCasted =
                qobject_cast<const QmlDesigner::QmlModelNodeProxy *>(modelNodeBackendObject);

        if (backendObjectCasted) {
            m_modelNode = backendObjectCasted->qmlObjectNode().modelNode();
        }

        emit modelNodeBackendChanged();
    }
}

QVariant EasingCurveEditor::modelNodeBackend() const
{
    NanotraceHR::Tracer tracer{"property editor context object model node backend", category()};

    return m_modelNodeBackend;
}

} //QmlDesigner
