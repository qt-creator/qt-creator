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

#ifndef ANNOTATIONHIGHLIGHTER_H
#define ANNOTATIONHIGHLIGHTER_H

#include <vcsbase/baseannotationhighlighter.h>

namespace ClearCase {
namespace Internal {

// Annotation highlighter for clearcase triggering on 'changenumber '
class ClearCaseAnnotationHighlighter : public VcsBase::BaseAnnotationHighlighter
{
    Q_OBJECT
public:
    explicit ClearCaseAnnotationHighlighter(const ChangeNumbers &changeNumbers, const QColor &bg,
                                            QTextDocument *document = 0);

private:
    virtual QString changeNumber(const QString &block) const;

    const QChar m_separator;
};

} // namespace Internal
} // namespace ClearCase

#endif // ANNOTATIONHIGHLIGHTER_H
