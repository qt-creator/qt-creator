/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "itemlibrarywidget.h"

#include "customfilesystemmodel.h"
#include "itemlibraryiconimageprovider.h"
#include "itemlibraryimport.h"

#include <theme.h>

#include <designeractionmanager.h>
#include <designermcumanager.h>
#include <itemlibraryimageprovider.h>
#include <itemlibraryinfo.h>
#include <itemlibrarymodel.h>
#include <itemlibraryaddimportmodel.h>
#include <metainfo.h>
#include <model.h>
#include <rewritingexception.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>
#include <utils/flowlayout.h>
#include <utils/fileutils.h>
#include <utils/stylehelper.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <QApplication>
#include <QDrag>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QImageReader>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QShortcut>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QWheelEvent>
#include <QQmlContext>
#include <QQuickItem>

namespace QmlDesigner {

static QString propertyEditorResourcesPath() {
    return Core::ICore::resourcePath() + QStringLiteral("/qmldesigner/propertyEditorQmlSources");
}

bool ItemLibraryWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        if (obj == m_itemViewQuickWidget.data())
            QMetaObject::invokeMethod(m_itemViewQuickWidget->rootObject(), "closeContextMenu");
    } else if (event->type() == QMouseEvent::MouseMove) {
        if (m_itemToDrag.isValid()) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if ((me->globalPos() - m_dragStartPoint).manhattanLength() > 10) {
                ItemLibraryEntry entry = m_itemToDrag.value<ItemLibraryEntry>();
                // For drag to be handled correctly, we must have the component properly imported
                // beforehand, so we import the module immediately when the drag starts
                if (!entry.requiredImport().isEmpty()) {
                    Import import = Import::createLibraryImport(entry.requiredImport());
                    if (!m_model->hasImport(import, true, true)) {
                        const QList<Import> possImports = m_model->possibleImports();
                        for (const auto &possImport : possImports) {
                            if (possImport.url() == import.url()) {
                                m_model->changeImports({possImport}, {});
                                break;
                            }
                        }
                    }
                }

                auto drag = new QDrag(this);
                drag->setPixmap(Utils::StyleHelper::dpiSpecificImageFile(entry.libraryEntryIconPath()));
                drag->setMimeData(m_itemLibraryModel->getMimeData(entry));
                drag->exec();
                drag->deleteLater();

                m_itemToDrag = {};
            }
        }
    } else if (event->type() == QMouseEvent::MouseButtonRelease) {
        m_itemToDrag = {};
    }

    return QObject::eventFilter(obj, event);
}

