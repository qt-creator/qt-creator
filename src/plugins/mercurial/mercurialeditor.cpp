/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion
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

#include "mercurialeditor.h"
#include "annotationhighlighter.h"
#include "constants.h"
#include "mercurialplugin.h"
#include "mercurialclient.h"

#include <coreplugin/editormanager/editormanager.h>
#include <vcsbase/diffandloghighlighter.h>

#include <QString>
#include <QTextCursor>
#include <QTextBlock>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace Mercurial {
namespace Internal  {

MercurialEditorWidget::MercurialEditorWidget() :
        exactIdentifier12(QLatin1String(Constants::CHANGEIDEXACT12)),
        exactIdentifier40(QLatin1String(Constants::CHANGEIDEXACT40)),
        changesetIdentifier12(QLatin1String(Constants::CHANGESETID12)),
        changesetIdentifier40(QLatin1String(Constants::CHANGESETID40))
{
    setDiffFilePattern(QRegExp(QLatin1String(Constants::DIFFIDENTIFIER)));
    setLogEntryPattern(QRegExp(QLatin1String("^changeset:\\s+(\\S+)$")));
    setAnnotateRevisionTextFormat(tr("&Annotate %1"));
    setAnnotatePreviousRevisionTextFormat(tr("Annotate &parent revision %1"));
}

QSet<QString> MercurialEditorWidget::annotationChanges() const
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

QString MercurialEditorWidget::changeUnderCursor(const QTextCursor &cursorIn) const
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

VcsBase::BaseAnnotationHighlighter *MercurialEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new MercurialAnnotationHighlighter(changes);
}

QString MercurialEditorWidget::decorateVersion(const QString &revision) const
{
    const QFileInfo fi(source());
    const QString workingDirectory = fi.absolutePath();
    // Format with short summary
    return MercurialPlugin::client()->shortDescriptionSync(workingDirectory, revision);
}

QStringList MercurialEditorWidget::annotationPreviousVersions(const QString &revision) const
{
    const QFileInfo fi(source());
    const QString workingDirectory = fi.absolutePath();
    // Retrieve parent revisions
    return MercurialPlugin::client()->parentRevisionsSync(workingDirectory, fi.fileName(), revision);
}

} // namespace Internal
} // namespace Mercurial
