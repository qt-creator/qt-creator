/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

#include <QSharedDataPointer>

QT_BEGIN_NAMESPACE
template <typename T> class QVector;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {

class Frame;

class Stack
{
public:
    Stack();
    Stack(const Stack &other);
    ~Stack();
    void operator=(const Stack &other);
    void swap(Stack &other);
    bool operator==(const Stack &other) const;

    QString auxWhat() const;
    void setAuxWhat(const QString &auxwhat);

    QVector<Frame> frames() const;
    void setFrames(const QVector<Frame> &frames);

    //memcheck, ptrcheck, helgrind:
    QString file() const;
    void setFile(const QString &file);

    QString directory() const;
    void setDirectory(const QString &directory);

    qint64 line() const;
    void setLine(qint64 line);

    //helgrind:
    qint64 helgrindThreadId() const;
    void setHelgrindThreadId(qint64 threadId );

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace XmlProtocol
} // namespace Stack
