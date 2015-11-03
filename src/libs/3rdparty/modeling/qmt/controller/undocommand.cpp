/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#include "undocommand.h"

#include "qmt/infrastructure/qmtassert.h"

namespace qmt {

UndoCommand::UndoCommand(const QString &text)
    : QUndoCommand(text),
      m_canRedo(false),
      m_doNotMerge(false)
{
}

UndoCommand::~UndoCommand()
{
}

int UndoCommand::id() const
{
    return 1;
}

void UndoCommand::setDoNotMerge(bool do_not_merge)
{
    m_doNotMerge = do_not_merge;
}

bool UndoCommand::mergeWith(const QUndoCommand *other)
{
    const UndoCommand *other_command = dynamic_cast<const UndoCommand *>(other);
    if (!other_command) {
        return false;
    }
    if (other_command->m_doNotMerge) {
        return false;
    }
    return mergeWith(other_command);
}

bool UndoCommand::mergeWith(const UndoCommand *other)
{
    Q_UNUSED(other);

    return false;
}

void UndoCommand::undo()
{
    QMT_CHECK(!m_canRedo);
    m_canRedo = true;
}

void UndoCommand::redo()
{
    QMT_CHECK(m_canRedo);
    m_canRedo = false;
}

}
