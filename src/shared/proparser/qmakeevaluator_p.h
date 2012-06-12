/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMAKEEVALUATOR_P_H
#define QMAKEEVALUATOR_P_H

#include "proitems.h"

#include <QRegExp>

QT_BEGIN_NAMESPACE

namespace ProFileEvaluatorInternal {

struct QMakeStatics {
    QString field_sep;
    QString strtrue;
    QString strfalse;
    ProString strCONFIG;
    ProString strARGS;
    QString strDot;
    QString strDotDot;
    QString strever;
    QString strforever;
    ProString strTEMPLATE;
    QHash<ProString, int> expands;
    QHash<ProString, int> functions;
    QHash<ProString, ProString> varMap;
    ProStringList fakeValue;
};

extern QMakeStatics statics;

}

QT_END_NAMESPACE

#endif // QMAKEEVALUATOR_P_H
