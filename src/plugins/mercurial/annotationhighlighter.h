// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/baseannotationhighlighter.h>

#include <QRegularExpression>

namespace Mercurial::Internal {

class MercurialAnnotationHighlighter : public VcsBase::BaseAnnotationHighlighter
{
public:
    explicit MercurialAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                            QTextDocument *document = nullptr);

private:
    QString changeNumber(const QString &block) const override;
    const QRegularExpression changeset;
};

} // Mercurial::Internal