ItemLibraryWidget::ItemLibraryWidget(AsynchronousImageCache &imageCache,
                                     AsynchronousImageCache &asynchronousFontImageCache,
                                     SynchronousImageCache &synchronousFontImageCache)
    : m_itemIconSize(24, 24)
    , m_itemLibraryModel(new ItemLibraryModel(this))
    , m_itemLibraryAddImportModel(new ItemLibraryAddImportModel(this))
    , m_resourcesFileSystemModel{new CustomFileSystemModel(synchronousFontImageCache, this)}
    , m_headerWidget(new QQuickWidget(this))
    , m_addImportWidget(new QQuickWidget(this))
    , m_itemViewQuickWidget(new QQuickWidget(this))
    , m_resourcesView(new ItemLibraryResourceView(asynchronousFontImageCache, this))
    , m_imageCache{imageCache}
{
    m_compressionTimer.setInterval(200);
    m_compressionTimer.setSingleShot(true);
    ItemLibraryModel::registerQmlTypes();

    setWindowTitle(tr("Library", "Title of library view"));
    setMinimumWidth(100);

    // create header widget
    m_headerWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    Theme::setupTheme(m_headerWidget->engine());
    m_headerWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_headerWidget->setClearColor(Theme::getColor(Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));
    m_headerWidget->rootContext()->setContextProperty("rootView", QVariant::fromValue(this));

    // create add imports widget
    m_addImportWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_addImportWidget->setClearColor(Theme::getColor(Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));
    Theme::setupTheme(m_addImportWidget->engine());
    m_addImportWidget->rootContext()->setContextProperties({
        {"addImportModel", QVariant::fromValue(m_itemLibraryAddImportModel.data())},
        {"rootView", QVariant::fromValue(this)},
    });

    // set up Item Library view and model
    m_itemViewQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_itemViewQuickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");

    m_itemViewQuickWidget->rootContext()->setContextProperties(QVector<QQmlContext::PropertyPair>{
        {{"itemLibraryModel"}, QVariant::fromValue(m_itemLibraryModel.data())},
        {{"itemLibraryIconWidth"}, m_itemIconSize.width()},
        {{"itemLibraryIconHeight"}, m_itemIconSize.height()},
        {{"rootView"}, QVariant::fromValue(this)},
        {{"highlightColor"}, Utils::StyleHelper::notTooBrightHighlightColor()},
    });

    m_previewTooltipBackend = std::make_unique<PreviewTooltipBackend>(m_imageCache);
    m_itemViewQuickWidget->rootContext()->setContextProperty("tooltipBackend",
                                                             m_previewTooltipBackend.get());

    m_itemViewQuickWidget->setClearColor(Theme::getColor(Theme::Color::QmlDesigner_ButtonColor));
    m_itemViewQuickWidget->engine()->addImageProvider(QStringLiteral("qmldesigner_itemlibrary"),
                                                      new Internal::ItemLibraryImageProvider);
    Theme::setupTheme(m_itemViewQuickWidget->engine());
    m_itemViewQuickWidget->installEventFilter(this);

    // connect Resources view and its model
    m_resourcesView->setModel(m_resourcesFileSystemModel.data());

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(m_itemViewQuickWidget.data());
    m_stackedWidget->addWidget(m_resourcesView.data());
    m_stackedWidget->addWidget(m_addImportWidget.data());
    m_stackedWidget->setMinimumHeight(30);
    m_stackedWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_headerWidget.data());
    layout->addWidget(m_stackedWidget.data());

    updateSearch();

    /* style sheets */
    setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/stylesheet.css")))));
    m_resourcesView->setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css")))));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F5), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &ItemLibraryWidget::reloadQmlSource);

    connect(&m_compressionTimer, &QTimer::timeout, this, &ItemLibraryWidget::updateModel);

    const auto dropSupport = new Utils::DropSupport(
                m_resourcesView.data(), [this](QDropEvent *event, Utils::DropSupport *) {
        // Accept supported file types
        if (event->type() == QDropEvent::DragEnter && !Utils::DropSupport::isFileDrop(event))
            return false; // do not accept drops without files
        bool accept = false;
        const QSet<QString> &suffixes = m_resourcesFileSystemModel->supportedSuffixes();
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            QFileInfo fi(url.toLocalFile());
            if (suffixes.contains(fi.suffix().toLower())) {
                accept = true;
                break;
            }
        }
        return accept;
    });
    connect(dropSupport, &Utils::DropSupport::filesDropped,
            this, &ItemLibraryWidget::importDroppedFiles);

    m_itemViewQuickWidget->engine()->addImageProvider("itemlibrary_preview",
                                                      new ItemLibraryIconImageProvider{m_imageCache});

    // init the first load of the QML UI elements
    reloadQmlSource();
}

ItemLibraryWidget::~ItemLibraryWidget() = default;

void ItemLibraryWidget::setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo)
{
    if (m_itemLibraryInfo.data() == itemLibraryInfo)
        return;

    if (m_itemLibraryInfo) {
        disconnect(m_itemLibraryInfo.data(), &ItemLibraryInfo::entriesChanged,
                   this, &ItemLibraryWidget::delayedUpdateModel);
        disconnect(m_itemLibraryInfo.data(), &ItemLibraryInfo::priorityImportsChanged,
                   this, &ItemLibraryWidget::handlePriorityImportsChanged);
    }
    m_itemLibraryInfo = itemLibraryInfo;
    if (itemLibraryInfo) {
        connect(m_itemLibraryInfo.data(), &ItemLibraryInfo::entriesChanged,
                this, &ItemLibraryWidget::delayedUpdateModel);
        connect(m_itemLibraryInfo.data(), &ItemLibraryInfo::priorityImportsChanged,
                this, &ItemLibraryWidget::handlePriorityImportsChanged);
        m_itemLibraryAddImportModel->setPriorityImports(m_itemLibraryInfo->priorityImports());
    }
    delayedUpdateModel();
}

QList<QToolButton *> ItemLibraryWidget::createToolBarWidgets()
{
//    TODO: implement
    QList<QToolButton *> buttons;
    return buttons;
}

void ItemLibraryWidget::handleSearchfilterChanged(const QString &filterText)
{
    m_filterText = filterText;

    updateSearch();
}

void ItemLibraryWidget::handleAddModule()
{
    QMetaObject::invokeMethod(m_headerWidget->rootObject(), "setTab", Q_ARG(QVariant, 0));
    handleTabChanged(2);
}

void ItemLibraryWidget::handleAddAsset()
{
    addResources({});
}

