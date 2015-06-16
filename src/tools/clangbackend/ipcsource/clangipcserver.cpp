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

#include "clangipcserver.h"

#include <QCoreApplication>

#include <cmbregistertranslationunitsforcodecompletioncommand.h>
#include <cmbunregistertranslationunitsforcodecompletioncommand.h>
#include <cmbregisterprojectsforcodecompletioncommand.h>
#include <cmbunregisterprojectsforcodecompletioncommand.h>
#include <cmbcodecompletedcommand.h>
#include <cmbcompletecodecommand.h>
#include <translationunitdoesnotexistcommand.h>
#include <projectpartsdonotexistcommand.h>

#include "translationunits.h"
#include "codecompleter.h"
#include "translationunitdoesnotexistexception.h"
#include "translationunitisnullexception.h"
#include "translationunitfilenotexitexception.h"
#include "projectpartsdonotexistexception.h"
#include "translationunitparseerrorexception.h"

#include <QDebug>

namespace ClangBackEnd {

ClangIpcServer::ClangIpcServer()
    : translationUnits(projects, unsavedFiles)
{
}

void ClangIpcServer::end()
{
    QCoreApplication::exit();
}

void ClangIpcServer::registerTranslationUnitsForCodeCompletion(const ClangBackEnd::RegisterTranslationUnitForCodeCompletionCommand &command)
{
    try {
        translationUnits.createOrUpdate(command.fileContainers());
        unsavedFiles.createOrUpdate(command.fileContainers());
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistCommand(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::registerTranslationUnitsForCodeCompletion:" << exception.what();
    }
}

void ClangIpcServer::unregisterTranslationUnitsForCodeCompletion(const ClangBackEnd::UnregisterTranslationUnitsForCodeCompletionCommand &command)
{
    try {
        translationUnits.remove(command.fileContainers());
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistCommand(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistCommand(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::unregisterTranslationUnitsForCodeCompletion:" << exception.what();
    }
}

void ClangIpcServer::registerProjectPartsForCodeCompletion(const RegisterProjectPartsForCodeCompletionCommand &command)
{
    try {
        projects.createOrUpdate(command.projectContainers());
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::registerProjectPartsForCodeCompletion:" << exception.what();
    }
}

void ClangIpcServer::unregisterProjectPartsForCodeCompletion(const UnregisterProjectPartsForCodeCompletionCommand &command)
{
    try {
        projects.remove(command.filePaths());
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistCommand(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::unregisterProjectPartsForCodeCompletion:" << exception.what();
    }
}

void ClangIpcServer::completeCode(const ClangBackEnd::CompleteCodeCommand &command)
{
    try {
        CodeCompleter codeCompleter(translationUnits.translationUnit(command.filePath(), command.projectPartId()));

        const auto codeCompletions = codeCompleter.complete(command.line(), command.column());

        client()->codeCompleted(CodeCompletedCommand(codeCompletions, command.ticketNumber()));
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistCommand(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistCommand(exception.projectPartIds()));
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::unregisterProjectPartsForCodeCompletion:" << exception.what();
    }
}

}
