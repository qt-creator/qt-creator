/**************************************************************************
**
** Copyright (C) 2015 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "clearcaseeditor.h"
#include "annotationhighlighter.h"
#include "clearcaseconstants.h"
#include "clearcaseplugin.h"

#include <utils/qtcassert.h>
#include <vcsbase/diffandloghighlighter.h>

#include <QDir>
#include <QFileInfo>
#include <QTextBlock>
#include <QTextCursor>

using namespace ClearCase;
using namespace ClearCase::Internal;

ClearCaseEditorWidget::ClearCaseEditorWidget() :
    m_versionNumberPattern(QLatin1String("[\\\\/]main[\\\\/][^ \t\n\"]*"))
{
    QTC_ASSERT(m_versionNumberPattern.isValid(), return);
    // Diff formats:
    // "+++ D:\depot\...\mainwindow.cpp@@\main\3" (versioned)
    // "+++ D:\depot\...\mainwindow.cpp[TAB]Sun May 01 14:22:37 2011" (local)
    QRegExp diffFilePattern(QLatin1String("^[-+]{3} ([^\\t]+)(?:@@|\\t)"));
    diffFilePattern.setMinimal(true);
    setDiffFilePattern(diffFilePattern);
    setLogEntryPattern(QRegExp(QLatin1String("version \"([^\"]+)\"")));
    setAnnotateRevisionTextFormat(tr("Annotate version \"%1\""));
}

QSet<QString> ClearCaseEditorWidget::annotationChanges() const
{
    QSet<QString> changes;
    QString txt = toPlainText();
    if (txt.isEmpty())
        return changes;
    // search until header
    int separator = txt.indexOf(QRegExp(QLatin1String("\n-{30}")));
    QRegExp r(QLatin1String("([^|]*)\\|[^\n]*\n"));
    QTC_ASSERT(r.isValid(), return changes);
    int pos = r.indexIn(txt, 0);
    while (pos != -1 && pos < separator) {
        changes.insert(r.cap(1));
        pos = r.indexIn(txt, pos + r.matchedLength());
    }
    return changes;
}

QString ClearCaseEditorWidget::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::BlockUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    QString change = cursor.selectedText();
    // Annotation output has number, log output has revision numbers
    // as r1, r2...
    if (m_versionNumberPattern.indexIn(change) != -1)
        return m_versionNumberPattern.cap();
    return QString();
}

VcsBase::BaseAnnotationHighlighter *ClearCaseEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new ClearCaseAnnotationHighlighter(changes);
}
