/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
    QSet<QString> annotationChanges() const;
    QString changeUnderCursor(const QTextCursor &cursor) const;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes, const QColor &bg) const;
    QString decorateVersion(const QString &revision) const;
    QStringList annotationPreviousVersions(const QString &revision) const;

    mutable QRegExp exactIdentifier12;
    mutable QRegExp exactIdentifier40;
    mutable QRegExp changesetIdentifier12;
    const QRegExp changesetIdentifier40;
};

} // namespace Internal
} // namespace Mercurial

#endif // MERCURIALEDITOR_H
