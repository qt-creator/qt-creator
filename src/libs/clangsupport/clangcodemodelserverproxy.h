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
#include "clangcodemodelserverinterface.h"
#include "readmessageblock.h"
#include "writemessageblock.h"

#include <QtGlobal>
#include <QTimer>

#include <memory>

QT_BEGIN_NAMESPACE
class QVariant;
class QProcess;
class QLocalServer;
class QLocalSocket;
QT_END_NAMESPACE

namespace ClangBackEnd {

class CLANGSUPPORT_EXPORT ClangCodeModelServerProxy : public ClangCodeModelServerInterface
{
public:
    ClangCodeModelServerProxy(ClangCodeModelClientInterface *client, QIODevice *ioDevice);
    ClangCodeModelServerProxy(const ClangCodeModelServerProxy&) = delete;
    ClangCodeModelServerProxy &operator=(const ClangCodeModelServerProxy&) = delete;

    void end() override;
    void registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message) override;
    void updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message) override;
    void unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message) override;
    void registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message) override;
    void unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message) override;
    void registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message) override;
    void unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message) override;
    void completeCode(const CompleteCodeMessage &message) override;
    void requestDocumentAnnotations(const RequestDocumentAnnotationsMessage &message) override;
    void requestReferences(const RequestReferencesMessage &message) override;
    void requestFollowSymbol(const RequestFollowSymbolMessage &message) override;
    void updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message) override;

    void readMessages();

    void resetCounter();

private:
    ClangBackEnd::WriteMessageBlock m_writeMessageBlock;
    ClangBackEnd::ReadMessageBlock m_readMessageBlock;
    ClangCodeModelClientInterface *m_client;
};

} // namespace ClangBackEnd
