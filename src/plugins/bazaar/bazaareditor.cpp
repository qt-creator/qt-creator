/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <vcsbase/diffhighlighter.h>

#include <QtCore/QString>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace Bazaar::Internal;
using namespace Bazaar;

BazaarEditor::BazaarEditor(const VCSBase::VCSBaseEditorParameters *type, QWidget *parent)
        : VCSBase::VCSBaseEditorWidget(type, parent),
        m_exactIdentifier12(QLatin1String(Constants::CHANGEIDEXACT12)),
        m_exactIdentifier40(QLatin1String(Constants::CHANGEIDEXACT40)),
        m_changesetIdentifier12(QLatin1String(Constants::CHANGESETID12)),
        m_changesetIdentifier40(QLatin1String(Constants::CHANGESETID40)),
        m_diffIdentifier(QLatin1String(Constants::DIFFIDENTIFIER))
{
    setAnnotateRevisionTextFormat(tr("Annotate %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Annotate parent revision %1"));
}

QSet<QString> BazaarEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString data = toPlainText();
    if (data.isEmpty())
        return changes;

    int position = 0;
    while ((position = m_changesetIdentifier12.indexIn(data, position)) != -1) {
        changes.insert(m_changesetIdentifier12.cap(1));
        position += m_changesetIdentifier12.matchedLength();
    }

    return changes;
}

QString BazaarEditor::changeUnderCursor(const QTextCursor &cursorIn) const
{
    QTextCursor cursor = cursorIn;
    cursor.select(QTextCursor::WordUnderCursor);
    if (cursor.hasSelection()) {
        const QString change = cursor.selectedText();
        if (m_exactIdentifier12.exactMatch(change))
            return change;
        if (m_exactIdentifier40.exactMatch(change))
            return change;
    }
    return QString();
}

VCSBase::DiffHighlighter *BazaarEditor::createDiffHighlighter() const
{
    return new VCSBase::DiffHighlighter(m_diffIdentifier);
}

VCSBase::BaseAnnotationHighlighter *BazaarEditor::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new BazaarAnnotationHighlighter(changes);
}

QString BazaarEditor::fileNameFromDiffSpecification(const QTextBlock &diffFileSpec) const
{
   const QString filechangeId(QLatin1String("+++ b/"));
    QTextBlock::iterator iterator;
    for (iterator = diffFileSpec.begin(); !(iterator.atEnd()); iterator++) {
        QTextFragment fragment = iterator.fragment();
        if(fragment.isValid()) {
            if (fragment.text().startsWith(filechangeId)) {
                const QString filename = fragment.text().remove(0, filechangeId.size());
                return findDiffFile(filename, BazaarPlugin::instance()->versionControl());
            }
        }
    }
    return QString();
}
