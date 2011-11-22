/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
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

#include "bazaareditor.h"
#include "annotationhighlighter.h"
#include "constants.h"
#include "bazaarplugin.h"
#include "bazaarclient.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>
#include <vcsbase/diffhighlighter.h>

#include <QtCore/QRegExp>
#include <QtCore/QString>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#define BZR_CHANGE_PATTERN "[0-9]+"

using namespace Bazaar::Internal;
using namespace Bazaar;

BazaarEditor::BazaarEditor(const VCSBase::VCSBaseEditorParameters *type, QWidget *parent)
    : VCSBase::VCSBaseEditorWidget(type, parent),
      m_exactChangesetId(QLatin1String(Constants::CHANGESET_ID_EXACT)),
      m_diffFileId(QLatin1String("^=== [a-z]+ [a-z]+ '(.*)'\\s*"))
{
    setAnnotateRevisionTextFormat(tr("Annotate %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Annotate parent revision %1"));
}

QSet<QString> BazaarEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString txt = toPlainText();
    if (txt.isEmpty())
        return changes;

    QRegExp changeNumRx(QLatin1String("^(" BZR_CHANGE_PATTERN ") "));
    QTC_ASSERT(changeNumRx.isValid(), return changes);
    if (changeNumRx.indexIn(txt) != -1) {
        changes.insert(changeNumRx.cap(1));
        changeNumRx.setPattern(QLatin1String("\n(" BZR_CHANGE_PATTERN ") "));
        QTC_ASSERT(changeNumRx.isValid(), return changes);
        int pos = 0;
        while ((pos = changeNumRx.indexIn(txt, pos)) != -1) {
            pos += changeNumRx.matchedLength();
            changes.insert(changeNumRx.cap(1));
        }
    }
    return changes;
}

QString BazaarEditor::changeUnderCursor(const QTextCursor &cursorIn) const
{
    QTextCursor cursor = cursorIn;
    cursor.select(QTextCursor::WordUnderCursor);
    if (cursor.hasSelection()) {
        const QString change = cursor.selectedText();
        if (m_exactChangesetId.exactMatch(change))
            return change;
    }
    return QString();
}

VCSBase::DiffHighlighter *BazaarEditor::createDiffHighlighter() const
{
    return new VCSBase::DiffHighlighter(m_diffFileId);
}

VCSBase::BaseAnnotationHighlighter *BazaarEditor::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new BazaarAnnotationHighlighter(changes);
}

QString BazaarEditor::fileNameFromDiffSpecification(const QTextBlock &inBlock) const
{
    // Check for:
    // === <change> <file|dir> 'mainwindow.cpp'
    for (QTextBlock  block = inBlock; block.isValid(); block = block.previous()) {
        const QString line = block.text();
        if (m_diffFileId.indexIn(line) != -1)
            return findDiffFile(m_diffFileId.cap(1), BazaarPlugin::instance()->versionControl());
    }
    return QString();
}
