/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "itemlibrarywidget.h"

#include <utils/filterlineedit.h>
#include <utils/fileutils.h>
#include <coreplugin/coreconstants.h>
#include "itemlibrarycomponents.h"
#include "itemlibrarymodel.h"
#include "itemlibraryimageprovider.h"
#include "customdraganddrop.h"
#include <model.h>
#include <metainfo.h>
#include "rewritingexception.h"

#include <QFileInfo>
#include <QFileIconProvider>
#include <QFileSystemModel>
#include <QStackedWidget>
#include <QGridLayout>
#include <QTabBar>
#include <QImageReader>
#include <QMimeData>
#include <QWheelEvent>
#include <QMenu>
#include <QApplication>

#include <QDeclarativeView>
#include <QDeclarativeItem>
#include <private/qdeclarativeengine_p.h>


namespace QmlDesigner {

class MyFileIconProvider : public QFileIconProvider
{
public:
    MyFileIconProvider(const QSize &iconSize)
        : QFileIconProvider(),
          m_iconSize(iconSize)
    {}

    virtual QIcon icon ( const QFileInfo & info ) const
    {
        QPixmap pixmap(info.absoluteFilePath());
        if (pixmap.isNull()) {
            QIcon defaultIcon(QFileIconProvider::icon(info));
            pixmap = defaultIcon.pixmap(defaultIcon.actualSize(m_iconSize));
        }

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

private:
    QSize m_iconSize;
};


// ---------- ItemLibraryPrivate
class ItemLibraryWidgetPrivate {
public:
    ItemLibraryWidgetPrivate(QObject *object);

    Internal::ItemLibraryModel *m_itemLibraryModel;
    QFileSystemModel *m_resourcesFileSystemModel;

    QStackedWidget *m_stackedWidget;
    Utils::FilterLineEdit *m_lineEdit;
    QDeclarativeView *m_itemsView;
    Internal::ItemLibraryTreeView *m_resourcesView;
    QWeakPointer<ItemLibraryInfo> m_itemLibraryInfo;

    QSize m_itemIconSize, m_resIconSize;
    MyFileIconProvider m_iconProvider;
    Model *model;
};

ItemLibraryWidgetPrivate::ItemLibraryWidgetPrivate(QObject *object) :
    m_itemLibraryModel(0),
    m_resourcesFileSystemModel(0),
    m_stackedWidget(0),
    m_lineEdit(0),
    m_itemsView(0),
    m_resourcesView(0),
    m_itemIconSize(24, 24),
    m_resIconSize(24, 24),
    m_iconProvider(m_resIconSize),
    model(0)
{
    Q_UNUSED(object);
}

ItemLibraryWidget::ItemLibraryWidget(QWidget *parent) :
    QFrame(parent),
    d(new ItemLibraryWidgetPrivate(this)),
    m_filterFlag(QtBasic)
{
    setWindowTitle(tr("Library", "Title of library view"));

    /* create Items view and its model */
    d->m_itemsView = new QDeclarativeView(this);
    d->m_itemsView->setAttribute(Qt::WA_OpaquePaintEvent);
    d->m_itemsView->setAttribute(Qt::WA_NoSystemBackground);
    d->m_itemsView->setAcceptDrops(false);
    d->m_itemsView->setFocusPolicy(Qt::ClickFocus);
    d->m_itemsView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    d->m_itemLibraryModel = new Internal::ItemLibraryModel(QDeclarativeEnginePrivate::getScriptEngine(d->m_itemsView->engine()), this);
    d->m_itemLibraryModel->setItemIconSize(d->m_itemIconSize);

    QDeclarativeContext *rootContext = d->m_itemsView->rootContext();
    rootContext->setContextProperty(QLatin1String("itemLibraryModel"), d->m_itemLibraryModel);
    rootContext->setContextProperty(QLatin1String("itemLibraryIconWidth"), d->m_itemIconSize.width());
    rootContext->setContextProperty(QLatin1String("itemLibraryIconHeight"), d->m_itemIconSize.height());

    QColor highlightColor = palette().highlight().color();
    if (0.5*highlightColor.saturationF()+0.75-highlightColor.valueF() < 0)
        highlightColor.setHsvF(highlightColor.hsvHueF(),0.1 + highlightColor.saturationF()*2.0, highlightColor.valueF());
    d->m_itemsView->rootContext()->setContextProperty(QLatin1String("highlightColor"), highlightColor);

    // loading the qml has to come after all needed context properties are set
    d->m_itemsView->setSource(QUrl("qrc:/ItemLibrary/qml/ItemsView.qml"));

    QDeclarativeItem *rootItem = qobject_cast<QDeclarativeItem*>(d->m_itemsView->rootObject());
    connect(rootItem, SIGNAL(itemSelected(int)), this, SLOT(showItemInfo(int)));
    connect(rootItem, SIGNAL(itemDragged(int)), this, SLOT(startDragAndDrop(int)));
    connect(this, SIGNAL(scrollItemsView(QVariant)), rootItem, SLOT(scrollView(QVariant)));
    connect(this, SIGNAL(resetItemsView()), rootItem, SLOT(resetView()));

    /* create Resources view and its model */
    d->m_resourcesFileSystemModel = new QFileSystemModel(this);
    d->m_resourcesFileSystemModel->setIconProvider(&d->m_iconProvider);
    d->m_resourcesView = new Internal::ItemLibraryTreeView(this);
    d->m_resourcesView->setModel(d->m_resourcesFileSystemModel);
    d->m_resourcesView->setIconSize(d->m_resIconSize);

    /* create image provider for loading item icons */
    d->m_itemsView->engine()->addImageProvider(QLatin1String("qmldesigner_itemlibrary"), new Internal::ItemLibraryImageProvider);

    /* other widgets */
    QTabBar *tabBar = new QTabBar(this);
    tabBar->addTab(tr("Items", "Title of library items view"));
    tabBar->addTab(tr("Resources", "Title of library resources view"));
    tabBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    d->m_lineEdit = new Utils::FilterLineEdit(this);
    d->m_lineEdit->setObjectName(QLatin1String("itemLibrarySearchInput"));
    d->m_lineEdit->setPlaceholderText(tr("<Filter>", "Library search input hint text"));
    d->m_lineEdit->setDragEnabled(false);
    d->m_lineEdit->setMinimumWidth(75);
    d->m_lineEdit->setTextMargins(0, 0, 20, 0);
    QWidget *lineEditFrame = new QWidget(this);
    lineEditFrame->setObjectName(QLatin1String("itemLibrarySearchInputFrame"));
    QGridLayout *lineEditLayout = new QGridLayout(lineEditFrame);
    lineEditLayout->setMargin(2);
    lineEditLayout->setSpacing(0);
    lineEditLayout->addItem(new QSpacerItem(5, 3, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 0, 1, 3);
    lineEditLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 0);
    lineEditLayout->addWidget(d->m_lineEdit, 1, 1, 1, 1);
    lineEditLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 2);
    connect(d->m_lineEdit, SIGNAL(filterChanged(QString)), this, SLOT(setSearchFilter(QString)));

