/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cdbassembler.h"
#include "registerhandler.h"
#include "cdbdebugengine_p.h"
#include "cdbsymbolgroupcontext.h"

#include <QtCore/QVector>

namespace Debugger {
namespace Internal {

bool getRegisters(IDebugControl4 *ctl,
                  IDebugRegisters2 *ireg,
                  QList<Register> *registers,
                  QString *errorMessage, int base)
{
    registers->clear();
    ULONG count;
    HRESULT hr = ireg->GetNumberRegisters(&count);
    if (FAILED(hr)) {
        *errorMessage= msgComFailed("GetNumberRegisters", hr);
        return false;
    }
    if (!count)
        return true;
    // Retrieve names
    WCHAR wszBuf[MAX_PATH];
    for (ULONG r = 0; r < count; r++) {
        hr = ireg->GetDescriptionWide(r, wszBuf, MAX_PATH - 1, 0, 0);
        if (FAILED(hr)) {
            *errorMessage= msgComFailed("GetDescriptionWide", hr);
            return false;
        }
        Register reg;
        reg.name = QString::fromUtf16(wszBuf);
        registers->push_back(reg);
    }
    // get values
    QVector<DEBUG_VALUE> values(count);
    DEBUG_VALUE *valuesPtr = &(*values.begin());
    memset(valuesPtr, 0, count * sizeof(DEBUG_VALUE));
    hr = ireg->GetValues(count, 0, 0, valuesPtr);
    if (FAILED(hr)) {
        *errorMessage= msgComFailed("GetValues", hr);
        return false;
    }
    if (base < 2)
        base = 10;
    for (ULONG r = 0; r < count; r++)
        (*registers)[r].value = CdbSymbolGroupContext::debugValueToString(values.at(r), ctl, 0, base);
    return true;
}

}
}
