/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CALLGRINDFUNCTIONCALL_H
#define CALLGRINDFUNCTIONCALL_H

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
    Q_DISABLE_COPY(FunctionCall)

    class Private;
    Private *d;
};

} // namespace Callgrind
} // namespace Valgrind

Q_DECLARE_METATYPE(const Valgrind::Callgrind::FunctionCall *)

#endif // CALLGRINDFUNCTIONCALL_H
