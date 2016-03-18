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

#include <QObject>
#include <QString>
#include <QSize>
#include <QVariant>

#include "itemlibraryinfo.h"

namespace QmlDesigner {

class ItemLibraryItem: public QObject {

    Q_OBJECT

    Q_PROPERTY(QVariant itemLibraryEntry READ itemLibraryEntry FINAL)
    Q_PROPERTY(QString itemName READ itemName FINAL)
    Q_PROPERTY(QString itemLibraryIconPath READ itemLibraryIconPath FINAL)
    Q_PROPERTY(bool itemVisible READ isVisible NOTIFY visibilityChanged FINAL)

public:
    ItemLibraryItem(QObject *parent);
    ~ItemLibraryItem();

    QString itemName() const;
    QString itemLibraryIconPath() const;

    bool setVisible(bool isVisible);
    bool isVisible() const;

    void setItemLibraryEntry(const ItemLibraryEntry &itemLibraryEntry);
    QVariant itemLibraryEntry() const;

signals:
    void visibilityChanged();

private:
    ItemLibraryEntry m_itemLibraryEntry;
    bool m_isVisible;
};

} // namespace QmlDesigner
