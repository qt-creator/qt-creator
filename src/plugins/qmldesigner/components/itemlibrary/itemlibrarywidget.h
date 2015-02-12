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

#ifndef ITEMLIBRARYWIDGET_H
#define ITEMLIBRARYWIDGET_H

#include "itemlibraryinfo.h"
#include "itemlibrarytreeview.h"

#include <utils/fancylineedit.h>

#include <QFrame>
#include <QToolButton>
#include <QFileIconProvider>
#include <QQuickWidget>

QT_BEGIN_NAMESPACE
class QFileSystemModel;
class QStackedWidget;
class QShortcut;
QT_END_NAMESPACE

namespace QmlDesigner {

class MetaInfo;
class ItemLibraryEntry;
class Model;


class ItemLibraryModel;
class ItemLibraryTreeView;


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

    void setImportsWidget(QWidget *importsWidget);

    static QString qmlSourcesPath();
public slots:
    void setSearchFilter(const QString &searchFilter);
    void updateModel();
    void updateSearch();

    void setResourcePath(const QString &resourcePath);

    void startDragAndDrop(QVariant itemLibId);

    void setModel(Model *model);

    void setImportFilter(FilterChangeFlag flag);

    void onQtBasicOnlyChecked(bool b);
    void onMeegoChecked(bool b);

protected:
    void removeImport(const QString &name);
    void addImport(const QString &name, const QString &version);
    void emitImportChecked();

signals:
    void itemActivated(const QString& itemName);
    void qtBasicOnlyChecked(bool b);
    void meegoChecked(bool b);

private slots:
    void setCurrentIndexOfStackedWidget(int index);
    void reloadQmlSource();

private:
    QSize m_itemIconSize;
    QSize m_resIconSize;
    ItemLibraryFileIconProvider m_iconProvider;

    QPointer<ItemLibraryInfo> m_itemLibraryInfo;

    QPointer<ItemLibraryModel> m_itemLibraryModel;
    QPointer<QFileSystemModel> m_resourcesFileSystemModel;

    QPointer<QStackedWidget> m_stackedWidget;

    QPointer<Utils::FancyLineEdit> m_filterLineEdit;
    QScopedPointer<QQuickWidget> m_itemViewQuickWidget;
    QScopedPointer<ItemLibraryTreeView> m_resourcesView;
    QShortcut *m_qmlSourceUpdateShortcut;

    QPointer<Model> m_model;
    FilterChangeFlag m_filterFlag;
    ItemLibraryEntry m_currentitemLibraryEntry;
};

}

#endif // ITEMLIBRARYWIDGET_H

