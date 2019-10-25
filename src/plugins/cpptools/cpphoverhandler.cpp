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

#include "cpphoverhandler.h"

#include "cppelementevaluator.h"
#include "cpptoolsreuse.h"

#include <coreplugin/helpmanager.h>
#include <texteditor/texteditor.h>

#include <utils/textutils.h>
#include <utils/executeondestruction.h>

#include <QTextCursor>
#include <QUrl>

using namespace Core;
using namespace TextEditor;

namespace CppTools {

void CppHoverHandler::identifyMatch(TextEditorWidget *editorWidget, int pos, ReportPriority report)
{
    Utils::ExecuteOnDestruction reportPriority([this, report](){ report(priority()); });

    QTextCursor tc(editorWidget->document());
    tc.setPosition(pos);

    CppElementEvaluator evaluator(editorWidget);
    evaluator.setTextCursor(tc);
    evaluator.execute();
    QString tip;
    if (evaluator.hasDiagnosis()) {
        tip += evaluator.diagnosis();
        setPriority(Priority_Diagnostic);
    }
    const QStringList fallback = identifierWordsUnderCursor(tc);
    if (evaluator.identifiedCppElement()) {
        const QSharedPointer<CppElement> &cppElement = evaluator.cppElement();
        const QStringList candidates = cppElement->helpIdCandidates;
        const HelpItem helpItem(candidates + fallback, cppElement->helpMark, cppElement->helpCategory);
        setLastHelpItemIdentified(helpItem);
        if (!helpItem.isValid())
            tip += cppElement->tooltip;
    } else {
        setLastHelpItemIdentified({fallback, {}, HelpItem::Unknown});
    }
    setToolTip(tip);
}

} // namespace CppTools
