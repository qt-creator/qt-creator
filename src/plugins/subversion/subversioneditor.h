// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseeditor.h>

#include <QRegularExpression>

namespace Subversion {
namespace Internal {

class SubversionEditorWidget : public VcsBase::VcsBaseEditorWidget
{
    Q_OBJECT

public:
    SubversionEditorWidget();

private:
    QString changeUnderCursor(const QTextCursor &) const override;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(
            const QSet<QString> &changes) const override;
    QStringList annotationPreviousVersions(const QString &) const override;

    QRegularExpression m_changeNumberPattern;
    QRegularExpression m_revisionNumberPattern;
};

} // namespace Internal
} // namespace Subversion
