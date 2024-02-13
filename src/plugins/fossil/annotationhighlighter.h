// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/baseannotationhighlighter.h>

namespace Fossil {
namespace Internal {

class FossilAnnotationHighlighter : public VcsBase::BaseAnnotationHighlighter
{
public:
    explicit FossilAnnotationHighlighter(const VcsBase::Annotation &annotation);

private:
    QString changeNumber(const QString &block) const final;
    QRegularExpression m_changesetIdPattern;
};

} // namespace Internal
} // namespace Fossil
