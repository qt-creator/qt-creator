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

#include "indenter.h"

using namespace SharedTools::IndenterInternal;

// --- Constants
Constants::Constants() :
    m_slashAster(QLatin1String("/*")),
    m_asterSlash(QLatin1String("*/")),
    m_slashSlash(QLatin1String("//")),
    m_else(QLatin1String("else")),
    m_qobject(QLatin1String("Q_OBJECT")),
    m_operators(QLatin1String("!=<>")),
    m_bracesSemicolon(QLatin1String("{};")),
    m_3dots(QLatin1String("...")),

    m_literal(QLatin1String("([\"'])(?:\\\\.|[^\\\\])*\\1")),
    m_label(QLatin1String("^\\s*((?:case\\b([^:]|::)+|[a-zA-Z_0-9]+)(?:\\s+slots|\\s+Q_SLOTS)?:)(?!:)")),
    m_inlineCComment(QLatin1String("/\\*.*\\*/")),
    m_braceX(QLatin1String("^\\s*\\}\\s*(?:else|catch)\\b")),
    m_iflikeKeyword(QLatin1String("\\b(?:catch|do|for|if|while|foreach)\\b")),
    m_caseLabel(QLatin1String("\\s*(?:case\\b(?:[^:]|::)+"
                "|(?:public|protected|private|signals|Q_SIGNALS|default)(?:\\s+slots|\\s+Q_SLOTS)?\\s*"
                ")?:.*"))
{
    m_literal.setMinimal(true);
    m_inlineCComment.setMinimal(true);
    Q_ASSERT(m_literal.isValid());
    Q_ASSERT(m_label.isValid());
    Q_ASSERT(m_inlineCComment.isValid());
    Q_ASSERT(m_braceX.isValid());
    Q_ASSERT(m_iflikeKeyword.isValid());
    Q_ASSERT(m_caseLabel.isValid());
}
