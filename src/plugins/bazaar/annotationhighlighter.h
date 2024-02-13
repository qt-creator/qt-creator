// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/baseannotationhighlighter.h>

namespace Bazaar::Internal {

class BazaarAnnotationHighlighter : public VcsBase::BaseAnnotationHighlighter
{
public:
    explicit BazaarAnnotationHighlighter(const VcsBase::Annotation &annotation);

private:
    QString changeNumber(const QString &block) const override;
    const QRegularExpression m_changeset;
};

} // Bazaar::Internal
