/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MOCKIPCSERVER_H
#define MOCKIPCSERVER_H

#include <ipcserverinterface.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

class MockIpcServer : public ClangBackEnd::IpcServerInterface {
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
    MOCK_METHOD1(requestDiagnostics,
                 void(const ClangBackEnd::RequestDiagnosticsMessage &message));
    MOCK_METHOD1(requestHighlighting,
                 void(const ClangBackEnd::RequestHighlightingMessage &message));
    MOCK_METHOD1(updateVisibleTranslationUnits,
                 void(const ClangBackEnd::UpdateVisibleTranslationUnitsMessage &message));
};

#endif // MOCKIPCSERVER_H

