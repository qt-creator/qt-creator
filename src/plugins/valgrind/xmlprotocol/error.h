// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMetaType>
#include <QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Valgrind::XmlProtocol {

Q_NAMESPACE

class Stack;
class Suppression;

/**
 * Error kinds, specific to memcheck
 */
enum MemcheckError
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
    ReallocSizeZero
};
Q_ENUM_NS(MemcheckError);

enum PtrcheckError
{
    SorG,
    Heap,
    Arith,
    SysParam
};
Q_ENUM_NS(PtrcheckError);

enum HelgrindError
{
    Race,
    UnlockUnlocked,
    UnlockForeign,
    UnlockBogus,
    PthAPIerror,
    LockOrder,
    Misc
};
Q_ENUM_NS(HelgrindError);

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

    QList<Stack> stacks() const;
    void setStacks(const QList<Stack> &stacks);

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

} // namespace Valgrind::XmlProtocol

Q_DECLARE_METATYPE(Valgrind::XmlProtocol::Error)
