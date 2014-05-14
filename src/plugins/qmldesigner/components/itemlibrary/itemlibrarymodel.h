/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef ITEMLIBRARYMODEL_H
#define ITEMLIBRARYMODEL_H

#include <QMap>
#include <QIcon>
#include <QAbstractListModel>
#include <QtQuick>

QT_FORWARD_DECLARE_CLASS(QMimeData)

namespace QmlDesigner {

class ItemLibraryInfo;
class ItemLibraryEntry;
class Model;

void registerQmlTypes();

class ItemLibrarySortedModel: public QAbstractListModel {

    Q_OBJECT

public:
    ItemLibrarySortedModel(QObject *parent = 0);
    ~ItemLibrarySortedModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void clearElements();

    void addElement(QObject *element, int libId);
    void removeElement(int libId);

    bool elementVisible(int libId) const;
    bool setElementVisible(int libId, bool visible);

    void privateInsert(int pos, QObject *element);
    void privateRemove(int pos);

    const QMap<int, QObject *> &elements() const;

    template<typename T>
    const QList<T> elementsByType() const;

    QObject *element(int libId);

    template<typename T>
    T elementByType(int libId);

    int findElement(int libId) const;
    int visibleElementPosition(int libId) const;

    void resetModel();

private:
    void addRoleName(const QByteArray &roleName);

    struct order_struct {
        int libId;
        bool visible;
    };

    QMap<int, QObject *> m_elementModels;
    QList<struct order_struct> m_elementOrder;

    QList<QObject *> m_privList;
    QHash<int, QByteArray> m_roleNames;
};

class ItemLibraryItemModel: public QObject {

    Q_OBJECT

    Q_PROPERTY(int itemLibId READ itemLibId FINAL)
    Q_PROPERTY(QString itemName READ itemName FINAL)
    Q_PROPERTY(QString itemLibraryIconPath READ itemLibraryIconPath FINAL)
    Q_PROPERTY(QVariant sortingRole READ sortingRole FINAL)

public:
    ItemLibraryItemModel(int itemLibId, const QString &itemName, QObject *parent);
    ~ItemLibraryItemModel();

    int itemLibId() const;
    QString itemName() const;
    QString itemLibraryIconPath() const;
    QVariant sortingRole() const;

    void setItemIconPath(const QString &iconPath);
    void setItemIconSize(const QSize &itemIconSize);

private:
    int m_libId;
    QString m_name;
    QString m_iconPath;
    QSize m_iconSize;
};


class ItemLibrarySectionModel: public QObject {

    Q_OBJECT

    Q_PROPERTY(QObject* sectionEntries READ sectionEntries NOTIFY sectionEntriesChanged FINAL)
    Q_PROPERTY(int sectionLibId READ sectionLibId FINAL)
    Q_PROPERTY(QString sectionName READ sectionName FINAL)
    Q_PROPERTY(bool sectionExpanded READ sectionExpanded FINAL)
    Q_PROPERTY(QVariant sortingRole READ sortingRole FINAL)

public:
    ItemLibrarySectionModel(int sectionLibId, const QString &sectionName, QObject *parent = 0);

    QString sectionName() const;
    int sectionLibId() const;
    bool sectionExpanded() const;
    QVariant sortingRole() const;

    void addSectionEntry(ItemLibraryItemModel *sectionEntry);
    void removeSectionEntry(int itemLibId);
    QObject *sectionEntries();

    int visibleItemIndex(int itemLibId);
    bool isItemVisible(int itemLibId);

    bool updateSectionVisibility(const QString &searchText, bool *changed);
    void updateItemIconSize(const QSize &itemIconSize);

signals:
    void sectionEntriesChanged();

private:
    QString m_name;
    int m_sectionLibId;
    bool m_sectionExpanded;
    ItemLibrarySortedModel m_sectionEntries;
};


class ItemLibraryModel: public ItemLibrarySortedModel {

    Q_OBJECT
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)

public:
    explicit ItemLibraryModel(QObject *parent = 0);
    ~ItemLibraryModel();

    QString searchText() const;

    void update(ItemLibraryInfo *itemLibraryInfo, Model *model);

    QString getTypeName(int libId);
    QMimeData *getMimeData(int libId);
    QIcon getIcon(int libId);

    ItemLibrarySectionModel* section(int libId);
    QList<ItemLibrarySectionModel*> sections() const;

public slots:
    void setSearchText(const QString &searchText);
    void setItemIconSize(const QSize &itemIconSize);

    int getItemSectionIndex(int itemLibId);
    int getSectionLibId(int itemLibId);
    bool isItemVisible(int itemLibId);
    void setExpanded(bool, const QString &section);

signals:
    void qmlModelChanged();
    void searchTextChanged();
    void visibilityChanged();
    void sectionVisibilityChanged(int changedSectionLibId);

private:
    void updateVisibility();

    int getWidth(const ItemLibraryEntry &entry);
    int getHeight(const ItemLibraryEntry &entry);
    QPixmap createDragPixmap(int width, int height);

    QMap<int, ItemLibraryEntry> m_itemInfos;
    QMap<int, int> m_sections;

    QString m_searchText;
    QSize m_itemIconSize;
    int m_nextLibId;
};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::ItemLibrarySortedModel)

#endif // ITEMLIBRARYMODEL_H

