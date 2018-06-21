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

#include "completionsmessage.h"

#include <QDebug>

namespace ClangBackEnd {

#define RETURN_TEXT_FOR_CASE(enumValue) case CompletionCorrection::enumValue: return #enumValue
static const char *completionCorrectionToText(CompletionCorrection correction)
{
    switch (correction) {
        RETURN_TEXT_FOR_CASE(NoCorrection);
        RETURN_TEXT_FOR_CASE(DotToArrowCorrection);
        default: return "UnhandledCompletionCorrection";
    }
}
#undef RETURN_TEXT_FOR_CASE

QDebug operator<<(QDebug debug, const CompletionsMessage &message)
{
    debug.nospace() << "CompletionsMessage(";

    debug.nospace() << message.codeCompletions << ", "
                    << completionCorrectionToText(message.neededCorrection) << ", "
                    << message.ticketNumber;

    debug.nospace() << ")";

    return debug;
}

} // namespace ClangBackEnd

