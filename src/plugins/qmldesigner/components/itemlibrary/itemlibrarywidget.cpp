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

#include <theme.h>

#include <itemlibrarymodel.h>
#include <itemlibraryimageprovider.h>
#include <itemlibraryinfo.h>
#include <metainfo.h>
#include <model.h>
#include <rewritingexception.h>
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
#include <QGridLayout>
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

ItemLibraryWidget::ItemLibraryWidget(QWidget *parent) :
    QFrame(parent),
    m_itemIconSize(24, 24),
    m_itemViewQuickWidget(new QQuickWidget),
    m_resourcesView(new ItemLibraryResourceView(this)),
    m_importTagsWidget(new QWidget(this)),
    m_addResourcesWidget(new QWidget(this)),
    m_filterFlag(QtBasic)
{
    m_compressionTimer.setInterval(200);
    m_compressionTimer.setSingleShot(true);
    ItemLibraryModel::registerQmlTypes();

    setWindowTitle(tr("Library", "Title of library view"));

    /* create Items view and its model */
    m_itemViewQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_itemViewQuickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_itemLibraryModel = new ItemLibraryModel(this);

    QQmlContext *rootContext = m_itemViewQuickWidget->rootContext();
    rootContext->setContextProperty(QStringLiteral("itemLibraryModel"), m_itemLibraryModel.data());
    rootContext->setContextProperty(QStringLiteral("itemLibraryIconWidth"), m_itemIconSize.width());
    rootContext->setContextProperty(QStringLiteral("itemLibraryIconHeight"), m_itemIconSize.height());
    rootContext->setContextProperty(QStringLiteral("rootView"), this);

    m_itemViewQuickWidget->rootContext()->setContextProperty(QStringLiteral("highlightColor"), Utils::StyleHelper::notTooBrightHighlightColor());

    /* create Resources view and its model */
    m_resourcesFileSystemModel = new CustomFileSystemModel(this);
    m_resourcesView->setModel(m_resourcesFileSystemModel.data());

    /* create image provider for loading item icons */
    m_itemViewQuickWidget->engine()->addImageProvider(QStringLiteral("qmldesigner_itemlibrary"), new Internal::ItemLibraryImageProvider);
    Theme::setupTheme(m_itemViewQuickWidget->engine());

    /* other widgets */
    auto tabBar = new QTabBar(this);
    tabBar->addTab(tr("QML Types", "Title of library QML types view"));
    tabBar->addTab(tr("Resources", "Title of library resources view"));
    tabBar->addTab(tr("Imports", "Title of library imports view"));
    tabBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(tabBar, &QTabBar::currentChanged, this, &ItemLibraryWidget::setCurrentIndexOfStackedWidget);
    connect(tabBar, &QTabBar::currentChanged, this, &ItemLibraryWidget::updateSearch);

    m_filterLineEdit = new Utils::FancyLineEdit(this);
    m_filterLineEdit->setObjectName(QStringLiteral("itemLibrarySearchInput"));
    m_filterLineEdit->setPlaceholderText(tr("<Filter>", "Library search input hint text"));
    m_filterLineEdit->setDragEnabled(false);
    m_filterLineEdit->setMinimumWidth(75);
    m_filterLineEdit->setTextMargins(0, 0, 20, 0);
    m_filterLineEdit->setFiltering(true);
    QWidget *lineEditFrame = new QWidget(this);
    lineEditFrame->setObjectName(QStringLiteral("itemLibrarySearchInputFrame"));
    auto lineEditLayout = new QGridLayout(lineEditFrame);
    lineEditLayout->setMargin(2);
    lineEditLayout->setSpacing(0);
    lineEditLayout->addItem(new QSpacerItem(5, 3, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 0, 1, 3);
    lineEditLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 0);
    lineEditLayout->addWidget(m_filterLineEdit.data(), 1, 1, 1, 1);
    lineEditLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 2);
    connect(m_filterLineEdit.data(), &Utils::FancyLineEdit::filterChanged, this, &ItemLibraryWidget::setSearchFilter);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(m_itemViewQuickWidget.data());
    m_stackedWidget->addWidget(m_resourcesView.data());

    QWidget *spacer = new QWidget(this);
    spacer->setObjectName(QStringLiteral("itemLibrarySearchInputSpacer"));
    spacer->setFixedHeight(4);

    auto layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(tabBar, 0, 0, 1, 1);
    layout->addWidget(spacer, 1, 0);
    layout->addWidget(lineEditFrame, 2, 0, 1, 1);
    layout->addWidget(m_importTagsWidget.data(), 3, 0, 1, 1);
    layout->addWidget(m_addResourcesWidget.data(), 4, 0, 1, 1);
    layout->addWidget(m_stackedWidget.data(), 5, 0, 1, 1);

    setSearchFilter(QString());

    /* style sheets */
    setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/stylesheet.css")))));
    m_resourcesView->setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css")))));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F5), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &ItemLibraryWidget::reloadQmlSource);

    connect(&m_compressionTimer, &QTimer::timeout, this, &ItemLibraryWidget::updateModel);

    auto flowLayout = new Utils::FlowLayout(m_importTagsWidget.data());
    flowLayout->setMargin(4);

    m_addResourcesWidget->setVisible(false);
    flowLayout = new Utils::FlowLayout(m_addResourcesWidget.data());
    flowLayout->setMargin(4);
    auto button = new QToolButton(m_addResourcesWidget.data());
    auto font = button->font();
    font.setPixelSize(Theme::instance()->smallFontPixelSize());
    button->setFont(font);
    button->setIcon(Utils::Icons::PLUS.icon());
    button->setText(tr("Add New Resources..."));
    button->setToolTip(tr("Add new resources to project."));
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    flowLayout->addWidget(button);
    connect(button, &QToolButton::clicked, this, &ItemLibraryWidget::addResources);

    // init the first load of the QML UI elements
    reloadQmlSource();
}

