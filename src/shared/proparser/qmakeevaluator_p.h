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

#include "proitems.h"

#include <qregexp.h>

#define debugMsg if (!m_debugLevel) {} else debugMsgInternal
#define traceMsg if (!m_debugLevel) {} else traceMsgInternal
#ifdef PROEVALUATOR_DEBUG
#  define dbgBool(b) (b ? "true" : "false")
#  define dbgReturn(r) \
    (r == ReturnError ? "error" : \
     r == ReturnBreak ? "break" : \
     r == ReturnNext ? "next" : \
     r == ReturnReturn ? "return" : \
     "<invalid>")
#  define dbgKey(s) qPrintable(s.toString().toQString())
#  define dbgStr(s) qPrintable(formatValue(s, true))
#  define dbgStrList(s) qPrintable(formatValueList(s))
#  define dbgSepStrList(s) qPrintable(formatValueList(s, true))
#  define dbgStrListList(s) qPrintable(formatValueListList(s))
#  define dbgQStr(s) dbgStr(ProString(s))
#else
#  define dbgBool(b) 0
#  define dbgReturn(r) 0
#  define dbgKey(s) 0
#  define dbgStr(s) 0
#  define dbgStrList(s) 0
#  define dbgSepStrList(s) 0
#  define dbgStrListList(s) 0
#  define dbgQStr(s) 0
#endif

QT_BEGIN_NAMESPACE

namespace QMakeInternal {

struct QMakeStatics {
    QString field_sep;
    QString strtrue;
    QString strfalse;
    ProKey strCONFIG;
    ProKey strARGS;
    ProKey strARGC;
    QString strDot;
    QString strDotDot;
    QString strever;
    QString strforever;
    QString strhost_build;
    ProKey strTEMPLATE;
    ProKey strQMAKE_PLATFORM;
    ProKey strQMAKESPEC;
#ifdef PROEVALUATOR_FULL
    ProKey strREQUIRES;
#endif
    QHash<ProKey, int> expands;
    QHash<ProKey, int> functions;
    QHash<ProKey, ProKey> varMap;
    ProStringList fakeValue;
};

extern QMakeStatics statics;

}

QT_END_NAMESPACE
