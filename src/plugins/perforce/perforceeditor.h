// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseeditor.h>

#include <QRegularExpression>

namespace Perforce::Internal {

class PerforceEditorWidget : public VcsBase::VcsBaseEditorWidget
{
    Q_OBJECT

public:
    PerforceEditorWidget();

private:
    QString changeUnderCursor(const QTextCursor &) const override;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(
            const QSet<QString> &changes) const override;
    QString findDiffFile(const QString &f) const override;
    QStringList annotationPreviousVersions(const QString &v) const override;

    const QRegularExpression m_changeNumberPattern;
};

} // Perforce::Internal
