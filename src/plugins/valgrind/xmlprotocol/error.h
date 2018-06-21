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

#include <QMetaType>
#include <QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QString;
template <typename T> class QVector;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {

class Stack;
class Suppression;

/**
 * Error kinds, specific to memcheck
 */
enum MemcheckErrorKind
{
    InvalidFree,
    MismatchedFree,
    InvalidRead,
    InvalidWrite,
    InvalidJump,
    Overlap,
    InvalidMemPool,
    UninitCondition,
    UninitValue,
    SyscallParam,
    ClientCheck,
    Leak_DefinitelyLost,
    Leak_PossiblyLost,
    Leak_StillReachable,
    Leak_IndirectlyLost,
    MemcheckErrorKindCount
};

enum PtrcheckErrorKind
{
    SorG,
    Heap,
    Arith,
    SysParam
};

enum HelgrindErrorKind
{
    Race,
    UnlockUnlocked,
    UnlockForeign,
    UnlockBogus,
    PthAPIerror,
    LockOrder,
    Misc
};

class Error
{
public:
    Error();
    ~Error();

    Error(const Error &other);

    Error &operator=(const Error &other);
    void swap(Error &other);

    bool operator==(const Error &other) const;
    bool operator!=(const Error &other) const;

    qint64 unique() const;
    void setUnique(qint64 unique);

    qint64 tid() const;
    void setTid(qint64);

    QString what() const;
    void setWhat(const QString &what);

    int kind() const;
    void setKind(int kind);

    QVector<Stack> stacks() const;
    void setStacks(const QVector<Stack> &stacks);

    Suppression suppression() const;
    void setSuppression(const Suppression &suppression);

    //memcheck
    quint64 leakedBytes() const;
    void setLeakedBytes(quint64);

    qint64 leakedBlocks() const;
    void setLeakedBlocks(qint64 blocks);

    //helgrind
    qint64 helgrindThreadId() const;
    void setHelgrindThreadId( qint64 threadId );

    QString toXml() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace XmlProtocol
} // namespace Valgrind

Q_DECLARE_METATYPE(Valgrind::XmlProtocol::Error)