void ItemLibraryWidget::handleAddImport(int index)
{
    Import import = m_itemLibraryAddImportModel->getImportAt(index);
    if (import.isLibraryImport() && (import.url().startsWith("QtQuick")
                                     || import.url().startsWith("SimulinkConnector"))) {
        QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_IMPORT_ADDED
                                               + import.toImportString());
    }

    m_model->changeImports({import}, {});
    QmlDesignerPlugin::instance()->currentDesignDocument()->updateSubcomponentManager();
    m_stackedWidget->setCurrentIndex(0); // switch to the Components view after import is added
    updateSearch();
}

void ItemLibraryWidget::delayedUpdateModel()
{
    static bool disableTimer = DesignerSettings::getValue(DesignerSettingsKey::DISABLE_ITEM_LIBRARY_UPDATE_TIMER).toBool();
    if (disableTimer)
        updateModel();
    else
        m_compressionTimer.start();
}

void ItemLibraryWidget::setModel(Model *model)
{
    m_model = model;
    if (!model)
        return;

    setItemLibraryInfo(model->metaInfo().itemLibraryInfo());
}

void ItemLibraryWidget::handleTabChanged(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    updateSearch();
}

QString ItemLibraryWidget::qmlSourcesPath()
{
    return Core::ICore::resourcePath() + QStringLiteral("/qmldesigner/itemLibraryQmlSources");
}

void ItemLibraryWidget::clearSearchFilter()
{
    QMetaObject::invokeMethod(m_headerWidget->rootObject(), "clearSearchFilter");
}

void ItemLibraryWidget::reloadQmlSource()
{
    const QString libraryHeaderQmlPath = qmlSourcesPath() + "/LibraryHeader.qml";
    QTC_ASSERT(QFileInfo::exists(libraryHeaderQmlPath), return);
    m_headerWidget->engine()->clearComponentCache();
    m_headerWidget->setSource(QUrl::fromLocalFile(libraryHeaderQmlPath));

    const QString addImportQmlPath = qmlSourcesPath() + "/AddImport.qml";
    QTC_ASSERT(QFileInfo::exists(addImportQmlPath), return);
    m_addImportWidget->engine()->clearComponentCache();
    m_addImportWidget->setSource(QUrl::fromLocalFile(addImportQmlPath));

    const QString itemLibraryQmlPath = qmlSourcesPath() + "/ItemsView.qml";
    QTC_ASSERT(QFileInfo::exists(itemLibraryQmlPath), return);
    m_itemViewQuickWidget->engine()->clearComponentCache();
    m_itemViewQuickWidget->setSource(QUrl::fromLocalFile(itemLibraryQmlPath));
}

void ItemLibraryWidget::updateModel()
{
    QTC_ASSERT(m_itemLibraryModel, return);

    if (m_compressionTimer.isActive()) {
        m_updateRetry = false;
        m_compressionTimer.stop();
    }

    m_itemLibraryModel->update(m_itemLibraryInfo.data(), m_model.data());

    if (m_itemLibraryModel->rowCount() == 0 && !m_updateRetry) {
        m_updateRetry = true; // Only retry once to avoid endless loops
        m_compressionTimer.start();
    } else {
        m_updateRetry = false;
    }
    updateSearch();
}

void ItemLibraryWidget::updatePossibleImports(const QList<Import> &possibleImports)
{
    m_itemLibraryAddImportModel->update(possibleImports);
    delayedUpdateModel();
}

void ItemLibraryWidget::updateUsedImports(const QList<Import> &usedImports)
{
    m_itemLibraryModel->updateUsedImports(usedImports);
}

void ItemLibraryWidget::updateSearch()
{
    if (m_stackedWidget->currentIndex() == 0) { // Item Library tab selected
        m_itemLibraryModel->setSearchText(m_filterText);
        m_itemViewQuickWidget->update();
    } else if (m_stackedWidget->currentIndex() == 1) { // Assets tab selected
        m_resourcesFileSystemModel->setSearchFilter(m_filterText);
        m_resourcesFileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        m_resourcesView->scrollToTop();
    } else if (m_stackedWidget->currentIndex() == 2) {  // QML imports tab selected
        m_itemLibraryAddImportModel->setSearchText(m_filterText);
    }
}

void ItemLibraryWidget::handlePriorityImportsChanged()
{
    if (!m_itemLibraryInfo.isNull()) {
        m_itemLibraryAddImportModel->setPriorityImports(m_itemLibraryInfo->priorityImports());
        m_itemLibraryAddImportModel->update(m_model->possibleImports());
    }
}

void ItemLibraryWidget::setResourcePath(const QString &resourcePath)
{
    if (m_resourcesView->model() == m_resourcesFileSystemModel.data()) {
        m_resourcesFileSystemModel->setRootPath(resourcePath);
        m_resourcesView->setRootIndex(m_resourcesFileSystemModel->indexForPath(resourcePath));
    }
    updateSearch();
}

