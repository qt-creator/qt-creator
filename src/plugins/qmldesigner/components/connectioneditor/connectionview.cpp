// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "connectionview.h"

#include "bindingmodel.h"
#include "connectionmodel.h"
#include "dynamicpropertiesmodel.h"
#include "theme.h"

#include <bindingproperty.h>
#include <customnotifications.h>
#include <nodeabstractproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmldesignertr.h>
#include <signalhandlerproperty.h>
#include <variantproperty.h>
#include <viewmanager.h>

#include <studioquickwidget.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <utils/qtcassert.h>

#include <QQmlEngine>
#include <QShortcut>
#include <QTableView>

namespace QmlDesigner {

static QString resourcesPath(const QString &dir)
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/" + dir;
#endif
    return Core::ICore::resourcePath("qmldesigner/" + dir).toUrlishString();
}

static QString propertyEditorResourcesPath()
{
    return resourcesPath("propertyEditorQmlSources");
}

static QString scriptsEditorResourcesPath()
{
    return resourcesPath("scriptseditor");
}

static QString connectionsEditorResourcesPath()
{
    return resourcesPath("connectionseditor");
}

class ConnectionViewQuickWidget : public StudioQuickWidget
{
    // Q_OBJECT carefull

public:
    ConnectionViewQuickWidget(ConnectionView *connectionEditorView,
                              ConnectionModel *connectionModel,
                              BindingModel *bindingModel,
                              DynamicPropertiesModel *dynamicPropertiesModel)
        : m_connectionEditorView(connectionEditorView)

    {
        engine()->addImportPath(connectionsEditorResourcesPath());
        engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
        engine()->addImportPath(scriptsEditorResourcesPath() + "/imports");
        engine()->addImportPath(connectionsEditorResourcesPath() + "/imports");

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

        map->setProperties({{"connectionModel", QVariant::fromValue(connectionModel)}});

        map->setProperties({{"bindingModel", QVariant::fromValue(bindingModel)}});

        map->setProperties({{"dynamicPropertiesModel", QVariant::fromValue(dynamicPropertiesModel)}});

        qmlRegisterType<ConnectionModelBackendDelegate>("ConnectionsEditorEditorBackend",
                                                        1,
                                                        0,
                                                        "DynamicPropertiesModelBackendDelegate");

        Theme::setupTheme(engine());

        setMinimumSize(QSize(195, 195));

        // init the first load of the QML UI elements
        reloadQmlSource();
    }
    ~ConnectionViewQuickWidget() = default;

private:
    void reloadQmlSource()
    {
        QString connectionEditorQmlFilePath = connectionsEditorResourcesPath()
                                              + QStringLiteral("/Main.qml");
        QTC_ASSERT(QFileInfo::exists(connectionEditorQmlFilePath), return );
        setSource(QUrl::fromLocalFile(connectionEditorQmlFilePath));

        if (!rootObject()) {
            QString errorString;
            for (const QQmlError &error : errors())
                errorString += "\n" + error.toString();

            Core::AsynchronousMessageBox::warning(
                Tr::tr("Cannot Create QtQuick View"),
                Tr::tr("ConnectionsEditorWidget: %1 cannot be created.%2")
                    .arg(connectionsEditorResourcesPath(), errorString));
            return;
        }
    }

private:
    QPointer<ConnectionView> m_connectionEditorView;
    QShortcut *m_qmlSourceUpdateShortcut;
};

struct ConnectionView::ConnectionViewData
{
    ConnectionViewData(ConnectionView *view)
        : connectionModel{view}
        , bindingModel{view}
        , dynamicPropertiesModel{false, view}
        , connectionViewQuickWidget{Utils::makeUniqueObjectPtr<ConnectionViewQuickWidget>(
              view, &connectionModel, &bindingModel, &dynamicPropertiesModel)}
    {}

    ConnectionModel connectionModel;
    BindingModel bindingModel;
    DynamicPropertiesModel dynamicPropertiesModel;
    int currentIndex = 0;

    // Ensure that QML is deleted first to avoid calling back to C++.
    Utils::UniqueObjectPtr<ConnectionViewQuickWidget> connectionViewQuickWidget;
};

ConnectionView::ConnectionView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , d{std::make_unique<ConnectionViewData>(this)}
{}

ConnectionView::~ConnectionView()
{
}
void ConnectionView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    d->bindingModel.reset();
    d->dynamicPropertiesModel.reset();
    d->connectionModel.resetModel();
}

void ConnectionView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    d->bindingModel.reset();
    d->dynamicPropertiesModel.reset();
    d->connectionModel.resetModel();
    d->connectionModel.modelAboutToBeDetached();
}

void ConnectionView::nodeCreated(const ModelNode & /*createdNode*/)
{
    d->connectionModel.resetModel();
}

