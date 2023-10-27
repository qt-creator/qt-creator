// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionwidget.h"

#include "collectiondetailsmodel.h"
#include "collectiondetailssortfiltermodel.h"
#include "collectionsourcemodel.h"
#include "collectionview.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "theme.h"

#include <studioquickwidget.h>
#include <coreplugin/icore.h>

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

static QString urlToLocalPath(const QUrl &url)
{
    QString localPath;

    if (url.isLocalFile())
        localPath = url.toLocalFile();

    if (url.scheme() == QLatin1String("qrc")) {
        const QString &path = url.path();
        localPath = QStringLiteral(":") + path;
    }

    return localPath;
}

} // namespace

namespace QmlDesigner {
CollectionWidget::CollectionWidget(CollectionView *view)
    : QFrame()
    , m_view(view)
    , m_sourceModel(new CollectionSourceModel)
    , m_collectionDetailsModel(new CollectionDetailsModel)
    , m_collectionDetailsSortFilterModel(std::make_unique<CollectionDetailsSortFilterModel>())
    , m_quickWidget(new StudioQuickWidget(this))
{
    setWindowTitle(tr("Collection View", "Title of collection view widget"));

    Core::IContext *icontext = nullptr;
    Core::Context context(Constants::C_QMLMATERIALBROWSER);
    icontext = new Core::IContext(this);
    icontext->setContext(context);
    icontext->setWidget(this);

    m_collectionDetailsSortFilterModel->setSourceModel(m_collectionDetailsModel);

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
    map->setProperties({
        {"rootView", QVariant::fromValue(this)},
        {"model", QVariant::fromValue(m_sourceModel.data())},
        {"collectionDetailsModel", QVariant::fromValue(m_collectionDetailsModel.data())},
        {"collectionDetailsSortFilterModel",
         QVariant::fromValue(m_collectionDetailsSortFilterModel.get())},
    });

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

QPointer<CollectionDetailsModel> CollectionWidget::collectionDetailsModel() const
{
    return m_collectionDetailsModel;
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

bool CollectionWidget::addCollection(const QString &collectionName,
                                     const QString &collectionType,
                                     const QString &sourceAddress,
                                     const QVariant &sourceNode)
{
    const ModelNode node = sourceNode.value<ModelNode>();
    bool isNewCollection = !node.isValid();

    if (isNewCollection) {
        QString sourcePath = ::urlToLocalPath(sourceAddress);
        if (collectionType == "json") {
            QJsonObject jsonObject;
            jsonObject.insert(collectionName, QJsonArray());

            QFile sourceFile(sourcePath);
            if (!sourceFile.open(QFile::WriteOnly)) {
                warn(tr("File error"),
                     tr("Can not open the file to write.\n") + sourceFile.errorString());
                return false;
            }

            sourceFile.write(QJsonDocument(jsonObject).toJson());
            sourceFile.close();

            bool loaded = loadJsonFile(sourcePath);
            if (!loaded)
                sourceFile.remove();

            return loaded;
        } else if (collectionType == "csv") {
            QFile sourceFile(sourcePath);
            if (!sourceFile.open(QFile::WriteOnly)) {
                warn(tr("File error"),
                     tr("Can not open the file to write.\n") + sourceFile.errorString());
                return false;
            }

            sourceFile.close();

            bool loaded = loadCsvFile(collectionName, sourcePath);
            if (!loaded)
                sourceFile.remove();

            return loaded;
        } else if (collectionType == "existing") {
            QFileInfo fileInfo(sourcePath);
            if (fileInfo.suffix() == "json")
                return loadJsonFile(sourcePath);
            else if (fileInfo.suffix() == "csv")
                return loadCsvFile(collectionName, sourcePath);
        }
    } else if (collectionType == "json") {
        QString errorMsg;
        bool added = m_sourceModel->addCollectionToSource(node, collectionName, &errorMsg);
        if (!added)
            warn(tr("Can not add a collection to the json file"), errorMsg);
        return added;
    }

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
