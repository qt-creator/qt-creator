/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <clangsupport/clangcodemodelserverinterface.h>

namespace ClangBackEnd { class ClangCodeModelConnectionClient; }

namespace ClangCodeModel {
namespace Internal {

class BackendSender : public ClangBackEnd::ClangCodeModelServerInterface
{
public:
    BackendSender(ClangBackEnd::ClangCodeModelConnectionClient *connectionClient);

    void end() override;
    void registerTranslationUnitsForEditor(const ClangBackEnd::RegisterTranslationUnitForEditorMessage &message) override;
    void updateTranslationUnitsForEditor(const ClangBackEnd::UpdateTranslationUnitsForEditorMessage &message) override;
    void unregisterTranslationUnitsForEditor(const ClangBackEnd::UnregisterTranslationUnitsForEditorMessage &message) override;
    void registerProjectPartsForEditor(const ClangBackEnd::RegisterProjectPartsForEditorMessage &message) override;
    void unregisterProjectPartsForEditor(const ClangBackEnd::UnregisterProjectPartsForEditorMessage &message) override;
    void registerUnsavedFilesForEditor(const ClangBackEnd::RegisterUnsavedFilesForEditorMessage &message) override;
    void unregisterUnsavedFilesForEditor(const ClangBackEnd::UnregisterUnsavedFilesForEditorMessage &message) override;
    void completeCode(const ClangBackEnd::CompleteCodeMessage &message) override;
    void requestDocumentAnnotations(const ClangBackEnd::RequestDocumentAnnotationsMessage &message) override;
    void requestReferences(const ClangBackEnd::RequestReferencesMessage &message) override;
    void requestFollowSymbol(const ClangBackEnd::RequestFollowSymbolMessage &message) override;
    void updateVisibleTranslationUnits(const ClangBackEnd::UpdateVisibleTranslationUnitsMessage &message) override;

private:
    bool isConnected() const;

private:
    ClangBackEnd::ClangCodeModelConnectionClient *m_connection = nullptr;
};

} // namespace Internal
} // namespace ClangCodeModel