void ItemLibraryWidget::setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo)
{
    if (m_itemLibraryInfo.data() == itemLibraryInfo)
        return;

    if (m_itemLibraryInfo)
        disconnect(m_itemLibraryInfo.data(), &ItemLibraryInfo::entriesChanged,
                   this, &ItemLibraryWidget::delayedUpdateModel);
    m_itemLibraryInfo = itemLibraryInfo;
    if (itemLibraryInfo)
        connect(m_itemLibraryInfo.data(), &ItemLibraryInfo::entriesChanged,
                this, &ItemLibraryWidget::delayedUpdateModel);
    delayedUpdateModel();
}

void ItemLibraryWidget::updateImports()
{
    if (m_model)
        setupImportTagWidget();
}

void ItemLibraryWidget::setImportsWidget(QWidget *importsWidget)
{
    m_stackedWidget->addWidget(importsWidget);
}

QList<QToolButton *> ItemLibraryWidget::createToolBarWidgets()
{
    QList<QToolButton *> buttons;
    return buttons;
}

void ItemLibraryWidget::setSearchFilter(const QString &searchFilter)
{
    if (m_stackedWidget->currentIndex() == 0) {
        m_itemLibraryModel->setSearchText(searchFilter);
        m_itemViewQuickWidget->update();
    } else {
        QStringList nameFilterList;

        m_resourcesFileSystemModel->setSearchFilter(searchFilter);
        m_resourcesFileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        m_resourcesView->scrollToTop();
    }
}

void ItemLibraryWidget::delayedUpdateModel()
{
    m_compressionTimer.start();
}

void ItemLibraryWidget::setModel(Model *model)
{
    m_model = model;
    if (!model)
        return;
    setItemLibraryInfo(model->metaInfo().itemLibraryInfo());
}

void ItemLibraryWidget::setCurrentIndexOfStackedWidget(int index)
{
    if (index == 2) {
        m_filterLineEdit->setVisible(false);
        m_importTagsWidget->setVisible(true);
        m_addResourcesWidget->setVisible(false);
    } if (index == 1) {
        m_filterLineEdit->setVisible(true);
        m_importTagsWidget->setVisible(false);
        m_addResourcesWidget->setVisible(true);
    } else {
        m_filterLineEdit->setVisible(true);
        m_importTagsWidget->setVisible(true);
        m_addResourcesWidget->setVisible(false);
    }

    m_stackedWidget->setCurrentIndex(index);
}

QString ItemLibraryWidget::qmlSourcesPath()
{
    return Core::ICore::resourcePath() + QStringLiteral("/qmldesigner/itemLibraryQmlSources");
}

void ItemLibraryWidget::clearSearchFilter()
{
    m_filterLineEdit->clear();
}

void ItemLibraryWidget::reloadQmlSource()
{
    QString itemLibraryQmlFilePath = qmlSourcesPath() + QStringLiteral("/ItemsView.qml");
    QTC_ASSERT(QFileInfo::exists(itemLibraryQmlFilePath), return);
    m_itemViewQuickWidget->engine()->clearComponentCache();
    m_itemViewQuickWidget->setSource(QUrl::fromLocalFile(itemLibraryQmlFilePath));
}

