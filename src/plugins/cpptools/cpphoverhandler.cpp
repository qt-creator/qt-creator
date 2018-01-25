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

#include <coreplugin/helpmanager.h>
#include <texteditor/texteditor.h>

#include <utils/optional.h>
#include <utils/textutils.h>
#include <utils/executeondestruction.h>

#include <QTextCursor>
#include <QUrl>

using namespace Core;
using namespace TextEditor;

namespace CppTools {

QString CppHoverHandler::tooltipTextForHelpItem(const HelpItem &helpItem)
{
    // If Qt is built with a namespace, we still show the tip without it, as
    // it is in the docs and for consistency with the doc extraction mechanism.
    const HelpItem::Category category = helpItem.category();
    const QString &contents = helpItem.extractContent(false);
    if (!contents.isEmpty()) {
        if (category == HelpItem::ClassOrNamespace)
            return helpItem.helpId() + contents;
        else
            return contents;
    } else if (category == HelpItem::Typedef ||
               category == HelpItem::Enum ||
               category == HelpItem::ClassOrNamespace) {
        // This approach is a bit limited since it cannot be used for functions
        // because the help id doesn't really help in that case.
        QString prefix;
        if (category == HelpItem::Typedef)
            prefix = QLatin1String("typedef ");
        else if (category == HelpItem::Enum)
            prefix = QLatin1String("enum ");
        return prefix + helpItem.helpId();
    }

    return QString();
}

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
    if (evaluator.identifiedCppElement()) {
        const QSharedPointer<CppElement> &cppElement = evaluator.cppElement();
        QStringList candidates = cppElement->helpIdCandidates;
        candidates.removeDuplicates();
        Utils::optional<HelpItem> helpItem;
        foreach (const QString &helpId, candidates) {
            if (helpId.isEmpty())
                continue;
            const QMap<QString, QUrl> helpLinks = HelpManager::linksForIdentifier(helpId);
            if (!helpLinks.isEmpty()) {
                helpItem.emplace(helpId, cppElement->helpMark, cppElement->helpCategory, helpLinks);
                break;
            }
        }
        if (helpItem)
            setLastHelpItemIdentified(helpItem.value()); // tool tip appended by decorateToolTip
        else
            tip += cppElement->tooltip;
    }
    setToolTip(tip);
}

} // namespace CppTools
