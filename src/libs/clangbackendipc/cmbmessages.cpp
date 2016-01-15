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

#include "cmbmessages.h"

#include "cmbalivemessage.h"
#include "cmbendmessage.h"
#include "cmbechomessage.h"
#include "cmbregistertranslationunitsforeditormessage.h"
#include "cmbunregistertranslationunitsforeditormessage.h"
#include "cmbregisterprojectsforeditormessage.h"
#include "cmbunregisterprojectsforeditormessage.h"
#include "cmbcompletecodemessage.h"
#include "cmbcodecompletedmessage.h"
#include "diagnosticcontainer.h"
#include "diagnosticschangedmessage.h"
#include "registerunsavedfilesforeditormessage.h"
#include "requestdiagnosticsmessage.h"
#include "requesthighlightingmessage.h"
#include "highlightingchangedmessage.h"
#include "highlightingmarkcontainer.h"
#include "projectpartsdonotexistmessage.h"
#include "sourcelocationcontainer.h"
#include "sourcerangecontainer.h"
#include "translationunitdoesnotexistmessage.h"
#include "unregisterunsavedfilesforeditormessage.h"
#include "updatetranslationunitsforeditormessage.h"
#include "updatevisibletranslationunitsmessage.h"

#include <QDataStream>

template <typename T>
static void registerMetaType()
{
    qRegisterMetaType<T>();
    qRegisterMetaTypeStreamOperators<T>();
    QMetaType::registerComparators<T>();
}

namespace ClangBackEnd {

void Messages::registerMessages()
{
    // Messages
    registerMetaType<AliveMessage>();
    registerMetaType<EchoMessage>();
    registerMetaType<EndMessage>();

    registerMetaType<RegisterTranslationUnitForEditorMessage>();
    registerMetaType<UpdateTranslationUnitsForEditorMessage>();
    registerMetaType<UnregisterTranslationUnitsForEditorMessage>();

    registerMetaType<RegisterUnsavedFilesForEditorMessage>();
    registerMetaType<UnregisterUnsavedFilesForEditorMessage>();

    registerMetaType<RegisterProjectPartsForEditorMessage>();
    registerMetaType<UnregisterProjectPartsForEditorMessage>();

    registerMetaType<RequestDiagnosticsMessage>();
    registerMetaType<DiagnosticsChangedMessage>();

    registerMetaType<RequestHighlightingMessage>();
    registerMetaType<HighlightingChangedMessage>();

    registerMetaType<UpdateVisibleTranslationUnitsMessage>();

    registerMetaType<CompleteCodeMessage>();
    registerMetaType<CodeCompletedMessage>();
    registerMetaType<CodeCompletion>();

    registerMetaType<TranslationUnitDoesNotExistMessage>();
    registerMetaType<ProjectPartsDoNotExistMessage>();

    // Containers
    registerMetaType<DiagnosticContainer>();
    registerMetaType<HighlightingMarkContainer>();
    registerMetaType<FileContainer>();
    registerMetaType<ProjectPartContainer>();
    registerMetaType<SourceLocationContainer>();
    registerMetaType<SourceRangeContainer>();
}

} // namespace ClangBackEnd

