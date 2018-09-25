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

#include <clangcodemodelserverinterface.h>

class MockClangCodeModelServer : public ClangBackEnd::ClangCodeModelServerInterface {
public:
    MOCK_METHOD0(end,
                 void());

    MOCK_METHOD1(documentsOpened,
                 void(const ClangBackEnd::DocumentsOpenedMessage &message));
    MOCK_METHOD1(documentsChanged,
                 void(const ClangBackEnd::DocumentsChangedMessage &message));
    MOCK_METHOD1(documentsClosed,
                 void(const ClangBackEnd::DocumentsClosedMessage &message));
    MOCK_METHOD1(documentVisibilityChanged,
                 void(const ClangBackEnd::DocumentVisibilityChangedMessage &message));

    MOCK_METHOD1(unsavedFilesUpdated,
                 void(const ClangBackEnd::UnsavedFilesUpdatedMessage &message));
    MOCK_METHOD1(unsavedFilesRemoved,
                 void(const ClangBackEnd::UnsavedFilesRemovedMessage &message));

    MOCK_METHOD1(requestCompletions,
                 void(const ClangBackEnd::RequestCompletionsMessage &message));
    MOCK_METHOD1(requestAnnotations,
                 void(const ClangBackEnd::RequestAnnotationsMessage &message));
    MOCK_METHOD1(requestReferences,
                 void(const ClangBackEnd::RequestReferencesMessage &message));
    MOCK_METHOD1(requestFollowSymbol,
                 void(const ClangBackEnd::RequestFollowSymbolMessage &message));
    MOCK_METHOD1(requestToolTip,
                 void(const ClangBackEnd::RequestToolTipMessage &message));

};
