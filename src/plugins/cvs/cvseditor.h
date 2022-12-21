// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseeditor.h>

#include <QRegularExpression>

namespace Cvs::Internal {

class CvsEditorWidget : public VcsBase::VcsBaseEditorWidget
{
    Q_OBJECT

public:
    CvsEditorWidget();

private:
    QString changeUnderCursor(const QTextCursor &) const override;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(
            const QSet<QString> &changes) const override;
    QStringList annotationPreviousVersions(const QString &revision) const override;

    const QRegularExpression m_revisionAnnotationPattern;
    const QRegularExpression m_revisionLogPattern;
    QString m_diffBaseDir;
};

} // Cvs::Internal
