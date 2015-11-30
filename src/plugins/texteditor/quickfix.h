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

#ifndef TEXTEDITORQUICKFIX_H
#define TEXTEDITORQUICKFIX_H

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

class TEXTEDITOR_EXPORT QuickFixOperations : public QList<QuickFixOperation::Ptr>
{
public:
    using QList<QuickFixOperation::Ptr>::append;
    void append(QuickFixOperation *op) { append(QuickFixOperation::Ptr(op)); }
};

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

#endif // TEXTEDITORQUICKFIX_H
