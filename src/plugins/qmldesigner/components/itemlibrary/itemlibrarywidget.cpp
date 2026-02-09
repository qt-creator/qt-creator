// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibrarywidget.h"

#include "itemlibraryconstants.h"
#include "itemlibraryiconimageprovider.h"
#include "itemlibraryimport.h"
#include "itemlibrarytracing.h"

#include <theme.h>

#include <designeractionmanager.h>
#include <designermcumanager.h>
#include <documentmanager.h>
#include <itemlibraryaddimportmodel.h>
#include <itemlibraryentry.h>
#include <itemlibraryimageprovider.h>
#include <modelnodeoperations.h>
#ifndef QDS_USE_PROJECTSTORAGE
#  include <itemlibraryinfo.h>
#endif
#include <itemlibrarymodel.h>
#include <model.h>
#include <modelutils.h>
#include <rewritingexception.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#ifndef QDS_USE_PROJECTSTORAGE
#  include <metainfo.h>
#endif

#include <qmldesigner/settings/designersettings.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
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

using ItemLibraryTracing::category;

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (::Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toUrlishString();
}

bool ItemLibraryWidget::eventFilter(QObject *obj, QEvent *event)
{
    NanotraceHR::Tracer tracer{"item library widget event filter", category()};

    if (event->type() == QEvent::FocusOut) {
        tracer.tick("focus out");
        if (obj == m_itemsWidget->quickWidget()) {
            tracer.tick("close context menue");
            QMetaObject::invokeMethod(m_itemsWidget->rootObject(), "closeContextMenu");
        }
    } else if (event->type() == QMouseEvent::MouseMove) {
        tracer.tick("mouse move");

        if (m_itemToDrag.isValid()) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if ((me->globalPosition().toPoint() - m_dragStartPoint).manhattanLength() > 10) {
                ItemLibraryEntry entry = m_itemToDrag.value<ItemLibraryEntry>();
                m_itemToDrag = {};

                // For drag to be handled correctly, we must have the component properly imported
                // beforehand, so we import the module immediately when the drag starts
#ifdef QDS_USE_PROJECTSTORAGE
                if (!entry.requiredImport().isEmpty()) {
                    Import import = Import::createLibraryImport(entry.requiredImport());
                    m_model->changeImports({import}, {});
                }
#else
                if (!entry.requiredImport().isEmpty()
                    && !ModelUtils::addImportWithCheck(entry.requiredImport(), m_model)) {
                    qWarning() << __FUNCTION__ << "Required import adding failed:"
                               << entry.requiredImport();
                }
#endif

                if (m_model) {
                    tracer.tick("start drag");

                    m_model->startDrag(m_itemLibraryModel->getMimeData(entry),
                                       ::Utils::StyleHelper::dpiSpecificImageFile(
                                           entry.libraryEntryIconPath()),
                                       this);
                }
            }
        }
    } else if (event->type() == QMouseEvent::MouseButtonRelease) {
        tracer.tick("mouse release");
        m_itemToDrag = {};

        setIsDragging(false);
    }

    return QObject::eventFilter(obj, event);
}

void ItemLibraryWidget::resizeEvent(QResizeEvent *event)
{
    NanotraceHR::Tracer tracer{"item library widget resize event", category()};

    isHorizontalLayout = event->size().width() >= HORIZONTAL_LAYOUT_WIDTH_LIMIT;
}

ItemLibraryWidget::ItemLibraryWidget(AsynchronousImageCache &imageCache)
    : m_itemIconSize(24, 24)
    , m_itemLibraryModel(std::make_unique<ItemLibraryModel>())
    , m_addModuleModel(std::make_unique<ItemLibraryAddImportModel>())
    , m_itemsWidget(Utils::makeUniqueObjectPtr<StudioQuickWidget>())
    , m_imageCache{imageCache}
{
    NanotraceHR::Tracer tracer{"item library widget constructor", category()};

    m_compressionTimer.setInterval(1000);
    m_compressionTimer.setSingleShot(true);
    ItemLibraryModel::registerQmlTypes();

    setWindowTitle(tr("Components Library", "Title of library view"));
    setMinimumWidth(100);

    // set up Component Library view and model
    m_itemsWidget->quickWidget()->setObjectName(Constants::OBJECT_NAME_COMPONENT_LIBRARY);
    m_itemsWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_itemsWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");

    m_previewTooltipBackend = std::make_unique<PreviewTooltipBackend>(m_imageCache);

    m_itemsWidget->setClearColor(Theme::getColor(Theme::Color::DSpanelBackground));
    m_itemsWidget->engine()->addImageProvider(QStringLiteral("qmldesigner_itemlibrary"),
                                              new Internal::ItemLibraryImageProvider);
    Theme::setupTheme(m_itemsWidget->engine());
    m_itemsWidget->quickWidget()->installEventFilter(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_itemsWidget.get());

    updateSearch();

    setStyleSheet(Theme::replaceCssColors(
        ::Utils::FileUtils::fetchQrc(":/qmldesigner/stylesheet.css")));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F5), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &ItemLibraryWidget::reloadQmlSource);

    connect(&m_compressionTimer, &QTimer::timeout, this, &ItemLibraryWidget::updateModel);

    m_itemsWidget->engine()->addImageProvider("itemlibrary_preview",
                                              new ItemLibraryIconImageProvider{m_imageCache});

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_ITEMLIBRARY_TIME);

    // init the first load of the QML UI elements

    auto map = m_itemsWidget->registerPropertyMap("ItemLibraryBackend");

    map->setProperties({{"itemLibraryModel", QVariant::fromValue(m_itemLibraryModel.get())},
                        {"addModuleModel", QVariant::fromValue(m_addModuleModel.get())},
                        {"itemLibraryIconWidth", m_itemIconSize.width()},
                        {"itemLibraryIconHeight", m_itemIconSize.height()},
                        {"rootView", QVariant::fromValue(this)},
                        {"widthLimit", HORIZONTAL_LAYOUT_WIDTH_LIMIT},
                        {"highlightColor", ::Utils::StyleHelper::notTooBrightHighlightColor()},
                        {"tooltipBackend", QVariant::fromValue(m_previewTooltipBackend.get())}});

    reloadQmlSource();
}

