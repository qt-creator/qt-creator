/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef ITEMLIBRARYWIDGET_H
#define ITEMLIBRARYWIDGET_H

#include "itemlibraryinfo.h"
#include "itemlibrarycomponents.h"

#include <utils/filterlineedit.h>

#include <QFrame>
#include <QToolButton>
#include <QFileIconProvider>
#include <QDeclarativeView>

QT_BEGIN_NAMESPACE
class QFileSystemModel;
class QStackedWidget;
QT_END_NAMESPACE

namespace QmlDesigner {

class MetaInfo;
class ItemLibraryEntry;
class Model;

namespace Internal {
    class ItemLibraryModel;
    class ItemLibraryTreeView;
}

class ItemLibraryFileIconProvider : public QFileIconProvider
{
public:
    ItemLibraryFileIconProvider(const QSize &iconSize);

    QIcon icon( const QFileInfo & info ) const;

private:
    QSize m_iconSize;
};


class ItemLibraryWidget : public QFrame
{
    Q_OBJECT

    enum FilterChangeFlag {
      QtBasic = 0x0,
      Meego = 0x1
    };

public:
    ItemLibraryWidget(QWidget *parent = 0);

    void setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo);
    QList<QToolButton *> createToolBarWidgets();

    void updateImports();

public Q_SLOTS:
    void setSearchFilter(const QString &searchFilter);
    void updateModel();
    void updateSearch();

    void setResourcePath(const QString &resourcePath);

    void startDragAndDrop(int itemLibId);
    void showItemInfo(int itemLibId);

    void setModel(Model *model);

    void setImportFilter(FilterChangeFlag flag);

    void onQtBasicOnlyChecked(bool b);
    void onMeegoChecked(bool b);

protected:
    void wheelEvent(QWheelEvent *event);
    void removeImport(const QString &name);
    void addImport(const QString &name, const QString &version);
    void emitImportChecked();

signals:
    void itemActivated(const QString& itemName);
    void scrollItemsView(QVariant delta);
    void resetItemsView();
    void qtBasicOnlyChecked(bool b);
    void meegoChecked(bool b);

private:
    ItemLibraryFileIconProvider m_iconProvider;
    QSize m_itemIconSize;
    QSize m_resIconSize;

    QWeakPointer<ItemLibraryInfo> m_itemLibraryInfo;

    QWeakPointer<Internal::ItemLibraryModel> m_itemLibraryModel;
    QWeakPointer<QFileSystemModel> m_resourcesFileSystemModel;

    QWeakPointer<QStackedWidget> m_stackedWidget;
    QWeakPointer<Utils::FilterLineEdit> m_lineEdit;
    QScopedPointer<QDeclarativeView> m_itemsView;
    QScopedPointer<Internal::ItemLibraryTreeView> m_resourcesView;

    QWeakPointer<Model> m_model;
    FilterChangeFlag m_filterFlag;
};

}

#endif // ITEMLIBRARYWIDGET_H

