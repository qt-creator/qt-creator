/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QVARIANTPARSER_H
#define QVARIANTPARSER_H

#include <QVariant>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QDeclarativeValueType;
QT_END_NAMESPACE

namespace QmlDesigner {
namespace Internal {

class VariantParser
{
public:
    VariantParser(const QVariant &value);
    ~VariantParser();
    QVariant value() const;
    QVariant property(QString name) const;
    bool setProperty(const QString &name, const QVariant &value);
    bool isValid();
    QStringList properties();
    void init(const QString &type);

    static bool isValueType(const QString &type);
    static VariantParser create(const QString &type);
private:
    QDeclarativeValueType *m_valueType;
};



} // namespace Internal
} // namespace QmlDesigner

#endif // QVARIANTPARSER_H