ItemLibraryWidget::~ItemLibraryWidget()
{
    NanotraceHR::Tracer tracer{"item library widget destructor", category()};
}

#ifndef QDS_USE_PROJECTSTORAGE
void ItemLibraryWidget::setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo)
{
    if (m_itemLibraryInfo.data() == itemLibraryInfo)
        return;

    if (m_itemLibraryInfo) {
        disconnect(m_itemLibraryInfo.data(), &ItemLibraryInfo::entriesChanged,
                   this, &ItemLibraryWidget::delayedUpdateModel);
    }
    m_itemLibraryInfo = itemLibraryInfo;
    if (itemLibraryInfo) {
        connect(m_itemLibraryInfo.data(), &ItemLibraryInfo::entriesChanged,
                this, &ItemLibraryWidget::delayedUpdateModel);
    }
    delayedUpdateModel();
}
#endif

QList<QToolButton *> ItemLibraryWidget::createToolBarWidgets()
{
    NanotraceHR::Tracer tracer{"item library widget create toolbar widgets", category()};

    return {};
}


void ItemLibraryWidget::handleSearchFilterChanged(const QString &filterText)
{
    NanotraceHR::Tracer tracer{"item library widget handle search filter changed", category()};

    if (filterText != m_filterText) {
        m_filterText = filterText;
        updateSearch();
    }
}

QString ItemLibraryWidget::getDependencyImport(const Import &import)
{
    NanotraceHR::Tracer tracer{"item library widget get dependency import", category()};

    static QStringList prefixDependencies = {"QtQuick3D"};

    const QStringList splitImport = import.url().split('.');

    if (splitImport.size() > 1) {
        if (prefixDependencies.contains(splitImport.first()))
            return splitImport.first();
    }

    return {};
}

void ItemLibraryWidget::handleAddImport(int index)
{
    NanotraceHR::Tracer tracer{"item library widget handle add import", category()};

    Import import = m_addModuleModel->getImportAt(index);
    if (import.isLibraryImport() && (import.url().startsWith("QtQuick")
                                     || import.url().startsWith("SimulinkConnector"))) {
        QString importStr = import.toImportString();
        importStr.replace(' ', '-');
        QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_IMPORT_ADDED
                                               + importStr);
    }

    Imports imports;
    const QString dependency = getDependencyImport(import);

    auto document = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = document->documentModel();

    if (!dependency.isEmpty()) {
        Import dependencyImport = m_addModuleModel->getImport(dependency);
        if (!dependencyImport.isEmpty())
            imports.append(dependencyImport);
    }
    imports.append(import);
    try {
        model->changeImports(imports, {});
    } catch (const Exception &e) {
        e.showException();
    }

    switchToComponentsView();
    updateSearch();
}

void ItemLibraryWidget::goIntoComponent(const QString &source)
{
    NanotraceHR::Tracer tracer{"item library widget go into component", category()};

    DocumentManager::goIntoComponent(source);
}

void ItemLibraryWidget::delayedUpdateModel()
{
    NanotraceHR::Tracer tracer{"item library widget delayed update model", category()};

    static bool disableTimer = designerSettings().disableItemLibraryUpdateTimer();

    if (disableTimer)
        updateModel();
    else
        m_compressionTimer.start();
}

