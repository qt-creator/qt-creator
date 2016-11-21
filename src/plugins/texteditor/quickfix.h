/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "texteditor_global.h"

#include <QString>
#include <QMetaType>
#include <QSharedPointer>

namespace TextEditor {

class AssistInterface;

/*!
    Class to perform a single quick-fix.

    Quick-fix operations cannot be copied, and must be passed around as explicitly
    shared pointers ( QuickFixOperation::Ptr ).

    Subclasses should make sure that they copy parts of, or the whole QuickFixState ,
    which are needed to perform the quick-fix operation.
 */
class TEXTEDITOR_EXPORT QuickFixOperation
{
    Q_DISABLE_COPY(QuickFixOperation)

public:
    typedef QSharedPointer<QuickFixOperation> Ptr;

public:
    QuickFixOperation(int priority = -1);
    virtual ~QuickFixOperation();

    /*!
        \returns The priority for this quick-fix. See the QuickFixCollector for more
                 information.
     */
    virtual int priority() const;

    /// Sets the priority for this quick-fix operation.
    void setPriority(int priority);

    /*!
        \returns The description for this quick-fix. This description is shown to the
                 user.
     */
    virtual QString description() const;

    /// Sets the description for this quick-fix, which will be shown to the user.
    void setDescription(const QString &description);

    /*!
        Perform this quick-fix's operation.

        Subclasses should implement this function to do the actual changes.
     */
    virtual void perform() = 0;

private:
    int _priority;
    QString _description;
};

typedef QList<QuickFixOperation::Ptr> QuickFixOperations;

inline QuickFixOperations &operator<<(QuickFixOperations &list, QuickFixOperation *op)
{
    list.append(QuickFixOperation::Ptr(op));
    return list;
}

typedef QSharedPointer<const AssistInterface> QuickFixInterface;

/*!
    The QuickFixFactory is responsible for generating QuickFixOperation s which are
    applicable to the given QuickFixState.

    A QuickFixFactory should not have any state -- it can be invoked multiple times
    for different QuickFixState objects to create the matching operations, before any
    of those operations are applied (or released).

    This way, a single factory can be used by multiple editors, and a single editor
    can have multiple QuickFixCollector objects for different parts of the code.
 */
class TEXTEDITOR_EXPORT QuickFixFactory: public QObject
{
    Q_OBJECT

public:
    QuickFixFactory(QObject *parent = 0);
    ~QuickFixFactory();

    virtual void matchingOperations(const QuickFixInterface &interface, QuickFixOperations &result) = 0;
};

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::QuickFixOperation::Ptr)
