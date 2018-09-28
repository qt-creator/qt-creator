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

    void documentsOpened(const ClangBackEnd::DocumentsOpenedMessage &message) override;
    void documentsChanged(const ClangBackEnd::DocumentsChangedMessage &message) override;
    void documentsClosed(const ClangBackEnd::DocumentsClosedMessage &message) override;
    void documentVisibilityChanged(const ClangBackEnd::DocumentVisibilityChangedMessage &message) override;

    void unsavedFilesUpdated(const ClangBackEnd::UnsavedFilesUpdatedMessage &message) override;
    void unsavedFilesRemoved(const ClangBackEnd::UnsavedFilesRemovedMessage &message) override;

    void requestCompletions(const ClangBackEnd::RequestCompletionsMessage &message) override;
    void requestAnnotations(const ClangBackEnd::RequestAnnotationsMessage &message) override;
    void requestReferences(const ClangBackEnd::RequestReferencesMessage &message) override;
    void requestToolTip(const ClangBackEnd::RequestToolTipMessage &message) override;
    void requestFollowSymbol(const ClangBackEnd::RequestFollowSymbolMessage &message) override;

private:
    ClangBackEnd::ClangCodeModelConnectionClient *m_connection = nullptr;
};

} // namespace Internal
} // namespace ClangCodeModel
