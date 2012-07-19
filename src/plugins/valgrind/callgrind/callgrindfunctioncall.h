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

#ifndef LIBVALGRIND_CALLGRIND_CALLEE_H
#define LIBVALGRIND_CALLGRIND_CALLEE_H

#include <QMetaType>

QT_BEGIN_NAMESPACE
template <typename T> class QVector;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {

class Function;

/**
 * This represents a function call.
 */
class FunctionCall
{
public:
    explicit FunctionCall();
    ~FunctionCall();

    /// the called function
    const Function *callee() const;
    void setCallee(const Function *function);

    /// the calling function
    const Function *caller() const;
    void setCaller(const Function *function);

    /// how often the function was called
    quint64 calls() const;
    void setCalls(quint64 calls);

    /**
     * Destination position data for the given position-index @p posIdx
     * @see ParseData::positions()
     */
    quint64 destination(int posIdx) const;
    QVector<quint64> destinations() const;
    void setDestinations(const QVector<quint64> &destinations);

    /**
     * Inclusive cost of the function call.
     * @see ParseData::events()
     */
    quint64 cost(int event) const;
    QVector<quint64> costs() const;
    void setCosts(const QVector<quint64> &costs);

private:
    Q_DISABLE_COPY(FunctionCall);

    class Private;
    Private *d;
};

} // namespace Callgrind
} // namespace Valgrind

Q_DECLARE_METATYPE(const Valgrind::Callgrind::FunctionCall *);

#endif // LIBVALGRIND_CALLGRIND_CALLEE_H
