// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QList>
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
    using Ptr = QSharedPointer<QuickFixOperation>;

public:
    QuickFixOperation(int priority = -1);
    virtual ~QuickFixOperation();

    /*!
        Returns The priority for this quick-fix. See the QuickFixCollector for more information.
     */
    virtual int priority() const;

    /// Sets the priority for this quick-fix operation.
    void setPriority(int priority);

    /*!
        Returns The description for this quick-fix. This description is shown to the user.
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

using QuickFixOperations = QList<QuickFixOperation::Ptr>;

inline QuickFixOperations &operator<<(QuickFixOperations &list, QuickFixOperation *op)
{
    list.append(QuickFixOperation::Ptr(op));
    return list;
}

using QuickFixInterface = QSharedPointer<const AssistInterface>;

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::QuickFixOperation::Ptr)
