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

#include <theming.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include "itemlibrarymodel.h"
#include "itemlibraryimageprovider.h"
#include <model.h>
#include <metainfo.h>
#include "rewritingexception.h"

#include <QFileInfo>
#include <QFileSystemModel>
#include <QStackedWidget>
#include <QGridLayout>
#include <QTabBar>
#include <QImageReader>
#include <QMimeData>
#include <QWheelEvent>
#include <QMenu>
#include <QApplication>
#include <QTimer>
#include <QShortcut>
#include <QQuickItem>

#include <private/qquickwidget_p.h> // mouse ungrabbing workaround on quickitems
#include <private/qquickwindow_p.h> // mouse ungrabbing workaround on quickitems


namespace QmlDesigner {

ItemLibraryWidget::ItemLibraryWidget(QWidget *parent) :
    QFrame(parent),
    m_itemIconSize(24, 24),
    m_resIconSize(64, 64),
    m_iconProvider(m_resIconSize),
    m_itemViewQuickWidget(new QQuickWidget),
    m_resourcesView(new ItemLibraryTreeView(this)),
    m_filterFlag(QtBasic)
{
    m_compressionTimer.setInterval(200);
    m_compressionTimer.setSingleShot(true);
    ItemLibraryModel::registerQmlTypes();

    setWindowTitle(tr("Library", "Title of library view"));

    /* create Items view and its model */
    m_itemViewQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_itemLibraryModel = new ItemLibraryModel(this);

    QQmlContext *rootContext = m_itemViewQuickWidget->rootContext();
    rootContext->setContextProperty(QStringLiteral("itemLibraryModel"), m_itemLibraryModel.data());
    rootContext->setContextProperty(QStringLiteral("itemLibraryIconWidth"), m_itemIconSize.width());
    rootContext->setContextProperty(QStringLiteral("itemLibraryIconHeight"), m_itemIconSize.height());
    rootContext->setContextProperty(QStringLiteral("rootView"), this);
    rootContext->setContextProperty(QLatin1String("creatorTheme"), Theming::theme());

    m_itemViewQuickWidget->rootContext()->setContextProperty(QStringLiteral("highlightColor"), Utils::StyleHelper::notTooBrightHighlightColor());

    /* create Resources view and its model */
    m_resourcesFileSystemModel = new QFileSystemModel(this);
    m_resourcesFileSystemModel->setIconProvider(&m_iconProvider);
    m_resourcesView->setModel(m_resourcesFileSystemModel.data());
    m_resourcesView->setIconSize(m_resIconSize);

    /* create image provider for loading item icons */
    m_itemViewQuickWidget->engine()->addImageProvider(QStringLiteral("qmldesigner_itemlibrary"), new Internal::ItemLibraryImageProvider);
    Theming::registerIconProvider(m_itemViewQuickWidget->engine());

    /* other widgets */
    QTabBar *tabBar = new QTabBar(this);
    tabBar->addTab(tr("QML Types", "Title of library QML types view"));
    tabBar->addTab(tr("Resources", "Title of library resources view"));
    tabBar->addTab(tr("Imports", "Title of library imports view"));
    tabBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(tabBar, SIGNAL(currentChanged(int)), this, SLOT(setCurrentIndexOfStackedWidget(int)));
    connect(tabBar, SIGNAL(currentChanged(int)), this, SLOT(updateSearch()));

    m_filterLineEdit = new Utils::FancyLineEdit(this);
    m_filterLineEdit->setObjectName(QStringLiteral("itemLibrarySearchInput"));
    m_filterLineEdit->setPlaceholderText(tr("<Filter>", "Library search input hint text"));
    m_filterLineEdit->setDragEnabled(false);
    m_filterLineEdit->setMinimumWidth(75);
    m_filterLineEdit->setTextMargins(0, 0, 20, 0);
    m_filterLineEdit->setFiltering(true);
    QWidget *lineEditFrame = new QWidget(this);
    lineEditFrame->setObjectName(QStringLiteral("itemLibrarySearchInputFrame"));
    QGridLayout *lineEditLayout = new QGridLayout(lineEditFrame);
    lineEditLayout->setMargin(2);
    lineEditLayout->setSpacing(0);
    lineEditLayout->addItem(new QSpacerItem(5, 3, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 0, 1, 3);
    lineEditLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 0);
    lineEditLayout->addWidget(m_filterLineEdit.data(), 1, 1, 1, 1);
    lineEditLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 2);
    connect(m_filterLineEdit.data(), SIGNAL(filterChanged(QString)), this, SLOT(setSearchFilter(QString)));


    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(m_itemViewQuickWidget.data());
    m_stackedWidget->addWidget(m_resourcesView.data());

    QWidget *spacer = new QWidget(this);
    spacer->setObjectName(QStringLiteral("itemLibrarySearchInputSpacer"));
    spacer->setFixedHeight(4);

    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(tabBar, 0, 0, 1, 1);
    layout->addWidget(spacer, 1, 0);
    layout->addWidget(lineEditFrame, 2, 0, 1, 1);
    layout->addWidget(m_stackedWidget.data(), 3, 0, 1, 1);

    setSearchFilter(QString());

