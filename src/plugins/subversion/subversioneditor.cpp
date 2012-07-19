/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "subversioneditor.h"
#include "subversionplugin.h"

#include "annotationhighlighter.h"
#include "subversionconstants.h"

#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>

#include <QDebug>
#include <QFileInfo>
#include <QTextCursor>
#include <QTextBlock>

using namespace Subversion;
using namespace Subversion::Internal;

SubversionEditor::SubversionEditor(const VcsBase::VcsBaseEditorParameters *type,
                                   QWidget *parent) :
    VcsBase::VcsBaseEditorWidget(type, parent),
    m_changeNumberPattern(QLatin1String("^\\d+$")),
    m_revisionNumberPattern(QLatin1String("^r\\d+$"))
{
    QTC_ASSERT(m_changeNumberPattern.isValid(), return);
    QTC_ASSERT(m_revisionNumberPattern.isValid(), return);
    setAnnotateRevisionTextFormat(tr("Annotate revision \"%1\""));
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

VcsBase::DiffHighlighter *SubversionEditor::createDiffHighlighter() const
{
    const QRegExp filePattern(QLatin1String("^[-+][-+][-+] .*|^Index: .*|^==*$"));
    QTC_CHECK(filePattern.isValid());
    return new VcsBase::DiffHighlighter(filePattern);
}

VcsBase::BaseAnnotationHighlighter *SubversionEditor::createAnnotationHighlighter(const QSet<QString> &changes,
                                                                                  const QColor &bg) const
{
    return new SubversionAnnotationHighlighter(changes, bg);
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
            const QString rc = findDiffFile(diffFileName);
            if (Subversion::Constants::debug)
                qDebug() << Q_FUNC_INFO << diffFileName << rc << source();
            return rc;
        }
    }
    return QString();
}

QStringList SubversionEditor::annotationPreviousVersions(const QString &v) const
{
    bool ok;
    const int revision = v.toInt(&ok);
    if (!ok || revision < 2)
        return QStringList();
    return QStringList(QString::number(revision - 1));
}
