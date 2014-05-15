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

#ifndef QMLDESIGNER_ITEMLIBRARYSECTIONMODEL_H
#define QMLDESIGNER_ITEMLIBRARYSECTIONMODEL_H

#include "itemlibrarymodel.h"

#include <QObject>

namespace QmlDesigner {

class ItemLibraryItemModel;

class ItemLibrarySectionModel: public QObject {

    Q_OBJECT

    Q_PROPERTY(QObject* sectionEntries READ sectionEntries NOTIFY sectionEntriesChanged FINAL)
    Q_PROPERTY(int sectionLibraryId READ sectionLibraryId FINAL)
    Q_PROPERTY(QString sectionName READ sectionName FINAL)
    Q_PROPERTY(bool sectionExpanded READ sectionExpanded FINAL)
    Q_PROPERTY(QVariant sortingRole READ sortingRole FINAL)

public:
    ItemLibrarySectionModel(int sectionLibraryId, const QString &sectionName, QObject *parent = 0);

    QString sectionName() const;
    int sectionLibraryId() const;
    bool sectionExpanded() const;
    QVariant sortingRole() const;

    void addSectionEntry(ItemLibraryItemModel *sectionEntry);
    void removeSectionEntry(int itemLibId);
    QObject *sectionEntries();

    int visibleItemIndex(int itemLibId);
    bool isItemVisible(int itemLibId);

    bool updateSectionVisibility(const QString &searchText, bool *changed);
    void updateItemIconSize(const QSize &itemIconSize);

    bool setVisible(bool isVisible);
    bool isVisible() const;

signals:
    void sectionEntriesChanged();

private:
    ItemLibrarySortedModel m_sectionEntries;
    QString m_name;
    int m_sectionLibraryId;
    bool m_sectionExpanded;
    bool m_isVisible;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_ITEMLIBRARYSECTIONMODEL_H
