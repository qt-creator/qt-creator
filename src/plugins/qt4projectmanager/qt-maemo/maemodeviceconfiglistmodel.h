/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMODEVICECONFIGLISTMODEL_H
#define MAEMODEVICECONFIGLISTMODEL_H

#include "maemodeviceconfigurations.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QVariantMap>

namespace Qt4ProjectManager {
namespace Internal {

class MaemoDeviceConfigListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit MaemoDeviceConfigListModel(QObject *parent = 0);
    void setCurrentIndex(int index);
    MaemoDeviceConfig current() const;
    int currentIndex() const { return m_currentIndex; }

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
        int role = Qt::DisplayRole) const;

signals:
    void currentChanged();

private:
    Q_SLOT void handleDeviceConfigListChange();
    void resetCurrentIndex();
    void setInvalid();
    void setupList();

    QList<MaemoDeviceConfig> m_devConfigs;
    quint64 m_currentId;
    int m_currentIndex;
};


} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEVICECONFIGLISTMODEL_H
