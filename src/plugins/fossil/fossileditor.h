// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseeditor.h>

namespace Fossil {
namespace Internal {

class FossilEditorWidgetPrivate;

class FossilEditorWidget final : public VcsBase::VcsBaseEditorWidget
{
    Q_OBJECT

public:
    FossilEditorWidget();
    ~FossilEditorWidget() final;

private:
    QString changeUnderCursor(const QTextCursor &cursor) const final;
    QString decorateVersion(const QString &revision) const final;
    QStringList annotationPreviousVersions(const QString &revision) const final;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(
            const QSet<QString> &changes) const final;

    FossilEditorWidgetPrivate *d;
};

} // namespace Internal
} // namespace Fossil
