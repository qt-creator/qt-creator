// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionwidget.h"

#include "collectiondetails.h"
#include "collectiondetailsmodel.h"
#include "collectiondetailssortfiltermodel.h"
#include "collectioneditorutils.h"
#include "collectionsourcemodel.h"
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
    : QFrame()
    , m_view(view)
    , m_sourceModel(new CollectionSourceModel)
    , m_collectionDetailsModel(new CollectionDetailsModel)
    , m_collectionDetailsSortFilterModel(std::make_unique<CollectionDetailsSortFilterModel>())
    , m_quickWidget(new StudioQuickWidget(this))
{
    setWindowTitle(tr("Model Editor", "Title of model editor widget"));

    Core::IContext *icontext = nullptr;
    Core::Context context(Constants::C_QMLMATERIALBROWSER);
    icontext = new Core::IContext(this);
    icontext->setContext(context);
    icontext->setWidget(this);

    connect(m_sourceModel, &CollectionSourceModel::warning, this, &CollectionWidget::warn);

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

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_MODELEDITOR_TIME);
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

    m_view->addResource(url, getPreferredCollectionName(url, collectionName), "json");

    return true;
}

bool CollectionWidget::loadCsvFile(const QUrl &url, const QString &collectionName)
{
    m_view->addResource(url, getPreferredCollectionName(url, collectionName), "csv");

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

bool CollectionWidget::importFile(const QString &collectionName, const QUrl &url)
{
    using Utils::FilePath;
    m_view->ensureDataStoreExists();

    const ModelNode node = dataStoreNode();
    if (!node.isValid()) {
        warn(tr("Can not import to the main model"), tr("The data store is not available."));
        return false;
    }

    FilePath fileInfo = FilePath::fromUserInput(url.isLocalFile() ? url.toLocalFile()
                                                                  : url.toString());
    bool added = false;
    QString errorMsg;
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
        loadedCollection = CollectionDetails::fromImportedCsv(fileContent);
    }

    if (loadedCollection.columns()) {
        const QString newCollectionName = generateUniqueCollectionName(node, collectionName);
        added = m_sourceModel->addCollectionToSource(node,
                                                     newCollectionName,
                                                     loadedCollection.toLocalJson(),
                                                     &errorMsg);
    } else {
        errorMsg = tr("The imported model is empty or is not supported.");
    }

    if (!added)
        warn(tr("Can not add a model to the JSON file"), errorMsg);

    return added;
}

bool CollectionWidget::addCollectionToDataStore(const QString &collectionName)
{
    m_view->ensureDataStoreExists();
    const ModelNode node = dataStoreNode();
    if (!node.isValid()) {
        warn(tr("Can not import to the main model"), tr("The default model node is not available."));
        return false;
    }

    QString errorMsg;
    bool added = m_sourceModel->addCollectionToSource(node,
                                                      generateUniqueCollectionName(node,
                                                                                   collectionName),
                                                      CollectionEditorUtils::defaultCollection(),
                                                      &errorMsg);
    if (!added)
        warn(tr("Failed to add a model to the default model group"), errorMsg);

    return added;
}

void CollectionWidget::assignCollectionToSelectedNode(const QString collectionName)
{
    m_view->assignCollectionToSelectedNode(collectionName);
}

void CollectionWidget::openCollection(const QString &collectionName)
{
    m_sourceModel->selectCollection(QVariant::fromValue(m_view->dataStoreNode()), collectionName);
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

QString CollectionWidget::generateUniqueCollectionName(const ModelNode &node, const QString &name)
{
    if (!m_sourceModel->collectionExists(node, name))
        return name;

    static QRegularExpression reg("^(?<mainName>[\\w\\d\\.\\_\\-]+)\\_(?<number>\\d+)$");
    QRegularExpressionMatch match = reg.match(name);
    if (match.hasMatch()) {
        int nextNumber = match.captured("number").toInt() + 1;
        return generateUniqueCollectionName(
            node, QString("%1_%2").arg(match.captured("mainName")).arg(nextNumber));
    } else {
        return generateUniqueCollectionName(node, QString("%1_1").arg(name));
    }
}

} // namespace QmlDesigner
