// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionwidget.h"

#include "collectiondetails.h"
#include "collectiondetailsmodel.h"
#include "collectiondetailssortfiltermodel.h"
#include "collectioneditorutils.h"
#include "collectionlistmodel.h"
#include "collectionview.h"
#include "designmodewidget.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "theme.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <studioquickwidget.h>

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

QString getPreferredCollectionName(const QUrl &url, const QString &collectionName)
{
    if (collectionName.isEmpty()) {
        QFileInfo fileInfo(url.isLocalFile() ? url.toLocalFile() : url.toString());
        return fileInfo.completeBaseName();
    }

    return collectionName;
}

} // namespace

namespace QmlDesigner {

CollectionWidget::CollectionWidget(CollectionView *view)
    : m_view(view)
    , m_listModel(new CollectionListModel)
    , m_collectionDetailsModel(new CollectionDetailsModel)
    , m_collectionDetailsSortFilterModel(std::make_unique<CollectionDetailsSortFilterModel>())
    , m_quickWidget(new StudioQuickWidget(this))
{
    setWindowTitle(tr("Model Editor", "Title of model editor widget"));

    Core::Context context(Constants::C_QMLCOLLECTIONEDITOR);
    m_iContext = new Core::IContext(this);
    m_iContext->setContext(context);
    m_iContext->setWidget(this);
    Core::ICore::addContextObject(m_iContext);

    connect(m_listModel, &CollectionListModel::warning, this, &CollectionWidget::warn);

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
        {"model", QVariant::fromValue(m_listModel.data())},
        {"collectionDetailsModel", QVariant::fromValue(m_collectionDetailsModel.data())},
        {"collectionDetailsSortFilterModel",
         QVariant::fromValue(m_collectionDetailsSortFilterModel.get())},
    });

    auto hotReloadShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F4), this);
    connect(hotReloadShortcut, &QShortcut::activated, this, &CollectionWidget::reloadQmlSource);

    reloadQmlSource();

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_MODELEDITOR_TIME);
}

CollectionWidget::~CollectionWidget() = default;

void CollectionWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (m_view)
        QmlDesignerPlugin::contextHelp(callback, m_view->contextHelpId());
    else
        callback({});
}

QPointer<CollectionListModel> CollectionWidget::listModel() const
{
    return m_listModel;
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

    if (!m_quickWidget->rootObject()) {
        QString errorString;
        const auto errors = m_quickWidget->errors();
        for (const QQmlError &error : errors)
            errorString.append("\n" + error.toString());

        Core::AsynchronousMessageBox::warning(tr("Cannot Create QtQuick View"),
                                              tr("StatesEditorWidget: %1 cannot be created.%2")
                                                  .arg(collectionViewQmlPath, errorString));
        return;
    }
}

QSize CollectionWidget::minimumSizeHint() const
{
    return {300, 300};
}

bool CollectionWidget::loadJsonFile(const QUrl &url, const QString &collectionName)
{
    if (!isJsonFile(url))
        return false;

    m_view->addResource(url, getPreferredCollectionName(url, collectionName));

    return true;
}

bool CollectionWidget::loadCsvFile(const QUrl &url, const QString &collectionName)
{
    m_view->addResource(url, getPreferredCollectionName(url, collectionName));

    return true;
}

bool CollectionWidget::isJsonFile(const QUrl &url) const
{
    Utils::FilePath filePath = Utils::FilePath::fromUserInput(url.isLocalFile() ? url.toLocalFile()
                                                                                : url.toString());
    Utils::FileReader file;
    if (!file.fetch(filePath))
        return false;

    QJsonParseError error;
    QJsonDocument::fromJson(file.data(), &error);
    if (error.error)
        return false;

    return true;
}

bool CollectionWidget::isCsvFile(const QUrl &url) const
{
    QString filePath = url.isLocalFile() ? url.toLocalFile() : url.toString();
    QFileInfo fileInfo(filePath);
    return fileInfo.exists() && !fileInfo.suffix().compare("csv", Qt::CaseInsensitive);
}

