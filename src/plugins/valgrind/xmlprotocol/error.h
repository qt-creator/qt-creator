/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef LIBVALGRIND_PROTOCOL_ERROR_H
#define LIBVALGRIND_PROTOCOL_ERROR_H

#include <QMetaType>
#include <QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QString;
template <typename T> class QVector;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {

class Frame;
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

#endif // LIBVALGRIND_PROTOCOL_ERROR_H
