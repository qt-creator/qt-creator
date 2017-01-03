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

#include "itemlibrarysectionmodel.h"

namespace QmlDesigner {

class ItemLibrarySection: public QObject {

    Q_OBJECT

    Q_PROPERTY(QObject* sectionEntries READ sectionEntries NOTIFY sectionEntriesChanged FINAL)
    Q_PROPERTY(QString sectionName READ sectionName FINAL)
    Q_PROPERTY(bool sectionVisible READ isVisible NOTIFY visibilityChanged FINAL)
    Q_PROPERTY(bool sectionExpanded READ sectionExpanded FINAL)

public:
    ItemLibrarySection(const QString &sectionName, QObject *parent = 0);

    QString sectionName() const;
    bool sectionExpanded() const;
    QString sortingName() const;

    void addSectionEntry(ItemLibraryItem *sectionEntry);
    QObject *sectionEntries();

    bool updateSectionVisibility(const QString &searchText, bool *changed);

    bool setVisible(bool isVisible);
    bool isVisible() const;

    void sortItems();

    void setSectionExpanded(bool expanded);

signals:
    void sectionEntriesChanged();
    void visibilityChanged();

private:
    ItemLibrarySectionModel m_sectionEntries;
    QString m_name;
    bool m_sectionExpanded;
    bool m_isVisible;
};

} // namespace QmlDesigner