bool CollectionWidget::isValidUrlToImport(const QUrl &url) const
{
    using Utils::FilePath;
    FilePath fileInfo = FilePath::fromUserInput(url.isLocalFile() ? url.toLocalFile()
                                                                  : url.toString());
    if (fileInfo.suffix() == "json")
        return isJsonFile(url);

    if (fileInfo.suffix() == "csv")
        return isCsvFile(url);

    return false;
}

bool CollectionWidget::importFile(const QString &collectionName,
                                  const QUrl &url,
                                  const bool &firstRowIsHeader)
{
    using Utils::FilePath;

    FilePath fileInfo = FilePath::fromUserInput(url.isLocalFile() ? url.toLocalFile()
                                                                  : url.toString());
    CollectionDetails loadedCollection;
    QByteArray fileContent;

    auto loadUrlContent = [&]() -> bool {
        Utils::FileReader file;
        if (file.fetch(fileInfo)) {
            fileContent = file.data();
            return true;
        }

        warn(tr("Import from file"), tr("Cannot import from file \"%1\"").arg(fileInfo.fileName()));
        return false;
    };

    if (fileInfo.suffix() == "json") {
        if (!loadUrlContent())
            return false;

        QJsonParseError parseError;
        loadedCollection = CollectionDetails::fromImportedJson(fileContent, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            warn(tr("Json file Import error"),
                 tr("Cannot parse json content\n%1").arg(parseError.errorString()));
        }
    } else if (fileInfo.suffix() == "csv") {
        if (!loadUrlContent())
            return false;
        loadedCollection = CollectionDetails::fromImportedCsv(fileContent, firstRowIsHeader);
    }

    if (loadedCollection.columns()) {
        m_view->addNewCollection(collectionName, loadedCollection.toLocalJson());
        return true;
    } else {
        warn(tr("Can not add a model to the JSON file"),
             tr("The imported model is empty or is not supported."));
    }
    return false;
}

void CollectionWidget::addProjectImport()
{
    m_view->addProjectImport();
}

void CollectionWidget::addCollectionToDataStore(const QString &collectionName)
{
    m_view->addNewCollection(collectionName, CollectionEditorUtils::defaultCollection());
}

void CollectionWidget::assignCollectionToSelectedNode(const QString collectionName)
{
    m_view->assignCollectionToSelectedNode(collectionName);
}

void CollectionWidget::openCollection(const QString &collectionName)
{
    m_listModel->selectCollectionName(collectionName);
    QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("CollectionEditor", true);
}

ModelNode CollectionWidget::dataStoreNode() const
{
    return m_view->dataStoreNode();
}

void CollectionWidget::warn(const QString &title, const QString &body)
{
    QMetaObject::invokeMethod(m_quickWidget->rootObject(),
                              "showWarning",
                              Q_ARG(QVariant, title),
                              Q_ARG(QVariant, body));
}

void CollectionWidget::setTargetNodeSelected(bool selected)
{
    if (m_targetNodeSelected == selected)
        return;

    m_targetNodeSelected = selected;
    emit targetNodeSelectedChanged(m_targetNodeSelected);
}

void CollectionWidget::setProjectImportExists(bool exists)
{
    if (m_projectImportExists == exists)
        return;

    m_projectImportExists = exists;
    emit projectImportExistsChanged(m_projectImportExists);
}

void CollectionWidget::setDataStoreExists(bool exists)
{
    if (m_dataStoreExists == exists)
        return;

    m_dataStoreExists = exists;
    emit dataStoreExistsChanged(m_dataStoreExists);
}

void CollectionWidget::deleteSelectedCollection()
{
    QMetaObject::invokeMethod(m_quickWidget->quickWidget()->rootObject(), "deleteSelectedCollection");
}

} // namespace QmlDesigner
