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
#include "requestdiagnosticsmessage.h"
#include "projectpartsdonotexistmessage.h"
#include "sourcelocationcontainer.h"
#include "sourcerangecontainer.h"
#include "translationunitdoesnotexistmessage.h"

#include <QDataStream>

namespace ClangBackEnd {

void Messages::registerMessages()
{
    qRegisterMetaType<EndMessage>();
    qRegisterMetaTypeStreamOperators<EndMessage>();
    QMetaType::registerComparators<EndMessage>();

    qRegisterMetaType<AliveMessage>();
    qRegisterMetaTypeStreamOperators<AliveMessage>();
    QMetaType::registerComparators<AliveMessage>();

    qRegisterMetaType<EchoMessage>();
    qRegisterMetaTypeStreamOperators<EchoMessage>();

    qRegisterMetaType<RegisterTranslationUnitForEditorMessage>();
    qRegisterMetaTypeStreamOperators<RegisterTranslationUnitForEditorMessage>();
    QMetaType::registerComparators<RegisterTranslationUnitForEditorMessage>();

    qRegisterMetaType<FileContainer>();
    qRegisterMetaTypeStreamOperators<FileContainer>();
    QMetaType::registerComparators<FileContainer>();

    qRegisterMetaType<UnregisterTranslationUnitsForEditorMessage>();
    qRegisterMetaTypeStreamOperators<UnregisterTranslationUnitsForEditorMessage>();
    QMetaType::registerComparators<UnregisterTranslationUnitsForEditorMessage>();

    qRegisterMetaType<CompleteCodeMessage>();
    qRegisterMetaTypeStreamOperators<CompleteCodeMessage>();
    QMetaType::registerComparators<CompleteCodeMessage>();

    qRegisterMetaType<CodeCompletion>();
    qRegisterMetaTypeStreamOperators<CodeCompletion>();
    QMetaType::registerComparators<CodeCompletion>();

    qRegisterMetaType<CodeCompletedMessage>();
    qRegisterMetaTypeStreamOperators<CodeCompletedMessage>();
    QMetaType::registerComparators<CodeCompletedMessage>();

    qRegisterMetaType<RegisterProjectPartsForEditorMessage>();
    qRegisterMetaTypeStreamOperators<RegisterProjectPartsForEditorMessage>();
    QMetaType::registerComparators<RegisterProjectPartsForEditorMessage>();

    qRegisterMetaType<ProjectPartContainer>();
    qRegisterMetaTypeStreamOperators<ProjectPartContainer>();
    QMetaType::registerComparators<ProjectPartContainer>();

    qRegisterMetaType<UnregisterProjectPartsForEditorMessage>();
    qRegisterMetaTypeStreamOperators<UnregisterProjectPartsForEditorMessage>();
    QMetaType::registerComparators<UnregisterProjectPartsForEditorMessage>();

    qRegisterMetaType<TranslationUnitDoesNotExistMessage>();
    qRegisterMetaTypeStreamOperators<TranslationUnitDoesNotExistMessage>();
    QMetaType::registerComparators<TranslationUnitDoesNotExistMessage>();

    qRegisterMetaType<ProjectPartsDoNotExistMessage>();
    qRegisterMetaTypeStreamOperators<ProjectPartsDoNotExistMessage>();
    QMetaType::registerComparators<ProjectPartsDoNotExistMessage>();

    qRegisterMetaType<DiagnosticsChangedMessage>();
    qRegisterMetaTypeStreamOperators<DiagnosticsChangedMessage>();
    QMetaType::registerComparators<DiagnosticsChangedMessage>();

    qRegisterMetaType<DiagnosticContainer>();
    qRegisterMetaTypeStreamOperators<DiagnosticContainer>();
    QMetaType::registerComparators<DiagnosticContainer>();

    qRegisterMetaType<SourceLocationContainer>();
    qRegisterMetaTypeStreamOperators<SourceLocationContainer>();
    QMetaType::registerComparators<SourceLocationContainer>();

    qRegisterMetaType<SourceRangeContainer>();
    qRegisterMetaTypeStreamOperators<SourceRangeContainer>();
    QMetaType::registerComparators<SourceRangeContainer>();

    qRegisterMetaType<RequestDiagnosticsMessage>();
    qRegisterMetaTypeStreamOperators<RequestDiagnosticsMessage>();
    QMetaType::registerComparators<RequestDiagnosticsMessage>();
}

} // namespace ClangBackEnd

