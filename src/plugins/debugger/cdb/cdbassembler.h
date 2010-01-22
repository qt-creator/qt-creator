/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CDBASSEMBLER_H
#define CDBASSEMBLER_H

#include <QtCore/QList>
#include <QtCore/QString>

#include "cdbcom.h"

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace CdbCore {
    class CoreEngine;
}

namespace Debugger {
namespace Internal {

// Utilities related to assembler code.
class Register;

bool getRegisters(CIDebugControl *ctl,
                  CIDebugRegisters *ireg,
                  QList<Register> *registers,
                  QString *errorMessage,
                  int base = 10 /* 16 for hex, etc */);

bool dissassemble(CdbCore::CoreEngine *engine,
                  ULONG64 offset,
                  unsigned long beforeLines,
                  unsigned long afterLines,
                  int addressFieldWidth /* = 0*/,
                  QTextStream &str,
                  QString *errorMessage);

} // namespace Internal
} // namespace Debugger

#endif // CDBASSEMBLER_H
