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
class ItemLibrarySection;

class ItemLibraryModel: public QAbstractListModel {

    Q_OBJECT
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)

public:
    explicit ItemLibraryModel(QObject *parent = 0);
    ~ItemLibraryModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QString searchText() const;

    void update(ItemLibraryInfo *itemLibraryInfo, Model *model);

    QMimeData *getMimeData(const ItemLibraryEntry &itemLibraryEntry);

    QList<ItemLibrarySection*> sections() const;

    void clearSections();

    static void registerQmlTypes();

    int visibleSectionCount() const;
    QList<ItemLibrarySection*> visibleSections() const;

    ItemLibrarySection *sectionByName(const QString &sectionName);

public slots:
    void setSearchText(const QString &searchText);

    void setExpanded(bool, const QString &section);

signals:
    void qmlModelChanged();
    void searchTextChanged();

private: // functions
    void updateVisibility();
    void addRoleNames();
    void resetModel();


private: // variables
    QList<ItemLibrarySection*> m_sections;
    QHash<int, QByteArray> m_roleNames;

    QString m_searchText;
};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::ItemLibraryModel)
#endif // ITEMLIBRARYMODEL_H

