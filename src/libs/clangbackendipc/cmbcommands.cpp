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

#include "cmbcommands.h"

#include "cmbalivecommand.h"
#include "cmbendcommand.h"
#include "cmbechocommand.h"
#include "cmbregistertranslationunitsforcodecompletioncommand.h"
#include "cmbunregistertranslationunitsforcodecompletioncommand.h"
#include "cmbregisterprojectsforcodecompletioncommand.h"
#include "cmbunregisterprojectsforcodecompletioncommand.h"
#include "cmbcompletecodecommand.h"
#include "cmbcodecompletedcommand.h"
#include "projectpartsdonotexistcommand.h"
#include "translationunitdoesnotexistcommand.h"

#include <QDataStream>

namespace ClangBackEnd {

void Commands::registerCommands()
{
    qRegisterMetaType<EndCommand>();
    qRegisterMetaTypeStreamOperators<EndCommand>();
    QMetaType::registerComparators<EndCommand>();

    qRegisterMetaType<AliveCommand>();
    qRegisterMetaTypeStreamOperators<AliveCommand>();
    QMetaType::registerComparators<AliveCommand>();

    qRegisterMetaType<EchoCommand>();
    qRegisterMetaTypeStreamOperators<EchoCommand>();

    qRegisterMetaType<RegisterTranslationUnitForCodeCompletionCommand>();
    qRegisterMetaTypeStreamOperators<RegisterTranslationUnitForCodeCompletionCommand>();
    QMetaType::registerComparators<RegisterTranslationUnitForCodeCompletionCommand>();

    qRegisterMetaType<FileContainer>();
    qRegisterMetaTypeStreamOperators<FileContainer>();
    QMetaType::registerComparators<FileContainer>();

    qRegisterMetaType<UnregisterTranslationUnitsForCodeCompletionCommand>();
    qRegisterMetaTypeStreamOperators<UnregisterTranslationUnitsForCodeCompletionCommand>();
    QMetaType::registerComparators<UnregisterTranslationUnitsForCodeCompletionCommand>();

    qRegisterMetaType<CompleteCodeCommand>();
    qRegisterMetaTypeStreamOperators<CompleteCodeCommand>();
    QMetaType::registerComparators<CompleteCodeCommand>();

    qRegisterMetaType<CodeCompletion>();
    qRegisterMetaTypeStreamOperators<CodeCompletion>();
    QMetaType::registerComparators<CodeCompletion>();

    qRegisterMetaType<CodeCompletedCommand>();
    qRegisterMetaTypeStreamOperators<CodeCompletedCommand>();
    QMetaType::registerComparators<CodeCompletedCommand>();

    qRegisterMetaType<RegisterProjectPartsForCodeCompletionCommand>();
    qRegisterMetaTypeStreamOperators<RegisterProjectPartsForCodeCompletionCommand>();
    QMetaType::registerComparators<RegisterProjectPartsForCodeCompletionCommand>();

    qRegisterMetaType<ProjectPartContainer>();
    qRegisterMetaTypeStreamOperators<ProjectPartContainer>();
    QMetaType::registerComparators<ProjectPartContainer>();

    qRegisterMetaType<UnregisterProjectPartsForCodeCompletionCommand>();
    qRegisterMetaTypeStreamOperators<UnregisterProjectPartsForCodeCompletionCommand>();
    QMetaType::registerComparators<UnregisterProjectPartsForCodeCompletionCommand>();

    qRegisterMetaType<TranslationUnitDoesNotExistCommand>();
    qRegisterMetaTypeStreamOperators<TranslationUnitDoesNotExistCommand>();
    QMetaType::registerComparators<TranslationUnitDoesNotExistCommand>();

    qRegisterMetaType<ProjectPartsDoNotExistCommand>();
    qRegisterMetaTypeStreamOperators<ProjectPartsDoNotExistCommand>();
    QMetaType::registerComparators<ProjectPartsDoNotExistCommand>();
}

} // namespace ClangBackEnd

