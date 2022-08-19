// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "annotationhighlighter.h"
#include "constants.h"

#include <QRegularExpression>

using namespace Bazaar::Internal;
using namespace Bazaar;

BazaarAnnotationHighlighter::BazaarAnnotationHighlighter(const ChangeNumbers &changeNumbers,
                                                         QTextDocument *document)
    : VcsBase::BaseAnnotationHighlighter(changeNumbers, document),
      m_changeset(QLatin1String(Constants::ANNOTATE_CHANGESET_ID))
{ }

QString BazaarAnnotationHighlighter::changeNumber(const QString &block) const
{
    const QRegularExpressionMatch match = m_changeset.match(block);
    if (match.hasMatch())
        return match.captured(1);
    return QString();
}
