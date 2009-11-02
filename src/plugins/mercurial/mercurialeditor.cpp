/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "mercurialeditor.h"
#include "annotationhighlighter.h"
#include "constants.h"
#include "mercurialclient.h"

#include <coreplugin/editormanager/editormanager.h>
#include <vcsbase/diffhighlighter.h>

#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtCore/QDir>
#include <QtCore/QDebug>

using namespace Mercurial::Internal;
using namespace Mercurial;

MercurialEditor::MercurialEditor(const VCSBase::VCSBaseEditorParameters *type, QWidget *parent)
        : VCSBase::VCSBaseEditor(type, parent),
        exactIdentifier12(Constants::CHANGEIDEXACT12),
        exactIdentifier40(Constants::CHANGEIDEXACT40),
        changesetIdentifier12(Constants::CHANGESETID12),
        changesetIdentifier40(Constants::CHANGESETID40),
        diffIdentifier(Constants::DIFFIDENTIFIER)
{
}

QSet<QString> MercurialEditor::annotationChanges() const
{
    QSet<QString> changes;
    const QString data = toPlainText();
    if (data.isEmpty())
        return changes;

    int position = 0;
    while ((position = changesetIdentifier12.indexIn(data, position)) != -1) {
        changes.insert(changesetIdentifier12.cap(1));
        position += changesetIdentifier12.matchedLength();
    }

    return changes;
}

QString MercurialEditor::changeUnderCursor(const QTextCursor &cursorIn) const
{
    QTextCursor cursor = cursorIn;
    cursor.select(QTextCursor::WordUnderCursor);
    if (cursor.hasSelection()) {
        const QString change = cursor.selectedText();
        if (exactIdentifier12.exactMatch(change))
            return change;
        if (exactIdentifier40.exactMatch(change))
            return change;
    }
    return QString();
}

VCSBase::DiffHighlighter *MercurialEditor::createDiffHighlighter() const
{
    return new VCSBase::DiffHighlighter(diffIdentifier);
}

VCSBase::BaseAnnotationHighlighter *MercurialEditor::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new MercurialAnnotationHighlighter(changes);
}

QString MercurialEditor::fileNameFromDiffSpecification(const QTextBlock &diffFileSpec) const
{
    QString filechangeId("+++ b/");
    QTextBlock::iterator iterator;
    for (iterator = diffFileSpec.begin(); !(iterator.atEnd()); iterator++) {
        QTextFragment fragment = iterator.fragment();
        if(fragment.isValid()) {
            if (fragment.text().startsWith(filechangeId)) {
                QFileInfo sourceFile(source());
                QDir repository(MercurialClient::findTopLevelForFile(sourceFile));
                QString filename = fragment.text().remove(0, filechangeId.size());
                return repository.absoluteFilePath(filename);
            }
        }
    }
    return QString();
}
