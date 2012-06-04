/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef MERCURIALEDITOR_H
#define MERCURIALEDITOR_H

#include <vcsbase/vcsbaseeditor.h>

#include <QRegExp>

namespace Mercurial {
namespace Internal {

class MercurialEditor : public VcsBase::VcsBaseEditorWidget
{
    Q_OBJECT
public:
    explicit MercurialEditor(const VcsBase::VcsBaseEditorParameters *type, QWidget *parent);

private:
    virtual QSet<QString> annotationChanges() const;
    virtual QString changeUnderCursor(const QTextCursor &cursor) const;
    virtual VcsBase::DiffHighlighter *createDiffHighlighter() const;
    virtual VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes, const QColor &bg) const;
    virtual QString fileNameFromDiffSpecification(const QTextBlock &diffFileSpec) const;
    virtual QString decorateVersion(const QString &revision) const;
    virtual QStringList annotationPreviousVersions(const QString &revision) const;

    mutable QRegExp exactIdentifier12;
    mutable QRegExp exactIdentifier40;
    mutable QRegExp changesetIdentifier12;
    const QRegExp changesetIdentifier40;
    const QRegExp diffIdentifier;
};

} // namespace Internal
} // namespace Mercurial

#endif // MERCURIALEDITOR_H
