/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 AudioCodes Ltd.
**
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "annotationhighlighter.h"
#include "clearcaseconstants.h"
#include "clearcaseeditor.h"
#include "clearcaseplugin.h"

#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTextBlock>
#include <QTextCursor>

using namespace ClearCase;
using namespace ClearCase::Internal;

ClearCaseEditor::ClearCaseEditor(const VcsBase::VcsBaseEditorParameters *type,
                                   QWidget *parent) :
    VcsBase::VcsBaseEditorWidget(type, parent),
    m_versionNumberPattern(QLatin1String("[\\\\/]main[\\\\/][^ \t\n\"]*"))
{
    QTC_ASSERT(m_versionNumberPattern.isValid(), return);
    setAnnotateRevisionTextFormat(tr("Annotate version \"%1\""));
}

QSet<QString> ClearCaseEditor::annotationChanges() const
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

QString ClearCaseEditor::changeUnderCursor(const QTextCursor &c) const
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

/*
 Diff header format (on Windows, native separators are used after the @@)
--- main.cpp@@\main\2
+++ main.cpp@@\main\1
@@ -6,6 +6,5 @@
*/
VcsBase::DiffHighlighter *ClearCaseEditor::createDiffHighlighter() const
{
    const QRegExp filePattern(QLatin1String("^[-+][-+][-+] "));
    QTC_CHECK(filePattern.isValid());
    return new VcsBase::DiffHighlighter(filePattern);
}

VcsBase::BaseAnnotationHighlighter *ClearCaseEditor::createAnnotationHighlighter(const QSet<QString> &changes,
                                                                                 const QColor &bg) const
{
    return new ClearCaseAnnotationHighlighter(changes, bg);
}

QString ClearCaseEditor::fileNameFromDiffSpecification(const QTextBlock &inBlock) const
{
    // "+++ D:\depot\...\mainwindow.cpp@@\main\3"
    // "+++ D:\depot\...\mainwindow.cpp[TAB]Sun May 01 14:22:37 2011"
    // Go back chunks
    const QString diffIndicator = QLatin1String("+++ ");
    for (QTextBlock  block = inBlock; block.isValid() ; block = block.previous()) {
        QString diffFileName = block.text();
        if (diffFileName.startsWith(diffIndicator)) {
            diffFileName.remove(0, diffIndicator.size());
            const int tabIndex = diffFileName.indexOf(QRegExp(QLatin1String("@@|\t")));
            if (tabIndex != -1)
                diffFileName.truncate(tabIndex);
            const QString rc = findDiffFile(diffFileName);
            if (ClearCase::Constants::debug)
                qDebug() << Q_FUNC_INFO << diffFileName << rc << source();
            return rc;
        }
    }
    return QString();
}
