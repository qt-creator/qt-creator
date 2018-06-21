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

#pragma once

#include "itemlibraryinfo.h"
#include "itemlibraryresourceview.h"

#include <utils/fancylineedit.h>

#include <QFrame>
#include <QToolButton>
#include <QFileIconProvider>
#include <QQuickWidget>
#include <QQmlPropertyMap>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QShortcut;
QT_END_NAMESPACE

namespace QmlDesigner {

class MetaInfo;
class ItemLibraryEntry;
class Model;
class CustomFileSystemModel;


class ItemLibraryModel;
class ItemLibraryResourceView;

class ItemLibraryWidget : public QFrame
{
    Q_OBJECT

    enum FilterChangeFlag {
      QtBasic = 0x0,
      Meego = 0x1
    };

public:
    ItemLibraryWidget(QWidget *parent = nullptr);

    void setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo);
    QList<QToolButton *> createToolBarWidgets();

    void updateImports();

    void setImportsWidget(QWidget *importsWidget);

    static QString qmlSourcesPath();
    void clearSearchFilter();

    void setSearchFilter(const QString &searchFilter);
    void delayedUpdateModel();
    void updateModel();
    void updateSearch();

    void setResourcePath(const QString &resourcePath);

    void setModel(Model *model);

    Q_INVOKABLE void startDragAndDrop(QQuickItem *mouseArea, QVariant itemLibId);

signals:
    void itemActivated(const QString& itemName);

private:
    void setCurrentIndexOfStackedWidget(int index);
    void reloadQmlSource();
    void setupImportTagWidget();
    void removeImport(const QString &name);
    void addImport(const QString &name, const QString &version);
    void addPossibleImport(const QString &name);
    void addResources();

    QTimer m_compressionTimer;
    QSize m_itemIconSize;

    QPointer<ItemLibraryInfo> m_itemLibraryInfo;

    QPointer<ItemLibraryModel> m_itemLibraryModel;
    QPointer<CustomFileSystemModel> m_resourcesFileSystemModel;

    QPointer<QStackedWidget> m_stackedWidget;

    QPointer<Utils::FancyLineEdit> m_filterLineEdit;
    QScopedPointer<QQuickWidget> m_itemViewQuickWidget;
    QScopedPointer<ItemLibraryResourceView> m_resourcesView;
    QScopedPointer<QWidget> m_importTagsWidget;
    QScopedPointer<QWidget> m_addResourcesWidget;

    QShortcut *m_qmlSourceUpdateShortcut;

    QPointer<Model> m_model;
    FilterChangeFlag m_filterFlag;
    ItemLibraryEntry m_currentitemLibraryEntry;
};

}