void ConnectionView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    d->connectionModel.nodeAboutToBeRemoved(removedNode);
}

void ConnectionView::nodeRemoved(const ModelNode & /*removedNode*/,
                                 const NodeAbstractProperty & /*parentProperty*/,
                                 AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    d->connectionModel.resetModel();
}

void ConnectionView::nodeReparented(const ModelNode & /*node*/, const NodeAbstractProperty & /*newPropertyParent*/,
                               const NodeAbstractProperty & /*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    d->connectionModel.resetModel();
}

void ConnectionView::nodeIdChanged(const ModelNode & /*node*/, const QString & /*newId*/, const QString & /*oldId*/)
{
    d->connectionModel.resetModel();
    d->bindingModel.reset();
    d->dynamicPropertiesModel.reset();
}

void ConnectionView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const AbstractProperty &property : propertyList) {
        if (property.isDefaultProperty())
            d->connectionModel.resetModel();

        d->dynamicPropertiesModel.dispatchPropertyChanges(property);
    }
}

void ConnectionView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const AbstractProperty &property : propertyList) {
        if (property.isBindingProperty()) {
            d->bindingModel.removeItem(property);
            d->dynamicPropertiesModel.removeItem(property);
        } else if (property.isVariantProperty()) {
            d->dynamicPropertiesModel.removeItem(property);
        } else if (property.isSignalHandlerProperty()) {
            d->connectionModel.removeRowFromTable(property.toSignalHandlerProperty());
        }
    }
}

void ConnectionView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    for (const VariantProperty &variantProperty : propertyList) {
        if (variantProperty.isDynamic())
            d->dynamicPropertiesModel.updateItem(variantProperty);

        d->connectionModel.variantPropertyChanged(variantProperty);

        d->dynamicPropertiesModel.dispatchPropertyChanges(variantProperty);
    }
}

void ConnectionView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    for (const BindingProperty &bindingProperty : propertyList) {
        d->bindingModel.updateItem(bindingProperty);
        if (bindingProperty.isDynamic())
            d->dynamicPropertiesModel.updateItem(bindingProperty);

        d->connectionModel.bindingPropertyChanged(bindingProperty);

        d->dynamicPropertiesModel.dispatchPropertyChanges(bindingProperty);
    }
}

void ConnectionView::signalDeclarationPropertiesChanged(
    const QVector<SignalDeclarationProperty> &propertyList, PropertyChangeFlags /* propertyChange */)
{
    for (const SignalDeclarationProperty &property : propertyList)
        d->dynamicPropertiesModel.updateItem(property);
}

void ConnectionView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &propertyList,
                                                    AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    for (const SignalHandlerProperty &signalHandlerProperty : propertyList)
        d->connectionModel.abstractPropertyChanged(signalHandlerProperty);
}

void ConnectionView::selectedNodesChanged(const QList<ModelNode> & selectedNodeList,
                                     const QList<ModelNode> & /*lastSelectedNodeList*/)
{
    d->bindingModel.reset(selectedNodeList);
    d->dynamicPropertiesModel.reset();
}

void ConnectionView::currentStateChanged(const ModelNode &)
{
    d->dynamicPropertiesModel.reset();
}

WidgetInfo ConnectionView::widgetInfo()
{
    return createWidgetInfo(d->connectionViewQuickWidget.get(),
                            QLatin1String("ConnectionView"),
                            WidgetInfo::LeftPane,

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
    return &d->connectionModel;
}

BindingModel *ConnectionView::bindingModel() const
{
    return &d->bindingModel;
}

DynamicPropertiesModel *ConnectionView::dynamicPropertiesModel() const
{
    return &d->dynamicPropertiesModel;
}

int ConnectionView::currentIndex() const
{
    return d->currentIndex;
}

void ConnectionView::setCurrentIndex(int i)
{
    if (d->currentIndex == i)
        return;

    d->currentIndex = i;
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

void ConnectionView::customNotification(const AbstractView *,
                                        const QString &identifier,
                                        const QList<ModelNode> &nodeList,
                                        const QList<QVariant> &data)
{
    if (identifier == AddConnectionNotification) {
        QTC_ASSERT(data.count() == 1, return );

        const PropertyName name = data.first().toString().toUtf8();
        d->connectionModel.addConnection(name);
        d->connectionModel.showPopup();
    } else if (identifier == EditConnectionNotification) {
        QTC_ASSERT(nodeList.count() == 1, return );
        ModelNode modelNode = nodeList.first();

        QTC_ASSERT(data.count() == 1, return );

        const PropertyName name = data.first().toByteArray();

        QTC_ASSERT(modelNode.hasSignalHandlerProperty(name), return );

        d->connectionModel.selectProperty(modelNode.signalHandlerProperty(name));
        d->connectionModel.showPopup();
    }
}

} // namespace QmlDesigner
