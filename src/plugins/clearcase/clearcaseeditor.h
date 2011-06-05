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

#ifndef CLEARCASEEDITOR_H
#define CLEARCASEEDITOR_H

#include <vcsbase/vcsbaseeditor.h>

#include <QRegExp>

namespace ClearCase {
namespace Internal {

class ClearCaseEditor : public VcsBase::VcsBaseEditorWidget
{
    Q_OBJECT

public:
    explicit ClearCaseEditor(const VcsBase::VcsBaseEditorParameters *type,
                            QWidget *parent);

private:
    QSet<QString> annotationChanges() const;
    QString changeUnderCursor(const QTextCursor &) const;
    VcsBase::DiffHighlighter *createDiffHighlighter() const;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes, const QColor &bg) const;
    QString fileNameFromDiffSpecification(const QTextBlock &diffFileName) const;

    mutable QRegExp m_versionNumberPattern;
};

} // namespace Internal
} // namespace ClearCase

#endif // CLEARCASEEDITOR_H
