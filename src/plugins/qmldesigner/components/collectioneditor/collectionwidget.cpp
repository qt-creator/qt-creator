// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionwidget.h"
#include "collectionsourcemodel.h"
#include "collectionview.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "singlecollectionmodel.h"
#include "theme.h"

#include <studioquickwidget.h>
#include <coreplugin/icore.h>

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMetaObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QShortcut>
#include <QVBoxLayout>

namespace {
QString collectionViewResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/collectionEditorQmlSource";
#endif
    return Core::ICore::resourcePath("qmldesigner/collectionEditorQmlSource").toString();
}
} // namespace

namespace QmlDesigner {
CollectionWidget::CollectionWidget(CollectionView *view)
    : QFrame()
    , m_view(view)
    , m_sourceModel(new CollectionSourceModel)
    , m_singleCollectionModel(new SingleCollectionModel)
    , m_quickWidget(new StudioQuickWidget(this))
{
    setWindowTitle(tr("Collection View", "Title of collection view widget"));

    Core::IContext *icontext = nullptr;
    Core::Context context(Constants::C_QMLMATERIALBROWSER);
    icontext = new Core::IContext(this);
    icontext->setContext(context);
    icontext->setWidget(this);

    m_quickWidget->quickWidget()->setObjectName(Constants::OBJECT_NAME_COLLECTION_EDITOR);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->engine()->addImportPath(collectionViewResourcesPath() + "/imports");
    m_quickWidget->setClearColor(Theme::getColor(Theme::Color::DSpanelBackground));

    Theme::setupTheme(m_quickWidget->engine());
    m_quickWidget->quickWidget()->installEventFilter(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_quickWidget.data());

    qmlRegisterAnonymousType<CollectionWidget>("CollectionEditorBackend", 1);
    auto map = m_quickWidget->registerPropertyMap("CollectionEditorBackend");
    map->setProperties(
        {{"rootView", QVariant::fromValue(this)},
         {"model", QVariant::fromValue(m_sourceModel.data())},
         {"singleCollectionModel", QVariant::fromValue(m_singleCollectionModel.data())}});

    auto hotReloadShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F4), this);
    connect(hotReloadShortcut, &QShortcut::activated, this, &CollectionWidget::reloadQmlSource);

    reloadQmlSource();
}

void CollectionWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (m_view)
        QmlDesignerPlugin::contextHelp(callback, m_view->contextHelpId());
    else
        callback({});
}

QPointer<CollectionSourceModel> CollectionWidget::sourceModel() const
{
    return m_sourceModel;
}

QPointer<SingleCollectionModel> CollectionWidget::singleCollectionModel() const
{
    return m_singleCollectionModel;
}

void CollectionWidget::reloadQmlSource()
{
    const QString collectionViewQmlPath = collectionViewResourcesPath() + "/CollectionView.qml";

    QTC_ASSERT(QFileInfo::exists(collectionViewQmlPath), return);

    m_quickWidget->setSource(QUrl::fromLocalFile(collectionViewQmlPath));
}

bool CollectionWidget::loadJsonFile(const QString &jsonFileAddress)
{
    if (!isJsonFile(jsonFileAddress))
        return false;

    QUrl jsonUrl(jsonFileAddress);
    QFileInfo fileInfo(jsonUrl.isLocalFile() ? jsonUrl.toLocalFile() : jsonUrl.toString());

    m_view->addResource(jsonUrl, fileInfo.completeBaseName(), "json");

    return true;
}

bool CollectionWidget::loadCsvFile(const QString &collectionName, const QString &csvFileAddress)
{
    QUrl csvUrl(csvFileAddress);
    m_view->addResource(csvUrl, collectionName, "csv");

    return true;
}

bool CollectionWidget::isJsonFile(const QString &jsonFileAddress) const
{
    QUrl jsonUrl(jsonFileAddress);
    QString fileAddress = jsonUrl.isLocalFile() ? jsonUrl.toLocalFile() : jsonUrl.toString();
    QFile file(fileAddress);

    if (!file.exists() || !file.open(QFile::ReadOnly))
        return false;

    QJsonParseError error;
    QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error)
        return false;

    return true;
}

bool CollectionWidget::isCsvFile(const QString &csvFileAddress) const
{
    QUrl csvUrl(csvFileAddress);
    QString fileAddress = csvUrl.isLocalFile() ? csvUrl.toLocalFile() : csvUrl.toString();
    QFile file(fileAddress);

    if (!file.exists())
        return false;

    // TODO: Evaluate the csv file
    return true;
}

bool CollectionWidget::addCollection([[maybe_unused]] const QString &collectionName) const
{
    // TODO
    return false;
}

void CollectionWidget::warn(const QString &title, const QString &body)
{
    QMetaObject::invokeMethod(m_quickWidget->rootObject(),
                              "showWarning",
                              Q_ARG(QVariant, title),
                              Q_ARG(QVariant, body));
}
} // namespace QmlDesigner