void ItemLibraryWidget::setModel(Model *model)
{
    NanotraceHR::Tracer tracer{"item library widget set model", category()};

    m_model = model;
    if (!model) {
        m_itemToDrag = {};
        return;
    }
#ifndef QDS_USE_PROJECTSTORAGE
    setItemLibraryInfo(model->metaInfo().itemLibraryInfo());
#endif

    if (DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument()) {
        const bool subCompEditMode = document->inFileComponentModelActive();
        if (m_subCompEditMode != subCompEditMode) {
            m_subCompEditMode = subCompEditMode;
            // Switch out of add module view if it's active
            if (m_subCompEditMode)
                switchToComponentsView();
            emit subCompEditModeChanged();
        }
    }
}

QString ItemLibraryWidget::qmlSourcesPath()
{
    NanotraceHR::Tracer tracer{"item library widget qml sources path", category()};

#ifdef SHARE_QML_PATH
    if (::Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/itemLibraryQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/itemLibraryQmlSources").toUrlishString();
}

void ItemLibraryWidget::clearSearchFilter()
{
    NanotraceHR::Tracer tracer{"item library widget clear search filter", category()};

    QMetaObject::invokeMethod(m_itemsWidget->rootObject(), "clearSearchFilter");
}

void ItemLibraryWidget::switchToComponentsView()
{
    NanotraceHR::Tracer tracer{"item library widget switch to components view", category()};

    QMetaObject::invokeMethod(m_itemsWidget->rootObject(), "switchToComponentsView");
}

void ItemLibraryWidget::reloadQmlSource()
{
    NanotraceHR::Tracer tracer{"item library widget reload qml source", category()};

    const QString itemLibraryQmlPath = qmlSourcesPath() + "/ItemsView.qml";
    QTC_ASSERT(QFileInfo::exists(itemLibraryQmlPath), return);
    m_itemsWidget->setSource(QUrl::fromLocalFile(itemLibraryQmlPath));
}

void ItemLibraryWidget::updateModel()
{
    NanotraceHR::Tracer tracer{"item library widget update model", category()};

    QTC_ASSERT(m_itemLibraryModel, return);

    if (m_compressionTimer.isActive()) {
        m_updateRetry = false;
        m_compressionTimer.stop();
    }

    m_itemLibraryModel->update(m_model.data());

    if (m_itemLibraryModel->rowCount() == 0 && !m_updateRetry) {
        m_updateRetry = true; // Only retry once to avoid endless loops
        m_compressionTimer.start();
    } else {
        m_updateRetry = false;
    }
    updateSearch();
}

void ItemLibraryWidget::updatePossibleImports(const Imports &possibleImports)
{
    NanotraceHR::Tracer tracer{"item library widget update possible imports", category()};

    m_addModuleModel->update(set_difference(possibleImports, m_model->imports()));
    delayedUpdateModel();
}

void ItemLibraryWidget::updateUsedImports(const Imports &usedImports)
{
    NanotraceHR::Tracer tracer{"item library widget update used imports", category()};

    m_itemLibraryModel->updateUsedImports(usedImports);
}

void ItemLibraryWidget::updateSearch()
{
    NanotraceHR::Tracer tracer{"item library widget update search", category()};

    m_itemLibraryModel->setSearchText(m_filterText);
    m_itemsWidget->update();
    m_addModuleModel->setSearchText(m_filterText);
}

void ItemLibraryWidget::setIsDragging(bool val)
{
    NanotraceHR::Tracer tracer{"item library widget set is dragging", category()};

    if (m_isDragging != val) {
        m_isDragging = val;
        emit isDraggingChanged();
    }
}

void ItemLibraryWidget::startDragAndDrop(const QVariant &itemLibEntry, const QPointF &mousePos)
{
    NanotraceHR::Tracer tracer{"item library widget start drag and drop", category()};

    // Actual drag is created after mouse has moved to avoid a QDrag bug that causes drag to stay
    // active (and blocks mouse release) if mouse is released at the same spot of the drag start.
    m_itemToDrag = itemLibEntry;
    m_dragStartPoint = mousePos.toPoint();
    setIsDragging(true);
}

bool ItemLibraryWidget::subCompEditMode() const
{
    NanotraceHR::Tracer tracer{"item library widget sub component edit mode", category()};

    return m_subCompEditMode;
}

void ItemLibraryWidget::removeImport(const QString &importUrl)
{
    NanotraceHR::Tracer tracer{"item library widget remove import", category()};

    QTC_ASSERT(m_model, return);

    ItemLibraryImport *importSection = m_itemLibraryModel->importByUrl(importUrl);
    if (importSection) {
        importSection->showAllCategories();
        m_model->changeImports({}, {importSection->importEntry()});
    }
}

void ItemLibraryWidget::addImportForItem(const QString &importUrl)
{
    NanotraceHR::Tracer tracer{"item library widget add import for item", category()};

    QTC_ASSERT(m_itemLibraryModel, return);
    QTC_ASSERT(m_model, return);

    Import import = m_addModuleModel->getImport(importUrl);
    m_model->changeImports({import}, {});
}

} // namespace QmlDesigner
