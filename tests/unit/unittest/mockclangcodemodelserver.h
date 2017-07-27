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

#include <clangcodemodelserverinterface.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

class MockClangCodeModelServer : public ClangBackEnd::ClangCodeModelServerInterface {
public:
    MOCK_METHOD0(end,
                 void());
    MOCK_METHOD1(registerTranslationUnitsForEditor,
                 void(const ClangBackEnd::RegisterTranslationUnitForEditorMessage &message));
    MOCK_METHOD1(updateTranslationUnitsForEditor,
                 void(const ClangBackEnd::UpdateTranslationUnitsForEditorMessage &message));
    MOCK_METHOD1(unregisterTranslationUnitsForEditor,
                 void(const ClangBackEnd::UnregisterTranslationUnitsForEditorMessage &message));
    MOCK_METHOD1(registerProjectPartsForEditor,
                 void(const ClangBackEnd::RegisterProjectPartsForEditorMessage &message));
    MOCK_METHOD1(unregisterProjectPartsForEditor,
                 void(const ClangBackEnd::UnregisterProjectPartsForEditorMessage &message));
    MOCK_METHOD1(registerUnsavedFilesForEditor,
                 void(const ClangBackEnd::RegisterUnsavedFilesForEditorMessage &message));
    MOCK_METHOD1(unregisterUnsavedFilesForEditor,
                 void(const ClangBackEnd::UnregisterUnsavedFilesForEditorMessage &message));
    MOCK_METHOD1(completeCode,
                 void(const ClangBackEnd::CompleteCodeMessage &message));
    MOCK_METHOD1(requestDocumentAnnotations,
                 void(const ClangBackEnd::RequestDocumentAnnotationsMessage &message));
    MOCK_METHOD1(requestReferences,
                 void(const ClangBackEnd::RequestReferencesMessage &message));
    MOCK_METHOD1(requestFollowSymbol,
                 void(const ClangBackEnd::RequestFollowSymbolMessage &message));
    MOCK_METHOD1(updateVisibleTranslationUnits,
                 void(const ClangBackEnd::UpdateVisibleTranslationUnitsMessage &message));
};