void ItemLibraryWidget::startDragAndDrop(const QVariant &itemLibEntry, const QPointF &mousePos)
{
    // Actual drag is created after mouse has moved to avoid a QDrag bug that causes drag to stay
    // active (and blocks mouse release) if mouse is released at the same spot of the drag start.
    // This doesn't completely eliminate the bug but makes it significantly harder to produce.
    m_itemToDrag = itemLibEntry;
    m_dragStartPoint = mousePos.toPoint();
}

void ItemLibraryWidget::setFlowMode(bool b)
{
    m_itemLibraryModel->setFlowMode(b);
}

void ItemLibraryWidget::removeImport(const QString &importUrl)
{
    QTC_ASSERT(m_model, return);

    ItemLibraryImport *importSection = m_itemLibraryModel->importByUrl(importUrl);
    if (importSection)
        m_model->changeImports({}, {importSection->importEntry()});
}

void ItemLibraryWidget::addImportForItem(const QVariant &entry)
{
    QTC_ASSERT(m_itemLibraryModel, return);
    QTC_ASSERT(m_model, return);

    Import import = m_itemLibraryModel->entryToImport(entry.value<ItemLibraryEntry>());
    m_model->changeImports({import}, {});
}

void ItemLibraryWidget::addResources(const QStringList &files)
{
    auto document = QmlDesignerPlugin::instance()->currentDesignDocument();

    QTC_ASSERT(document, return);

    QList<AddResourceHandler> handlers = QmlDesignerPlugin::instance()->viewManager().designerActionManager().addResourceHandler();

    QMultiMap<QString, QString> map;
    for (const AddResourceHandler &handler : handlers) {
        map.insert(handler.category, handler.filter);
    }

    QMap<QString, QString> reverseMap;
    for (const AddResourceHandler &handler : handlers) {
        reverseMap.insert(handler.filter, handler.category);
    }

    QMap<QString, int> priorities;
    for (const AddResourceHandler &handler : handlers) {
        priorities.insert(handler.category, handler.piority);
    }

    QStringList sortedKeys = map.uniqueKeys();
    Utils::sort(sortedKeys, [&priorities](const QString &first,
                const QString &second){
        return priorities.value(first) < priorities.value(second);
    });

    QStringList fileNames = files;
    if (fileNames.isEmpty()) {
        QStringList filters;

        for (const QString &key : qAsConst(sortedKeys)) {
            QString str = key + " (";
            str.append(map.values(key).join(" "));
            str.append(")");
            filters.append(str);
        }

        filters.prepend(tr("All Files (%1)").arg(map.values().join(" ")));

        static QString lastDir;
        const QString currentDir = lastDir.isEmpty() ? document->fileName().parentDir().toString() : lastDir;

        fileNames = QFileDialog::getOpenFileNames(Core::ICore::dialogParent(),
                                                  tr("Add Assets"),
                                                  currentDir,
                                                  filters.join(";;"));

        if (!fileNames.isEmpty()) {
            lastDir = QFileInfo(fileNames.first()).absolutePath();
            // switch to assets view after an asset is added
            m_stackedWidget->setCurrentIndex(1);
            QMetaObject::invokeMethod(m_headerWidget->rootObject(), "setTab", Q_ARG(QVariant, 1));
        }
    }

    QMultiMap<QString, QString> partitionedFileNames;

    for (const QString &fileName : qAsConst(fileNames)) {
        const QString suffix = "*." + QFileInfo(fileName).suffix().toLower();
        const QString category = reverseMap.value(suffix);
        partitionedFileNames.insert(category, fileName);
    }

    for (const QString &category : partitionedFileNames.uniqueKeys()) {
         for (const AddResourceHandler &handler : handlers) {
             QStringList fileNames = partitionedFileNames.values(category);
             if (handler.category == category) {
                 QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_RESOURCE_IMPORTED + category);
                 if (!handler.operation(fileNames, document->fileName().parentDir().toString()))
                     Core::AsynchronousMessageBox::warning(tr("Failed to Add Files"), tr("Could not add %1 to project.").arg(fileNames.join(" ")));
                 break;
             }
         }
    }
}

void ItemLibraryWidget::importDroppedFiles(const QList<Utils::DropSupport::FileSpec> &files)
{
    QStringList fileNames;
    for (const auto &file : files) {
        QFileInfo fi(file.filePath);
        if (m_resourcesFileSystemModel->supportedSuffixes().contains(fi.suffix().toLower()))
            fileNames.append(fi.absoluteFilePath());
    }
    if (!fileNames.isEmpty())
        addResources(fileNames);
}
} // namespace QmlDesigner
