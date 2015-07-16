/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "itemlibrarywidget.h"

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
    m_resIconSize(24, 24),
    m_iconProvider(m_resIconSize),
    m_itemViewQuickWidget(new QQuickWidget),
    m_resourcesView(new ItemLibraryTreeView(this)),
    m_filterFlag(QtBasic)
{
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

    m_itemViewQuickWidget->rootContext()->setContextProperty(QStringLiteral("highlightColor"), Utils::StyleHelper::notTooBrightHighlightColor());

    /* create Resources view and its model */
    m_resourcesFileSystemModel = new QFileSystemModel(this);
    m_resourcesFileSystemModel->setIconProvider(&m_iconProvider);
    m_resourcesView->setModel(m_resourcesFileSystemModel.data());
    m_resourcesView->setIconSize(m_resIconSize);

    /* create image provider for loading item icons */
    m_itemViewQuickWidget->engine()->addImageProvider(QStringLiteral("qmldesigner_itemlibrary"), new Internal::ItemLibraryImageProvider);

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

    setResourcePath(QDir::currentPath());
    setSearchFilter(QString());

    /* style sheets */
    setStyleSheet(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/stylesheet.css"))));
    m_resourcesView->setStyleSheet(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css"))));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F5), this);
    connect(m_qmlSourceUpdateShortcut, SIGNAL(activated()), this, SLOT(reloadQmlSource()));

    // init the first load of the QML UI elements
    reloadQmlSource();
}

void ItemLibraryWidget::setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo)
{
    if (m_itemLibraryInfo.data() == itemLibraryInfo)
        return;

    if (m_itemLibraryInfo)
        disconnect(m_itemLibraryInfo.data(), SIGNAL(entriesChanged()),
                   this, SLOT(updateModel()));
    m_itemLibraryInfo = itemLibraryInfo;
    if (itemLibraryInfo)
        connect(m_itemLibraryInfo.data(), SIGNAL(entriesChanged()),
                this, SLOT(updateModel()));

    updateModel();
    updateSearch();
}

void ItemLibraryWidget::updateImports()
{
    FilterChangeFlag filter;
    filter = QtBasic;
    if (m_model) {
        QStringList imports;
        foreach (const Import &import, m_model->imports())
            if (import.isLibraryImport())
                imports << import.url();
        if (imports.contains(QLatin1String("com.nokia.meego"), Qt::CaseInsensitive))
            filter = Meego;
    }

    setImportFilter(filter);
}

void ItemLibraryWidget::setImportsWidget(QWidget *importsWidget)
{
    m_stackedWidget->addWidget(importsWidget);
}

QList<QToolButton *> ItemLibraryWidget::createToolBarWidgets()
{
    QList<QToolButton *> buttons;

    return buttons; //import management gets disabled for now (TODO ###)

    buttons << new QToolButton();
    buttons.first()->setText(tr("I "));
    buttons.first()->setIcon(QIcon(QLatin1String(Core::Constants::ICON_FILTER)));
    buttons.first()->setToolTip(tr("Manage imports for components."));
    buttons.first()->setPopupMode(QToolButton::InstantPopup);
    QMenu * menu = new QMenu;
    QAction * basicQtAction = new QAction(menu);
    basicQtAction->setCheckable(true);
    basicQtAction->setText(tr("Basic Qt Quick only"));
    QAction * meegoAction= new QAction(menu);
    meegoAction->setCheckable(true);
    meegoAction->setText(tr("Meego Components"));
    menu->addAction(basicQtAction);
    menu->addAction(meegoAction);
    buttons.first()->setMenu(menu);

    connect(basicQtAction, SIGNAL(toggled(bool)), this, SLOT(onQtBasicOnlyChecked(bool)));
    connect(this, SIGNAL(qtBasicOnlyChecked(bool)), basicQtAction, SLOT(setChecked(bool)));

    connect(meegoAction, SIGNAL(toggled(bool)), this, SLOT(onMeegoChecked(bool)));
    connect(this, SIGNAL(meegoChecked(bool)), meegoAction, SLOT(setChecked(bool)));

    updateImports();

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

void ItemLibraryWidget::setModel(Model *model)
{
    m_model = model;
    if (!model)
        return;
    setItemLibraryInfo(model->metaInfo().itemLibraryInfo());
    updateModel();
}

void ItemLibraryWidget::emitImportChecked()
{
    if (!m_model)
        return;

    bool qtOnlyImport = false;
    bool meegoImport = false;

    foreach (const Import &import, m_model->imports()) {
        if (import.isLibraryImport()) {
            if (import.url().contains(QLatin1String("meego"), Qt::CaseInsensitive))
                meegoImport = true;
            if (import.url().contains(QLatin1String("Qt"), Qt::CaseInsensitive) || import.url().contains(QLatin1String("QtQuick"), Qt::CaseInsensitive))
                qtOnlyImport = true;
        }
    }

    if (meegoImport)
        qtOnlyImport = false;

    emit qtBasicOnlyChecked(qtOnlyImport);
    emit meegoChecked(meegoImport);
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

void ItemLibraryWidget::reloadQmlSource()
{
    QString itemLibraryQmlFilePath = qmlSourcesPath() + QStringLiteral("/ItemsView.qml");
    QTC_ASSERT(QFileInfo::exists(itemLibraryQmlFilePath), return);
    m_itemViewQuickWidget->engine()->clearComponentCache();
    m_itemViewQuickWidget->setSource(QUrl::fromLocalFile(itemLibraryQmlFilePath));
}

void ItemLibraryWidget::setImportFilter(FilterChangeFlag flag)
{
    return;

    static bool block = false;
    if (!m_model)
        return;
    if (flag == m_filterFlag)
        return;

    if (block == true)
        return;


    FilterChangeFlag oldfilterFlag = m_filterFlag;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    try {
        block = true;
        if (flag == QtBasic)
            removeImport(QStringLiteral("com.nokia.meego"));
        else if (flag == Meego)
            addImport(QStringLiteral("com.nokia.meego"), QStringLiteral("1.0"));
        QApplication::restoreOverrideCursor();
        block = false;
        m_filterFlag = flag;
    } catch (const RewritingException &) {
        QApplication::restoreOverrideCursor();
        m_filterFlag = oldfilterFlag;
        block = false;
        // do something about it
    }

    emitImportChecked();
}

void ItemLibraryWidget::onQtBasicOnlyChecked(bool b)
{
    if (b)
        setImportFilter(QtBasic);

}

void ItemLibraryWidget::onMeegoChecked(bool b)
{
    if (b)
        setImportFilter(Meego);
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

    drag->setPixmap(m_currentitemLibraryEntry.libraryEntryIconPath());
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
    QPixmap pixmap(info.absoluteFilePath());
    if (pixmap.isNull()) {
        QIcon defaultIcon(QFileIconProvider::icon(info));
        pixmap = defaultIcon.pixmap(defaultIcon.actualSize(m_iconSize));
    }

    if (pixmap.isNull())
        return pixmap;

    if (pixmap.width() == m_iconSize.width()
            && pixmap.height() == m_iconSize.height())
        return pixmap;

    if ((pixmap.width() > m_iconSize.width())
            || (pixmap.height() > m_iconSize.height()))
        return pixmap.scaled(m_iconSize, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);

    QPoint offset((m_iconSize.width() - pixmap.width()) / 2,
                  (m_iconSize.height() - pixmap.height()) / 2);
    QImage newIcon(m_iconSize, QImage::Format_ARGB32_Premultiplied);
    newIcon.fill(Qt::transparent);
    QPainter painter(&newIcon);
    painter.drawPixmap(offset, pixmap);
    return QPixmap::fromImage(newIcon);
}

ItemLibraryFileIconProvider::ItemLibraryFileIconProvider(const QSize &iconSize)
    : QFileIconProvider(),
      m_iconSize(iconSize)
{}
}
