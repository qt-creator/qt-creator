/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ENUMERATORMETAINFO_H
#define ENUMERATORMETAINFO_H

#include "corelib_global.h"

#include <QExplicitlySharedDataPointer>
#include <QList>
#include <QString>


namespace QmlDesigner {

class MetaInfo;

namespace Internal {

class MetaInfoParser;
class MetaInfoPrivate;
class EnumeratorMetaInfoData;

}

class CORESHARED_EXPORT EnumeratorMetaInfo
{
    friend class QmlDesigner::MetaInfo;
    friend class QmlDesigner::Internal::MetaInfoPrivate;
    friend class QmlDesigner::Internal::MetaInfoParser;
public:
    EnumeratorMetaInfo();
    ~EnumeratorMetaInfo();

    EnumeratorMetaInfo(const EnumeratorMetaInfo &other);
    EnumeratorMetaInfo& operator=(const EnumeratorMetaInfo &other);

    QString name() const;
    QString scope() const;
    bool isFlagType() const;
    bool isValid() const;
    QString scopeAndName(const QString &combiner = QString("::")) const;
    QList<QString> elementNames() const;
    int elementValue(const QString &enumeratorName) const;
    QString valueToString(int value) const;

private:
    void setScope(const QString &scope);
    void setName(const QString &name);
    void setIsFlagType(bool isFlagType);
    void setValid(bool valid);
    void addElement(const QString &enumeratorName, int enumeratorValue);

private:
    QExplicitlySharedDataPointer<Internal::EnumeratorMetaInfoData> m_data;
};

}

#endif // ENUMERATORMETAINFO_H
