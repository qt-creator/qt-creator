// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "connectionview.h"

#include "backendmodel.h"
#include "bindingmodel.h"
#include "connectionmodel.h"
#include "dynamicpropertiesmodel.h"
#include "propertytreemodel.h"
#include "theme.h"

#include <bindingproperty.h>
#include <nodeabstractproperty.h>
#include <signalhandlerproperty.h>
#include <variantproperty.h>
#include <viewmanager.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmldesignertr.h>

#include <studioquickwidget.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <utils/qtcassert.h>

#include <QQmlEngine>
#include <QShortcut>
#include <QTableView>

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

class ConnectionViewQuickWidget : public StudioQuickWidget
{
    // Q_OBJECT carefull

public:
    ConnectionViewQuickWidget(ConnectionView *connectionEditorView)
        : m_connectionEditorView(connectionEditorView)

    {
        engine()->addImportPath(qmlSourcesPath());
        engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
        engine()->addImportPath(qmlSourcesPath() + "/imports");

        m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F12), this);
        connect(m_qmlSourceUpdateShortcut,
                &QShortcut::activated,
                this,
                &ConnectionViewQuickWidget::reloadQmlSource);

        quickWidget()->setObjectName(Constants::OBJECT_NAME_CONNECTION_EDITOR);
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto map = registerPropertyMap("ConnectionsEditorEditorBackend");
        qmlRegisterAnonymousType<DynamicPropertiesModel>("ConnectionsEditorEditorBackend", 1);
        qmlRegisterAnonymousType<DynamicPropertiesModelBackendDelegate>(
            "ConnectionsEditorEditorBackend", 1);

        map->setProperties(
            {{"connectionModel", QVariant::fromValue(m_connectionEditorView->connectionModel())}});

        map->setProperties(
            {{"bindingModel", QVariant::fromValue(m_connectionEditorView->bindingModel())}});

        map->setProperties(
            {{"dynamicPropertiesModel",
              QVariant::fromValue(m_connectionEditorView->dynamicPropertiesModel())}});

        qmlRegisterType<ConnectionModelBackendDelegate>("ConnectionsEditorEditorBackend",
                                                        1,
                                                        0,
                                                        "DynamicPropertiesModelBackendDelegate");

        qmlRegisterType<ConnectionModelStatementDelegate>("ConnectionsEditorEditorBackend",
                                                          1,
                                                          0,
                                                          "ConnectionModelStatementDelegate");

        qmlRegisterType<ConditionListModel>("ConnectionsEditorEditorBackend", 1, 0, "ConditionListModel");

        qmlRegisterType<PropertyTreeModel>("ConnectionsEditorEditorBackend", 1, 0, "PropertyTreeModel");
        qmlRegisterType<PropertyListProxyModel>("ConnectionsEditorEditorBackend",
                                                1,
                                                0,
                                                "PropertyListProxyModel");

        Theme::setupTheme(engine());

        setMinimumWidth(195);
        setMinimumHeight(195);

        // init the first load of the QML UI elements
        reloadQmlSource();
    }
    ~ConnectionViewQuickWidget() = default;

    static QString qmlSourcesPath()
    {
#ifdef SHARE_QML_PATH
        if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
            return QLatin1String(SHARE_QML_PATH) + "/connectionseditor";
#endif
        return Core::ICore::resourcePath("qmldesigner/connectionseditor").toString();
    }

private:
    void reloadQmlSource()
    {
        QString connectionEditorQmlFilePath = qmlSourcesPath() + QStringLiteral("/Main.qml");
        QTC_ASSERT(QFileInfo::exists(connectionEditorQmlFilePath), return );
        setSource(QUrl::fromLocalFile(connectionEditorQmlFilePath));

        if (!rootObject()) {
            QString errorString;
            for (const QQmlError &error : errors())
                errorString += "\n" + error.toString();

            Core::AsynchronousMessageBox::warning(
                Tr::tr("Cannot Create QtQuick View"),
                Tr::tr("ConnectionsEditorWidget: %1 cannot be created.%2")
                    .arg(qmlSourcesPath(), errorString));
            return;
        }
    }

private:
    QPointer<ConnectionView> m_connectionEditorView;
    QShortcut *m_qmlSourceUpdateShortcut;
};

ConnectionView::ConnectionView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_connectionModel(new ConnectionModel(this))
    , m_bindingModel(new BindingModel(this))
    , m_dynamicPropertiesModel(new DynamicPropertiesModel(false, this))
    , m_backendModel(new BackendModel(this))
    , m_connectionViewQuickWidget(new ConnectionViewQuickWidget(this))
{}

ConnectionView::~ConnectionView()
{
    // Ensure that QML is deleted first to avoid calling back to C++.
    delete m_connectionViewQuickWidget.data();
}
void ConnectionView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    bindingModel()->reset();
    dynamicPropertiesModel()->reset();
    connectionModel()->resetModel();
    backendModel()->resetModel();
}

void ConnectionView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    bindingModel()->reset();
    dynamicPropertiesModel()->reset();
    connectionModel()->resetModel();
    connectionModel()->modelAboutToBeDetached();
}

void ConnectionView::nodeCreated(const ModelNode & /*createdNode*/)
{
    connectionModel()->resetModel();
}

void ConnectionView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    connectionModel()->nodeAboutToBeRemoved(removedNode);
}

void ConnectionView::nodeRemoved(const ModelNode & /*removedNode*/,
                                 const NodeAbstractProperty & /*parentProperty*/,
                                 AbstractView::PropertyChangeFlags /*propertyChange*/)
{
     connectionModel()->resetModel();
}

void ConnectionView::nodeReparented(const ModelNode & /*node*/, const NodeAbstractProperty & /*newPropertyParent*/,
                               const NodeAbstractProperty & /*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    connectionModel()->resetModel();
}

void ConnectionView::nodeIdChanged(const ModelNode & /*node*/, const QString & /*newId*/, const QString & /*oldId*/)
{
    connectionModel()->resetModel();
    bindingModel()->reset();
    dynamicPropertiesModel()->reset();
}

void ConnectionView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const AbstractProperty &property : propertyList) {
        if (property.isDefaultProperty())
            connectionModel()->resetModel();

        dynamicPropertiesModel()->dispatchPropertyChanges(property);
    }
}

void ConnectionView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const AbstractProperty &property : propertyList) {
        if (property.isBindingProperty()) {
            bindingModel()->removeItem(property);
            dynamicPropertiesModel()->removeItem(property);
        } else if (property.isVariantProperty()) {
            dynamicPropertiesModel()->removeItem(property);
        } else if (property.isSignalHandlerProperty()) {
            connectionModel()->removeRowFromTable(property.toSignalHandlerProperty());
        }
    }
}

void ConnectionView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    for (const VariantProperty &variantProperty : propertyList) {
        if (variantProperty.isDynamic())
            dynamicPropertiesModel()->updateItem(variantProperty);
        if (variantProperty.isDynamic() && variantProperty.parentModelNode().isRootNode())
            backendModel()->resetModel();

        connectionModel()->variantPropertyChanged(variantProperty);

        dynamicPropertiesModel()->dispatchPropertyChanges(variantProperty);
    }
}

void ConnectionView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    for (const BindingProperty &bindingProperty : propertyList) {
        bindingModel()->updateItem(bindingProperty);
        if (bindingProperty.isDynamic())
            dynamicPropertiesModel()->updateItem(bindingProperty);
        if (bindingProperty.isDynamic() && bindingProperty.parentModelNode().isRootNode())
            backendModel()->resetModel();

        connectionModel()->bindingPropertyChanged(bindingProperty);

        dynamicPropertiesModel()->dispatchPropertyChanges(bindingProperty);
    }
}

void ConnectionView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &propertyList,
                                                    AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    for (const SignalHandlerProperty &signalHandlerProperty : propertyList)
        connectionModel()->abstractPropertyChanged(signalHandlerProperty);
}

void ConnectionView::selectedNodesChanged(const QList<ModelNode> & selectedNodeList,
                                     const QList<ModelNode> & /*lastSelectedNodeList*/)
{
    bindingModel()->reset(selectedNodeList);
    dynamicPropertiesModel()->reset();
}

void ConnectionView::importsChanged(const Imports & /*addedImports*/, const Imports & /*removedImports*/)
{
    backendModel()->resetModel();
}

void ConnectionView::currentStateChanged(const ModelNode &)
{
    dynamicPropertiesModel()->reset();
}

WidgetInfo ConnectionView::widgetInfo()
{
    return createWidgetInfo(m_connectionViewQuickWidget.data(),
                            QLatin1String("ConnectionView"),
                            WidgetInfo::LeftPane,
                            0,
                            tr("Connections"));
}

bool ConnectionView::hasWidget() const
{
    return true;
}

bool ConnectionView::isWidgetEnabled()
{
    return widgetInfo().widget->isEnabled();
}

ConnectionModel *ConnectionView::connectionModel() const
{
    return m_connectionModel;
}

BindingModel *ConnectionView::bindingModel() const
{
    return m_bindingModel;
}

DynamicPropertiesModel *ConnectionView::dynamicPropertiesModel() const
{
    return m_dynamicPropertiesModel;
}

BackendModel *ConnectionView::backendModel() const
{
    return m_backendModel;
}

int ConnectionView::currentIndex() const
{
    return m_currentIndex;
}

void ConnectionView::setCurrentIndex(int i)
{
    if (m_currentIndex == i)
        return;

    m_currentIndex = i;
    emit currentIndexChanged();
}

ConnectionView *ConnectionView::instance()
{
    static ConnectionView *s_instance = nullptr;

    if (s_instance)
        return s_instance;

    const auto views = QmlDesignerPlugin::instance()->viewManager().views();
    for (auto *view : views) {
        ConnectionView *myView = qobject_cast<ConnectionView*>(view);
        if (myView)
            s_instance =  myView;
    }

    QTC_ASSERT(s_instance, return nullptr);
    return s_instance;
}

} // namespace QmlDesigner
