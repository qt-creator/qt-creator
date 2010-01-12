/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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


namespace QmlDesigner {

class MyFileIconProvider : public QFileIconProvider
{
public:
    MyFileIconProvider() : QFileIconProvider()
    {}
    virtual QIcon icon ( const QFileInfo & info ) const
    {
        QPixmap pixmap(info.absoluteFilePath());
        if (pixmap.isNull())
            return QFileIconProvider::icon(info);
        else return pixmap; //pixmap.scaled(128, 128, Qt::KeepAspectRatio);
    }
};



class GrabHelper {
    Q_DISABLE_COPY(GrabHelper)
public:
    GrabHelper();
    QPixmap grabItem(QGraphicsItem *item);

private:
    QGraphicsScene m_scene;
    QGraphicsView m_view;
};

GrabHelper::GrabHelper()
{
    m_view.setScene(&m_scene);
    m_view.setFrameShape(QFrame::NoFrame);
    m_view.setAlignment(Qt::AlignLeft|Qt::AlignTop);
    m_view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

QPixmap GrabHelper::grabItem(QGraphicsItem *item)
{
    if (item->scene()) {
        qWarning("%s: WARNING: Attempt to grab item that is part of another scene!", Q_FUNC_INFO);
        return QPixmap();
    }
    // Temporarily add the item, resize the view widget and grab it.
    m_scene.addItem(item);
    item->setPos(0.0, 0.0);
    const QSize size = item->boundingRect().size().toSize();
    QPixmap rc;
    if (!size.isEmpty()) { // We have seen horses barf...
        m_view.resize(size);
        rc = QPixmap::grabWidget(&m_view);
    }
    m_scene.removeItem(item);
    return rc;
}

// ---------- ItemLibraryPrivate
class ItemLibraryPrivate {
public:
    ItemLibraryPrivate(QObject *object);
    ~ItemLibraryPrivate();

    Ui::ItemLibrary m_ui;
    Internal::ItemLibraryModel *m_itemLibraryModel;
    QDirModel *m_resourcesDirModel;
    QSortFilterProxyModel *m_filterProxy;
    GrabHelper *m_grabHelper;
    QString m_resourcePath;
};

ItemLibraryPrivate::ItemLibraryPrivate(QObject *object) :
    m_itemLibraryModel(0),
    m_grabHelper(0)
{
    m_resourcePath = QDir::currentPath();
    Q_UNUSED(object);
}

ItemLibraryPrivate::~ItemLibraryPrivate()
{
    delete m_grabHelper;
}

ItemLibrary::ItemLibrary(QWidget *parent) :
    QFrame(parent),
    m_d(new ItemLibraryPrivate(this))
{
    m_d->m_ui.setupUi(this);
    m_d->m_itemLibraryModel = new Internal::ItemLibraryModel(this);
    m_d->m_resourcesDirModel = new QDirModel(this);
    m_d->m_filterProxy = new QSortFilterProxyModel(this);
    m_d->m_filterProxy->setSourceModel(m_d->m_itemLibraryModel);
    m_d->m_ui.ItemLibraryTreeView->setModel(m_d->m_filterProxy);
    m_d->m_filterProxy->setDynamicSortFilter(true);
    m_d->m_ui.ItemLibraryTreeView->setRealModel(m_d->m_itemLibraryModel);
    m_d->m_ui.ItemLibraryTreeView->setIconSize(QSize(64, 64));
    m_d->m_ui.ItemLibraryTreeView->setSortingEnabled(true);
    m_d->m_ui.ItemLibraryTreeView->setHeaderHidden(true);
    m_d->m_ui.ItemLibraryTreeView->setIndentation(10);
    m_d->m_ui.ItemLibraryTreeView->setAnimated(true);
    m_d->m_ui.ItemLibraryTreeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_d->m_ui.ItemLibraryTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_d->m_ui.ItemLibraryTreeView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_d->m_filterProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_d->m_filterProxy->setFilterRole(Qt::UserRole);
    m_d->m_filterProxy->setSortRole(Qt::DisplayRole);
    connect(m_d->m_ui.lineEdit, SIGNAL(textChanged(QString)), m_d->m_filterProxy, SLOT(setFilterRegExp(QString)));
    connect(m_d->m_ui.lineEdit, SIGNAL(textChanged(QString)), this, SLOT(setNameFilter(QString)));
    connect(m_d->m_ui.lineEdit, SIGNAL(textChanged(QString)), this, SLOT(expandAll()));
    connect(m_d->m_ui.buttonItems, SIGNAL(toggled (bool)), this, SLOT(itemLibraryButton()));
    connect(m_d->m_ui.buttonResources, SIGNAL(toggled (bool)), this, SLOT(resourcesButton()));
    connect(m_d->m_ui.ItemLibraryTreeView, SIGNAL(itemActivated(const QString&)), this, SIGNAL(itemActivated(const QString&)));
    m_d->m_ui.lineEdit->setDragEnabled(false);
    setNameFilter("");

    MyFileIconProvider *fileIconProvider = new MyFileIconProvider();
    m_d->m_resourcesDirModel->setIconProvider(fileIconProvider);

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

    m_d->m_ui.buttonItems->setChecked(true);
}

ItemLibrary::~ItemLibrary()
{
    delete m_d;
}

void ItemLibrary::setNameFilter(const QString &nameFilter)
{
    QStringList nameFilterList;
    nameFilterList.append(nameFilter + "*.gif");
    nameFilterList.append(nameFilter + "*.png");
    nameFilterList.append(nameFilter + "*.jpg");
    nameFilterList.append(nameFilter + "*.");
    m_d->m_resourcesDirModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_d->m_resourcesDirModel->setNameFilters(nameFilterList);
    if (m_d->m_ui.ItemLibraryTreeView->model() == m_d->m_resourcesDirModel)
        m_d->m_ui.ItemLibraryTreeView->setRootIndex(m_d->m_resourcesDirModel->index(m_d->m_resourcePath));
}

void ItemLibrary::itemLibraryButton()
{
    if (m_d->m_ui.buttonItems->isChecked()) {
        m_d->m_filterProxy->setSourceModel(m_d->m_itemLibraryModel);
        m_d->m_ui.ItemLibraryTreeView->setModel(m_d->m_filterProxy);
        m_d->m_ui.ItemLibraryTreeView->setIconSize(QSize(64, 64));
        m_d->m_ui.buttonResources->setChecked(false);
        m_d->m_ui.ItemLibraryTreeView->setRealModel(m_d->m_itemLibraryModel);
        expandAll();
    }
}

void ItemLibrary::resourcesButton()
{
    if (m_d->m_ui.buttonResources->isChecked()) {
        m_d->m_ui.ItemLibraryTreeView->setModel(m_d->m_resourcesDirModel);
        m_d->m_ui.ItemLibraryTreeView->setIconSize(QSize(32, 32));
        m_d->m_ui.buttonItems->setChecked(false);
        m_d->m_ui.ItemLibraryTreeView->setRootIndex(m_d->m_resourcesDirModel->index(m_d->m_resourcePath));
        m_d->m_ui.ItemLibraryTreeView->setColumnHidden(1, true);
        m_d->m_ui.ItemLibraryTreeView->setColumnHidden(2, true);
        m_d->m_ui.ItemLibraryTreeView->setColumnHidden(3, true);
        expandAll();
    }
}

void ItemLibrary::addItemLibraryInfo(const ItemLibraryInfo &itemLibraryInfo)
{
    m_d->m_itemLibraryModel->addItemLibraryInfo(itemLibraryInfo);
}

void ItemLibrary::setResourcePath(const QString &resourcePath)
{
    m_d->m_resourcePath = resourcePath;
}

void ItemLibrary::setMetaInfo(const MetaInfo &metaInfo)
{
    m_d->m_itemLibraryModel->clear();

    foreach (const QString &type, metaInfo.itemLibraryItems()) {
        NodeMetaInfo nodeInfo = metaInfo.nodeMetaInfo(type);

        QList<ItemLibraryInfo> itemLibraryRepresentationList = metaInfo.itemLibraryRepresentations(nodeInfo);

        if (!metaInfo.hasNodeMetaInfo(type))
            qWarning() << "ItemLibrary: type not declared: " << type;
        if (!itemLibraryRepresentationList.isEmpty() && metaInfo.hasNodeMetaInfo(type)) {
            foreach (ItemLibraryInfo itemLibraryRepresentation, itemLibraryRepresentationList) {
                QImage image(64, 64, QImage::Format_RGB32); // = m_d->m_queryView->paintObject(nodeInfo, itemLibraryRepresentation.properties()); TODO
                if (!image.isNull()) {
                    QPainter p(&image);
                    QPen pen(Qt::gray);
                    pen.setWidth(2);
                    p.setPen(pen);
                    p.drawRect(1, 1, image.width() - 2, image.height() - 2);
                }
                QIcon icon = itemLibraryRepresentation.icon();
                if (itemLibraryRepresentation.icon().isNull())
                    itemLibraryRepresentation.setIcon(QIcon(":/ItemLibrary/images/default-icon.png"));

                if (itemLibraryRepresentation.category().isEmpty())
                    itemLibraryRepresentation.setCategory(nodeInfo.category());
                if (!image.isNull()) {
                    itemLibraryRepresentation.setDragIcon(QPixmap::fromImage(image));
                    addItemLibraryInfo(itemLibraryRepresentation);
                }
            }
        } else {
            QImage image; // = m_d->m_queryView->paintObject(nodeInfo); TODO we have to render image
            QIcon icon = nodeInfo.icon();
            if (icon.isNull())
                icon = QIcon(":/ItemLibrary/images/default-icon.png");

            ItemLibraryInfo itemLibraryInfo;
            itemLibraryInfo.setName(type);
            itemLibraryInfo.setTypeName(nodeInfo.typeName());
            itemLibraryInfo.setCategory(nodeInfo.category());
            itemLibraryInfo.setIcon(icon);
            itemLibraryInfo.setMajorVersion(nodeInfo.majorVersion());
            itemLibraryInfo.setMinorVersion(nodeInfo.minorVersion());
            itemLibraryInfo.setDragIcon(QPixmap::fromImage(image));
            addItemLibraryInfo(itemLibraryInfo);
        }
    }
    expandAll();
}

void ItemLibrary::expandAll()
{
    m_d->m_ui.ItemLibraryTreeView->expandToDepth(1);
}

void ItemLibrary::contextMenuEvent (QContextMenuEvent *event)
{
    event->accept();
    QMenu menu;
    menu.addAction(tr("About plugins..."));
    menu.exec(event->globalPos());
}

}
