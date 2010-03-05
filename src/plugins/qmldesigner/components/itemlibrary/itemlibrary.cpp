/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "itemlibrary.h"

#include "itemlibrarymodel.h"
#include "customdraganddrop.h"

#include "ui_itemlibrary.h"
#include <metainfo.h>
#include <itemlibraryinfo.h>
#include <formeditorscene.h>

#include <QtCore/QDebug>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsItem>
#include <QtGui/QPixmap>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QFile>
#include <QDirModel>
#include <QFileIconProvider>
#include <QImageReader>

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
            return pixmap.scaled(m_iconSize, Qt::KeepAspectRatio);

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
class ItemLibraryPrivate {
public:
    ItemLibraryPrivate(QObject *object);

    Ui::ItemLibrary m_ui;
    Internal::ItemLibraryModel *m_itemLibraryModel;
    QDeclarativeView *m_itemsView;
    QDirModel *m_resourcesDirModel;
    QString m_resourcePath;
    QSize m_itemIconSize, m_resIconSize;
    MyFileIconProvider m_iconProvider;
};

ItemLibraryPrivate::ItemLibraryPrivate(QObject *object) :
    m_itemLibraryModel(0),
    m_itemsView(0),
    m_itemIconSize(22, 22),
    m_resIconSize(22, 22),
    m_iconProvider(m_resIconSize)
{
    m_resourcePath = QDir::currentPath();
    Q_UNUSED(object);
}

ItemLibrary::ItemLibrary(QWidget *parent) :
    QFrame(parent),
    m_d(new ItemLibraryPrivate(this))
{
    m_d->m_ui.setupUi(this);
    layout()->setContentsMargins(2, 2, 2, 0);
    layout()->setSpacing(2);

    m_d->m_resourcesDirModel = new QDirModel(this);

    m_d->m_ui.ItemLibraryTreeView->setModel(m_d->m_resourcesDirModel);
    m_d->m_ui.ItemLibraryTreeView->setIconSize(m_d->m_resIconSize);
    m_d->m_ui.ItemLibraryTreeView->setColumnHidden(1, true);
    m_d->m_ui.ItemLibraryTreeView->setColumnHidden(2, true);
    m_d->m_ui.ItemLibraryTreeView->setColumnHidden(3, true);
    m_d->m_ui.ItemLibraryTreeView->setSortingEnabled(true);
    m_d->m_ui.ItemLibraryTreeView->setHeaderHidden(true);
    m_d->m_ui.ItemLibraryTreeView->setIndentation(10);
    m_d->m_ui.ItemLibraryTreeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_d->m_ui.ItemLibraryTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_d->m_ui.ItemLibraryTreeView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_d->m_ui.ItemLibraryTreeView->setRootIndex(m_d->m_resourcesDirModel->index(m_d->m_resourcePath));

    m_d->m_itemsView = new QDeclarativeView(this);
    m_d->m_itemsView->setAttribute(Qt::WA_OpaquePaintEvent);
    m_d->m_itemsView->setAttribute(Qt::WA_NoSystemBackground);
    m_d->m_itemsView->setAcceptDrops(false);
    m_d->m_itemsView->setFocusPolicy(Qt::ClickFocus);
    m_d->m_itemsView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    m_d->m_ui.ItemLibraryGridLayout->addWidget(m_d->m_itemsView, 0, 0);

    m_d->m_itemLibraryModel = new Internal::ItemLibraryModel(QDeclarativeEnginePrivate::getScriptEngine(m_d->m_itemsView->engine()), this);
    m_d->m_itemLibraryModel->setItemIconSize(m_d->m_itemIconSize);
    m_d->m_itemsView->rootContext()->setContextProperty(QLatin1String("itemLibraryModel"), m_d->m_itemLibraryModel);
    m_d->m_itemsView->rootContext()->setContextProperty(QLatin1String("itemLibraryIconWidth"), m_d->m_itemIconSize.width());
    m_d->m_itemsView->rootContext()->setContextProperty(QLatin1String("itemLibraryIconHeight"), m_d->m_itemIconSize.height());

    m_d->m_itemsView->setSource(QUrl("qrc:/ItemLibrary/qml/ItemsView.qml"));

    QDeclarativeItem *rootItem = qobject_cast<QDeclarativeItem*>(m_d->m_itemsView->rootObject());
    connect(rootItem, SIGNAL(itemSelected(int)), this, SLOT(showItemInfo(int)));
    connect(rootItem, SIGNAL(itemDragged(int)), this, SLOT(startDragAndDrop(int)));
    connect(this, SIGNAL(expandAllItems()), rootItem, SLOT(expandAll()));

    connect(m_d->m_ui.lineEdit, SIGNAL(textChanged(QString)), this, SLOT(setSearchFilter(QString)));
    m_d->m_ui.lineEdit->setDragEnabled(false);

    connect(m_d->m_ui.buttonItems, SIGNAL(clicked()), this, SLOT(itemLibraryButtonToggled()));
    connect(m_d->m_ui.buttonResources, SIGNAL(clicked()), this, SLOT(resourcesButtonToggled()));

    m_d->m_ui.buttonItems->setChecked(true);
    itemLibraryButtonToggled();
    setSearchFilter("");

    m_d->m_resourcesDirModel->setIconProvider(&m_d->m_iconProvider);

    setWindowTitle(tr("Library", "Title of library view"));

    {
        QFile file(":/qmldesigner/stylesheet.css");
        file.open(QFile::ReadOnly);
        QString styleSheet = QLatin1String(file.readAll());
        setStyleSheet(styleSheet);

        QFile fileTool(":/qmldesigner/toolbutton.css");
        fileTool.open(QFile::ReadOnly);
        styleSheet = QLatin1String(fileTool.readAll());
        m_d->m_ui.buttonItems->setStyleSheet(styleSheet);
        m_d->m_ui.buttonResources->setStyleSheet(styleSheet);
    }

    {
        QFile file(":/qmldesigner/scrollbar.css");
        file.open(QFile::ReadOnly);
        QString styleSheet = QLatin1String(file.readAll());
        m_d->m_ui.ItemLibraryTreeView->setStyleSheet(styleSheet);
    }
}

