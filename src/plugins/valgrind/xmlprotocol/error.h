// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QSharedDataPointer>
#include <QVector>

QT_BEGIN_NAMESPACE
class QString;
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