    d->m_stackedWidget = new QStackedWidget(this);
    d->m_stackedWidget->addWidget(d->m_itemsView);
    d->m_stackedWidget->addWidget(d->m_resourcesView);
    connect(tabBar, SIGNAL(currentChanged(int)),
            d->m_stackedWidget, SLOT(setCurrentIndex(int)));
    connect(tabBar, SIGNAL(currentChanged(int)),
            this, SLOT(updateSearch()));

    QWidget *spacer = new QWidget(this);
    spacer->setObjectName(QLatin1String("itemLibrarySearchInputSpacer"));
    spacer->setFixedHeight(4);

    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(tabBar, 0, 0, 1, 1);
    layout->addWidget(spacer, 1, 0);
    layout->addWidget(lineEditFrame, 2, 0, 1, 1);
    layout->addWidget(d->m_stackedWidget, 3, 0, 1, 1);

    setResourcePath(QDir::currentPath());
    setSearchFilter(QString());

    /* style sheets */
    setStyleSheet(QLatin1String(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css")));
    d->m_resourcesView->setStyleSheet(
            QLatin1String(Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css")));
}

ItemLibraryWidget::~ItemLibraryWidget()
{
    /* workaround: delete the items view before the model is deleted.
       This prevents qml warnings when the item library is destructed. */
    delete d->m_itemsView;
    delete d->m_resourcesView;
    delete d;
}

void ItemLibraryWidget::setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo)
{
    if (d->m_itemLibraryInfo.data() == itemLibraryInfo)
        return;

    if (d->m_itemLibraryInfo)
        disconnect(d->m_itemLibraryInfo.data(), SIGNAL(entriesChanged()),
                   this, SLOT(updateModel()));
    d->m_itemLibraryInfo = itemLibraryInfo;
    if (itemLibraryInfo)
        connect(d->m_itemLibraryInfo.data(), SIGNAL(entriesChanged()),
                this, SLOT(updateModel()));

    updateModel();
    updateSearch();
}

void ItemLibraryWidget::updateImports()
{
    FilterChangeFlag filter;
    filter = QtBasic;
    if (d->model) {
        QStringList imports;
        foreach (const Import &import, d->model->imports())
            if (import.isLibraryImport())
                imports << import.url();
        if (imports.contains("com.nokia.meego", Qt::CaseInsensitive))
            filter = Meego;
    }

    setImportFilter(filter);
}

QList<QToolButton *> ItemLibraryWidget::createToolBarWidgets()
{
    QList<QToolButton *> buttons;

    return buttons; //import management gets disabled for now (TODO ###)

    buttons << new QToolButton();
    buttons.first()->setText("I ");
    buttons.first()->setIcon(QIcon(QLatin1String(Core::Constants::ICON_FILTER)));
    buttons.first()->setToolTip("Manage imports for components");
    buttons.first()->setPopupMode(QToolButton::InstantPopup);
    QMenu * menu = new QMenu;
    QAction * basicQtAction = new QAction(menu);
    basicQtAction->setCheckable(true);
    basicQtAction->setText("Basic Qt Quick only");
    QAction * meegoAction= new QAction(menu);
    meegoAction->setCheckable(true);
    meegoAction->setText("Meego Components");
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
    if (d->m_stackedWidget->currentIndex() == 0) {
        d->m_itemLibraryModel->setSearchText(searchFilter);
        emit resetItemsView();
        d->m_itemsView->update();
    } else {
        QStringList nameFilterList;
        if (searchFilter.contains('.')) {
            nameFilterList.append(QString("*%1*").arg(searchFilter));
        } else {
            foreach (const QByteArray &extension, QImageReader::supportedImageFormats()) {
                nameFilterList.append(QString("*%1*.%2").arg(searchFilter, QString::fromAscii(extension)));
            }
        }

        d->m_resourcesFileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        d->m_resourcesFileSystemModel->setNameFilterDisables(false);
        d->m_resourcesFileSystemModel->setNameFilters(nameFilterList);
        d->m_resourcesView->expandToDepth(1);
        d->m_resourcesView->scrollToTop();
    }
}

void ItemLibraryWidget::setModel(Model *model)
{
    d->model = model;
    if (!model)
        return;
    setItemLibraryInfo(model->metaInfo().itemLibraryInfo());
    updateModel();
}

void ItemLibraryWidget::emitImportChecked()
{
    if (!d->model)
        return;

    bool qtOnlyImport = false;
    bool meegoImport = false;

    foreach (const Import &import, d->model->imports()) {
        if (import.isLibraryImport()) {
            if (import.url().contains(QString("meego"), Qt::CaseInsensitive))
                meegoImport = true;
            if (import.url().contains(QString("Qt"), Qt::CaseInsensitive) || import.url().contains(QString("QtQuick"), Qt::CaseInsensitive))
                qtOnlyImport = true;
        }
    }

    if (meegoImport)
        qtOnlyImport = false;

    emit qtBasicOnlyChecked(qtOnlyImport);
    emit meegoChecked(meegoImport);
}

void ItemLibraryWidget::setImportFilter(FilterChangeFlag flag)
{
    return;

    static bool block = false;
    if (!d->model)
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
            removeImport(QLatin1String("com.nokia.meego"));
        else if (flag == Meego)
            addImport(QLatin1String("com.nokia.meego"), QLatin1String("1.0"));
        QApplication::restoreOverrideCursor();
        block = false;
        m_filterFlag = flag;
    } catch (RewritingException &) {
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
    d->m_itemLibraryModel->update(d->m_itemLibraryInfo.data(), d->model);
    updateImports();
    updateSearch();
}

void ItemLibraryWidget::updateSearch()
{
    setSearchFilter(d->m_lineEdit->text());
}

void ItemLibraryWidget::setResourcePath(const QString &resourcePath)
{
    if (d->m_resourcesView->model() == d->m_resourcesFileSystemModel) {
        d->m_resourcesFileSystemModel->setRootPath(resourcePath);
        d->m_resourcesView->setRootIndex(d->m_resourcesFileSystemModel->index(resourcePath));
    }
    updateSearch();
}

void ItemLibraryWidget::startDragAndDrop(int itemLibId)
{
    QMimeData *mimeData = d->m_itemLibraryModel->getMimeData(itemLibId);
    CustomItemLibraryDrag *drag = new CustomItemLibraryDrag(this);
    const QImage image = qvariant_cast<QImage>(mimeData->imageData());

    drag->setPixmap(d->m_itemLibraryModel->getIcon(itemLibId).pixmap(32, 32));
    drag->setPreview(QPixmap::fromImage(image));
    drag->setMimeData(mimeData);

    QDeclarativeItem *rootItem = qobject_cast<QDeclarativeItem*>(d->m_itemsView->rootObject());
    connect(rootItem, SIGNAL(stopDragAndDrop()), drag, SLOT(stopDrag()));

    drag->exec();
}

void ItemLibraryWidget::showItemInfo(int /*itemLibId*/)
{
//    qDebug() << "showing item info about id" << itemLibId;
}

void ItemLibraryWidget::wheelEvent(QWheelEvent *event)
{
    if (d->m_stackedWidget->currentIndex() == 0 &&
        d->m_itemsView->rect().contains(event->pos())) {
        emit scrollItemsView(event->delta());
        event->accept();
    } else
        QFrame::wheelEvent(event);
}

 void ItemLibraryWidget::removeImport(const QString &name)
 {
     if (!d->model)
         return;

     QList<Import> toBeRemovedImportList;
     foreach (const Import &import, d->model->imports())
         if (import.isLibraryImport() && import.url().compare(name, Qt::CaseInsensitive) == 0)
             toBeRemovedImportList.append(import);

     d->model->changeImports(QList<Import>(), toBeRemovedImportList);
 }

 void ItemLibraryWidget::addImport(const QString &name, const QString &version)
 {
     if (!d->model)
         return;
     d->model->changeImports(QList<Import>() << Import::createLibraryImport(name, version), QList<Import>());
 }

}