ItemLibrary::~ItemLibrary()
{
    delete m_d;
}

void ItemLibrary::setSearchFilter(const QString &searchFilter)
{
    if (m_d->m_ui.buttonItems->isChecked()) {
        m_d->m_itemLibraryModel->setSearchText(searchFilter);
        m_d->m_itemsView->update();
        emit expandAllItems();
    } else {
        QStringList nameFilterList;
        if (searchFilter.contains('.')) {
            nameFilterList.append(QString("*%1*").arg(searchFilter));
        } else {
            foreach (const QByteArray &extension, QImageReader::supportedImageFormats()) {
                nameFilterList.append(QString("*%1*.%2").arg(searchFilter, QString::fromAscii(extension)));
            }
        }

        m_d->m_resourcesDirModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        m_d->m_resourcesDirModel->setNameFilters(nameFilterList);
        if (m_d->m_ui.ItemLibraryTreeView->model() == m_d->m_resourcesDirModel)
            m_d->m_ui.ItemLibraryTreeView->setRootIndex(m_d->m_resourcesDirModel->index(m_d->m_resourcePath));
        m_d->m_ui.ItemLibraryTreeView->expandToDepth(1);
    }
}

void ItemLibrary::itemLibraryButtonToggled()
{
    m_d->m_ui.LibraryStackedWidget->setCurrentIndex(0);
    m_d->m_ui.buttonResources->setChecked(false);
    setSearchFilter(m_d->m_ui.lineEdit->text());
}

void ItemLibrary::resourcesButtonToggled()
{
    m_d->m_ui.LibraryStackedWidget->setCurrentIndex(1);
    m_d->m_ui.buttonItems->setChecked(false);
    setSearchFilter(m_d->m_ui.lineEdit->text());
}

void ItemLibrary::setResourcePath(const QString &resourcePath)
{
    m_d->m_resourcePath = resourcePath;
}

void ItemLibrary::startDragAndDrop(int itemLibId)
{
    QMimeData *mimeData = m_d->m_itemLibraryModel->getMimeData(itemLibId);
    CustomItemLibraryDrag *drag = new CustomItemLibraryDrag(this);
    const QImage image = qvariant_cast<QImage>(mimeData->imageData());

    drag->setPixmap(m_d->m_itemLibraryModel->getIcon(itemLibId).pixmap(32, 32));
    drag->setPreview(QPixmap::fromImage(image));
    drag->setMimeData(mimeData);

    QDeclarativeItem *rootItem = qobject_cast<QDeclarativeItem*>(m_d->m_itemsView->rootObject());
    connect(rootItem, SIGNAL(stopDragAndDrop()), drag, SLOT(stopDrag()));

    drag->exec();
}

void ItemLibrary::showItemInfo(int /*itemLibId*/)
{
//    qDebug() << "showing item info about id" << itemLibId;
}

void ItemLibrary::setMetaInfo(const MetaInfo &metaInfo)
{
    m_d->m_itemLibraryModel->update(metaInfo);
}

}