    /* style sheets */
    setStyleSheet(Theming::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/stylesheet.css")))));
    m_resourcesView->setStyleSheet(Theming::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css")))));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F5), this);
    connect(m_qmlSourceUpdateShortcut, SIGNAL(activated()), this, SLOT(reloadQmlSource()));

    connect(&m_compressionTimer, SIGNAL(timeout()), this, SLOT(updateModel()));

    // init the first load of the QML UI elements
    reloadQmlSource();
}

void ItemLibraryWidget::setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo)
{
    if (m_itemLibraryInfo.data() == itemLibraryInfo)
        return;

    if (m_itemLibraryInfo)
        disconnect(m_itemLibraryInfo.data(), SIGNAL(entriesChanged()),
                   this, SLOT(delayedUpdateModel()));
    m_itemLibraryInfo = itemLibraryInfo;
    if (itemLibraryInfo)
        connect(m_itemLibraryInfo.data(), SIGNAL(entriesChanged()),
                this, SLOT(delayedUpdateModel()));
    delayedUpdateModel();
}

void ItemLibraryWidget::updateImports()
{
    if (m_model) {
        QStringList imports;
        foreach (const Import &import, m_model->imports())
            if (import.isLibraryImport())
                imports << import.url();
    }
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
        if (searchFilter.contains(QLatin1Char('.'))) {
            nameFilterList.append(QString(QStringLiteral("*%1*")).arg(searchFilter));
        } else {
            foreach (const QByteArray &extension, QImageReader::supportedImageFormats()) {
                nameFilterList.append(QString(QStringLiteral("*%1*.%2")).arg(searchFilter, QString::fromUtf8(extension)));
            }
        }

        m_resourcesFileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        m_resourcesFileSystemModel->setNameFilterDisables(false);
        m_resourcesFileSystemModel->setNameFilters(nameFilterList);
        m_resourcesView->expandToDepth(1);
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
    if (index == 2)
        m_filterLineEdit->setVisible(false);
    else
        m_filterLineEdit->setVisible(true);

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

void ItemLibraryWidget::updateModel()
{
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
        m_resourcesView->setRootIndex(m_resourcesFileSystemModel->index(resourcePath));
    }
    updateSearch();
}

static void ungrabMouseOnQMLWorldWorkAround(QQuickWidget *quickWidget)
{
    const QQuickWidgetPrivate *widgetPrivate = QQuickWidgetPrivate::get(quickWidget);
    if (widgetPrivate && widgetPrivate->offscreenWindow && widgetPrivate->offscreenWindow->mouseGrabberItem())
        widgetPrivate->offscreenWindow->mouseGrabberItem()->ungrabMouse();
}

void ItemLibraryWidget::startDragAndDrop(QVariant itemLibraryId)
{
    m_currentitemLibraryEntry = itemLibraryId.value<ItemLibraryEntry>();

    QMimeData *mimeData = m_itemLibraryModel->getMimeData(m_currentitemLibraryEntry);
    QDrag *drag = new QDrag(this);

    drag->setPixmap(Utils::StyleHelper::dpiSpecificImageFile(
                        m_currentitemLibraryEntry.libraryEntryIconPath()));
    drag->setMimeData(mimeData);

    drag->exec();

    ungrabMouseOnQMLWorldWorkAround(m_itemViewQuickWidget.data());
}

void ItemLibraryWidget::removeImport(const QString &name)
{
    if (!m_model)
        return;

    QList<Import> toBeRemovedImportList;
    foreach (const Import &import, m_model->imports())
        if (import.isLibraryImport() && import.url().compare(name, Qt::CaseInsensitive) == 0)
            toBeRemovedImportList.append(import);

    m_model->changeImports(QList<Import>(), toBeRemovedImportList);
}

void ItemLibraryWidget::addImport(const QString &name, const QString &version)
{
    if (!m_model)
        return;
    m_model->changeImports(QList<Import>() << Import::createLibraryImport(name, version), QList<Import>());
}

QIcon ItemLibraryFileIconProvider::icon(const QFileInfo &info) const
{
    QSize iconSize = m_iconSize;

    QPixmap pixmap(info.absoluteFilePath());

    if (pixmap.isNull()) {
        QIcon defaultIcon(QFileIconProvider::icon(info));
        pixmap = defaultIcon.pixmap(defaultIcon.actualSize(QSize(16, 16)));
    }

    if (pixmap.isNull())
        return pixmap;

    if (pixmap.width() == iconSize.width()
            && pixmap.height() == iconSize.height())
        return pixmap;

    if ((pixmap.width() > iconSize.width())
            || (pixmap.height() > iconSize.height())) {

        pixmap = pixmap.scaled(iconSize, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }

    QImage newIcon(iconSize, QImage::Format_ARGB32_Premultiplied);
    newIcon.fill(Qt::transparent);
    QPainter painter(&newIcon);

    painter.drawPixmap(qAbs(m_iconSize.width() - pixmap.width()) / 2, qAbs(m_iconSize.height() - pixmap.height()) / 2, pixmap);

    QIcon icon(QPixmap::fromImage(newIcon));

    return icon;
}

ItemLibraryFileIconProvider::ItemLibraryFileIconProvider(const QSize &iconSize)
    : QFileIconProvider(),
      m_iconSize(iconSize)
{}
}
