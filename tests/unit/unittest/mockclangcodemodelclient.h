/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "googletest.h"

#include <clangsupport/clangcodemodelclientinterface.h>
#include <clangsupport/clangcodemodelclientmessages.h>

class MockClangCodeModelClient : public ClangBackEnd::ClangCodeModelClientInterface
{
public:
    MOCK_METHOD0(alive,
                 void());
    MOCK_METHOD1(echo,
                 void(const ClangBackEnd::EchoMessage &message));
    MOCK_METHOD1(codeCompleted,
                 void(const ClangBackEnd::CodeCompletedMessage &message));
    MOCK_METHOD1(documentAnnotationsChanged,
                 void(const ClangBackEnd::DocumentAnnotationsChangedMessage &message));
    MOCK_METHOD1(references,
                 void(const ClangBackEnd::ReferencesMessage &message));
    MOCK_METHOD1(followSymbol,
                 void(const ClangBackEnd::FollowSymbolMessage &message));
};
