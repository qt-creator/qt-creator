// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionwidget.h"

#include "collectiondetailsmodel.h"
#include "collectiondetailssortfiltermodel.h"
#include "collectioneditorutils.h"
#include "collectionsourcemodel.h"
#include "collectionview.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "theme.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <studioquickwidget.h>

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
    return {300, 400};
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
    QString filePath = url.isLocalFile() ? url.toLocalFile() : url.toString();
    QFile file(filePath);

    if (!file.exists() || !file.open(QFile::ReadOnly))
        return false;

    QJsonParseError error;
    QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error)
        return false;

    return true;
}

bool CollectionWidget::isCsvFile(const QUrl &url) const
{
    QString filePath = url.isLocalFile() ? url.toLocalFile() : url.toString();
    QFile file(filePath);

    return file.exists() && file.fileName().endsWith(".csv");
}

bool CollectionWidget::addCollection(const QString &collectionName,
                                     const QString &collectionType,
                                     const QUrl &sourceUrl,
                                     const QVariant &sourceNode)
{
    const ModelNode node = sourceNode.value<ModelNode>();
    bool isNewCollection = !node.isValid();

    if (isNewCollection) {
        QString sourcePath = sourceUrl.isLocalFile() ? sourceUrl.toLocalFile() : sourceUrl.toString();

        if (collectionType == "json") {
            QJsonObject jsonObject;
            jsonObject.insert(collectionName, CollectionEditor::defaultCollectionArray());

            QFile sourceFile(sourcePath);
            if (!sourceFile.open(QFile::WriteOnly)) {
                warn(tr("File error"),
                     tr("Can not open the file to write.\n") + sourceFile.errorString());
                return false;
            }

            sourceFile.write(QJsonDocument(jsonObject).toJson());
            sourceFile.close();

            bool loaded = loadJsonFile(sourcePath, collectionName);
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

            sourceFile.write("Column1\n\n");
            sourceFile.close();

            bool loaded = loadCsvFile(sourcePath, collectionName);
            if (!loaded)
                sourceFile.remove();

            return loaded;
        } else if (collectionType == "existing") {
            QFileInfo fileInfo(sourcePath);
            if (fileInfo.suffix() == "json")
                return loadJsonFile(sourcePath, collectionName);
            else if (fileInfo.suffix() == "csv")
                return loadCsvFile(sourcePath, collectionName);
        }
    } else if (collectionType == "json") {
        QString errorMsg;
        bool added = m_sourceModel->addCollectionToSource(node, collectionName, &errorMsg);
        if (!added)
            warn(tr("Can not add a model to the JSON file"), errorMsg);
        return added;
    }

    return false;
}

void CollectionWidget::assignSourceNodeToSelectedItem(const QVariant &sourceNode)
{
    ModelNode sourceModel = sourceNode.value<ModelNode>();
    ModelNode targetNode = m_view->singleSelectedModelNode();

    QTC_ASSERT(sourceModel.isValid() && targetNode.isValid(), return);

    if (sourceModel.id().isEmpty()) {
        warn(tr("Assigning the model group"),
             tr("The model group must have a valid id to be assigned."));
        return;
    }

    CollectionEditor::assignCollectionSourceToNode(m_view, targetNode, sourceModel);
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

} // namespace QmlDesigner
