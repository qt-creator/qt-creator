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

#include "subversioneditor.h"

#include "annotationhighlighter.h"
#include "subversionconstants.h"

#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>

#include <QtCore/QDebug>
#include <QtGui/QTextCursor>

using namespace Subversion;
using namespace Subversion::Internal;

SubversionEditor::SubversionEditor(const VCSBase::VCSBaseEditorParameters *type,
                                   QWidget *parent) :
    VCSBase::VCSBaseEditor(type, parent),
    m_changeNumberPattern(QLatin1String("^\\d+$")),
    m_revisionNumberPattern(QLatin1String("^r\\d+$"))
{
    QTC_ASSERT(m_changeNumberPattern.isValid(), return);
    QTC_ASSERT(m_revisionNumberPattern.isValid(), return);
}

QSet<QString> SubversionEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString txt = toPlainText();
    if (txt.isEmpty())
        return changes;
    // Hunt for first change number in annotation: "<change>:"
    QRegExp r(QLatin1String("^(\\d+):"));
    QTC_ASSERT(r.isValid(), return changes);
    if (r.indexIn(txt) != -1) {
        changes.insert(r.cap(1));
        r.setPattern(QLatin1String("\n(\\d+):"));
        QTC_ASSERT(r.isValid(), return changes);
        int pos = 0;
        while ((pos = r.indexIn(txt, pos)) != -1) {
            pos += r.matchedLength();
            changes.insert(r.cap(1));
        }
    }
    if (Subversion::Constants::debug)
        qDebug() << "SubversionEditor::annotationChanges() returns #" << changes.size();
    return changes;
}

QString SubversionEditor::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::WordUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    QString change = cursor.selectedText();
    // Annotation output has number, log output has revision numbers
    // as r1, r2...
    if (m_changeNumberPattern.exactMatch(change))
        return change;
    if (m_revisionNumberPattern.exactMatch(change)) {
        change.remove(0, 1);
        return change;
    }
    return QString();
}

/* code:
    Index: main.cpp
===================================================================
--- main.cpp    (revision 2)
+++ main.cpp    (working copy)
@@ -6,6 +6,5 @@
\endcode
*/

VCSBase::DiffHighlighter *SubversionEditor::createDiffHighlighter() const
{
    const QRegExp filePattern(QLatin1String("^[-+][-+][-+] .*|^Index: .*|^==*$"));
    QTC_ASSERT(filePattern.isValid(), /**/);
    return new VCSBase::DiffHighlighter(filePattern);
}

VCSBase::BaseAnnotationHighlighter *SubversionEditor::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new SubversionAnnotationHighlighter(changes);
}

QString SubversionEditor::fileNameFromDiffSpecification(const QTextBlock &inBlock) const
{
    // "+++ /depot/.../mainwindow.cpp<tab>(revision 3)"
    // Go back chunks
    const QString diffIndicator = QLatin1String("+++ ");
    for (QTextBlock  block = inBlock; block.isValid() ; block = block.previous()) {
        QString diffFileName = block.text();
        if (diffFileName.startsWith(diffIndicator)) {
            diffFileName.remove(0, diffIndicator.size());
            const int tabIndex = diffFileName.lastIndexOf(QLatin1Char('\t'));
            if (tabIndex != -1)
                diffFileName.truncate(tabIndex);
            if (Subversion::Constants::debug)
                qDebug() << Q_FUNC_INFO << diffFileName;
            return diffFileName;
        }
    }
    return QString();
}
