/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "tokeninfo.h"

namespace ClangBackEnd {

class FullTokenInfo : public TokenInfo
{
    template<class T> friend class TokenProcessor;
public:
    FullTokenInfo() = default;
    FullTokenInfo(const Cursor &cursor,
                  const Token *token,
                  std::vector<CXSourceRange> &m_currentOutputArgumentRanges);
    void evaluate() override;

    operator TokenInfoContainer() const override;
protected:
    void identifierKind(const Cursor &cursor, Recursion recursion) override;
    void referencedTypeKind(const Cursor &cursor) override;
    void functionKind(const Cursor &cursor, Recursion recursion) override;
    void variableKind(const Cursor &cursor) override;
    void fieldKind(const Cursor &cursor) override;
    void memberReferenceKind(const Cursor &cursor) override;
    void keywordKind() override;
    void overloadedOperatorKind() override;
private:
    void updateTypeSpelling(const Cursor &cursor, bool functionLike = false);
    void updatePropertyData();

    ExtraInfo m_extraInfo;
};

} // namespace ClangBackEnd
