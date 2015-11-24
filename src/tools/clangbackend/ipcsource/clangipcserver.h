/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef CLANGIPCSERVER_H
#define CLANGIPCSERVER_H

#include "ipcserverinterface.h"

#include "projectpart.h"
#include "projects.h"
#include "clangtranslationunit.h"
#include "translationunits.h"
#include "unsavedfiles.h"

#include <utf8string.h>

#include <QMap>
#include <QTimer>

namespace ClangBackEnd {

class ClangIpcServer : public IpcServerInterface
{
public:
    ClangIpcServer();

    void end() override;
    void registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message) override;
    void updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message) override;
    void unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message) override;
    void registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message) override;
    void unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message) override;
    void registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message) override;
    void unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message) override;
    void completeCode(const CompleteCodeMessage &message) override;
    void requestDiagnostics(const RequestDiagnosticsMessage &message) override;
    void updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message) override;

    const TranslationUnits &translationUnitsForTestOnly() const;

private:
    void startSendDiagnosticTimerIfFileIsNotATranslationUnit(const Utf8String &filePath);

private:
    ProjectParts projects;
    UnsavedFiles unsavedFiles;
    TranslationUnits translationUnits;
    QTimer sendDiagnosticsTimer;
};

} // namespace ClangBackEnd
#endif // CLANGIPCSERVER_H
