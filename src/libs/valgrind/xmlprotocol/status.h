/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

#ifndef LIBVALGRIND_PROTOCOL_STATUS_H
#define LIBVALGRIND_PROTOCOL_STATUS_H

#include "../valgrind_global.h"

#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {

class VALGRINDSHARED_EXPORT Status
{
public:
    enum State {
        Running,
        Finished
    };

    Status();
    Status(const Status &other);
    ~Status();
    Status &operator=(const Status &other);
    void swap(Status &other);
    bool operator==(const Status &other) const;

    State state() const;
    void setState(State state);

    QString time() const;
    void setTime(const QString &time);

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace XmlProtocol
} // namespace Valgrind

Q_DECLARE_METATYPE(Valgrind::XmlProtocol::Status)

#endif // LIBVALGRIND_PROTOCOL_STATUS_H
