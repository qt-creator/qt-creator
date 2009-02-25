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

#ifndef ANNOTATIONHIGHLIGHTER_H
#define ANNOTATIONHIGHLIGHTER_H

#include <vcsbase/baseannotationhighlighter.h>

namespace Git {
namespace Internal {

// Annotation highlighter for p4 triggering on 'changenumber:'
class GitAnnotationHighlighter : public VCSBase::BaseAnnotationHighlighter
{
    Q_OBJECT
public:
    explicit GitAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                      QTextDocument *document = 0);

private:
    virtual QString changeNumber(const QString &block) const;

    const QChar m_blank;
};

} // namespace Internal
} // namespace Git

#endif // ANNOTATIONHIGHLIGHTER_H