void ItemLibraryWidget::setupImportTagWidget()
{
    QTC_ASSERT(m_model, return);

    const QStringList imports = m_model->metaInfo().itemLibraryInfo()->showTagsForImports();

    qDeleteAll(m_importTagsWidget->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));

    auto flowLayout = m_importTagsWidget->layout();

    auto createButton = [this](const QString &import) {
        auto button = new QToolButton(m_importTagsWidget.data());
        auto font = button->font();
        font.setPixelSize(Theme::instance()->smallFontPixelSize());
        button->setFont(font);
        button->setIcon(Utils::Icons::PLUS.icon());
        button->setText(import);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setToolTip(tr("Add import %1").arg(import));
        connect(button, &QToolButton::clicked, this, [this, import]() {
            addPossibleImport(import);
        });
        return button;
    };

    for (const QString &importPath : imports) {
        const Import import = Import::createLibraryImport(importPath);
        if (!m_model->hasImport(import, true, true)
                && m_model->isImportPossible(import, true, true))
            flowLayout->addWidget(createButton(importPath));
    }
}

void ItemLibraryWidget::updateModel()
{
    QTC_ASSERT(m_itemLibraryModel, return);

    m_itemLibraryModel->update(m_itemLibraryInfo.data(), m_model.data());
    updateImports();
    updateSearch();
}

void ItemLibraryWidget::updateSearch()
{
    setSearchFilter(m_filterLineEdit->text());
}

void ItemLibraryWidget::setResourcePath(const QString &resourcePath)
{
    if (m_resourcesView->model() == m_resourcesFileSystemModel.data()) {
        m_resourcesFileSystemModel->setRootPath(resourcePath);
        m_resourcesView->setRootIndex(m_resourcesFileSystemModel->indexForPath(resourcePath));
    }
    updateSearch();
}

void ItemLibraryWidget::startDragAndDrop(QQuickItem *mouseArea, QVariant itemLibraryId)
{
    m_currentitemLibraryEntry = itemLibraryId.value<ItemLibraryEntry>();

    QMimeData *mimeData = m_itemLibraryModel->getMimeData(m_currentitemLibraryEntry);
    auto drag = new QDrag(this);

    drag->setPixmap(Utils::StyleHelper::dpiSpecificImageFile(
                        m_currentitemLibraryEntry.libraryEntryIconPath()));
    drag->setMimeData(mimeData);

    /* Workaround for bug in Qt. The release event is not delivered for Qt < 5.9 if a drag is started */
    QMouseEvent event (QEvent::MouseButtonRelease, QPoint(-1, -1), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(mouseArea, &event);

    QTimer::singleShot(0, [drag]() {
        drag->exec();
        drag->deleteLater();
    });
}

void ItemLibraryWidget::removeImport(const QString &name)
{
    QTC_ASSERT(m_model, return);

    QList<Import> toBeRemovedImportList;
    foreach (const Import &import, m_model->imports())
        if (import.isLibraryImport() && import.url().compare(name, Qt::CaseInsensitive) == 0)
            toBeRemovedImportList.append(import);

    m_model->changeImports({}, toBeRemovedImportList);
}

void ItemLibraryWidget::addImport(const QString &name, const QString &version)
{
    QTC_ASSERT(m_model, return);
    m_model->changeImports({Import::createLibraryImport(name, version)}, {});
}

void ItemLibraryWidget::addPossibleImport(const QString &name)
{
    QTC_ASSERT(m_model, return);
    const Import import = m_model->highestPossibleImport(name);
    try {
        m_model->changeImports({Import::createLibraryImport(name, import.version())}, {});
    }
    catch (const RewritingException &e) {
        e.showException();
    }
    QmlDesignerPlugin::instance()->currentDesignDocument()->updateSubcomponentManager();
}

void ItemLibraryWidget::addResources()
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

    QStringList filters;

    for (const QString &key : sortedKeys) {
        QString str = key + " (";
        str.append(map.values(key).join(" "));
        str.append(")");
        filters.append(str);
    }

    filters.prepend(tr("All Files (%1)").arg(map.values().join(" ")));

    static QString lastDir;
    const QString currentDir = lastDir.isEmpty() ? document->fileName().parentDir().toString() : lastDir;

    const auto fileNames = QFileDialog::getOpenFileNames(this,
                                                   tr("Add Resources"),
                                                   currentDir,
                                                   filters.join(";;"));


    if (!fileNames.isEmpty())
        lastDir = QFileInfo(fileNames.first()).absolutePath();

    QMultiMap<QString, QString> partitionedFileNames;

    for (const QString &fileName : fileNames) {
        const QString suffix = "*." + QFileInfo(fileName).suffix();
        const QString category = reverseMap.value(suffix);
        partitionedFileNames.insert(category, fileName);
    }

    for (const QString &category : partitionedFileNames.uniqueKeys()) {
         for (const AddResourceHandler &handler : handlers) {
             QStringList fileNames = partitionedFileNames.values(category);
             if (handler.category == category) {
                 if (!handler.operation(fileNames, document->fileName().parentDir().toString()))
                     Core::AsynchronousMessageBox::warning(tr("Failed to Add Files"), tr("Could not add %1 to project.").arg(fileNames.join(" ")));
                 break;
             }
         }
    }
}

}
