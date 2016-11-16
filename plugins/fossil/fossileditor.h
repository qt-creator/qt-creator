/****************************************************************************
**
** Copyright (c) 2016 Artur Shepilko
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <vcsbase/vcsbaseeditor.h>

#include <QRegularExpression>

namespace Fossil {
namespace Internal {

class FossilEditorWidget : public VcsBase::VcsBaseEditorWidget
{
    Q_OBJECT

public:
    FossilEditorWidget();

private:
    QSet<QString> annotationChanges() const final;
    QString changeUnderCursor(const QTextCursor &cursor) const final;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes) const final;

    const QRegularExpression m_exactChangesetId;
    const QRegularExpression m_firstChangesetId;
    const QRegularExpression m_nextChangesetId;
};

} // namespace Internal
} // namespace Fossil
