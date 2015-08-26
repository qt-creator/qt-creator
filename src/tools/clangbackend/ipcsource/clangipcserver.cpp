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

#include "clangfilesystemwatcher.h"
#include "codecompleter.h"
#include "diagnosticset.h"
#include "projectpartsdonotexistexception.h"
#include "translationunitdoesnotexistexception.h"
#include "translationunitfilenotexitexception.h"
#include "translationunitisnullexception.h"
#include "translationunitparseerrorexception.h"
#include "translationunits.h"

#include <clangbackendipcdebugutils.h>
#include <cmbcodecompletedmessage.h>
#include <cmbcompletecodemessage.h>
#include <cmbregisterprojectsforcodecompletionmessage.h>
#include <cmbregistertranslationunitsforcodecompletionmessage.h>
#include <cmbunregisterprojectsforcodecompletionmessage.h>
#include <cmbunregistertranslationunitsforcodecompletionmessage.h>
#include <diagnosticschangedmessage.h>
#include <requestdiagnosticsmessage.h>
#include <projectpartsdonotexistmessage.h>
#include <translationunitdoesnotexistmessage.h>

#include <QCoreApplication>
#include <QDebug>

namespace ClangBackEnd {

ClangIpcServer::ClangIpcServer()
    : translationUnits(projects, unsavedFiles)
{
    translationUnits.setSendChangeDiagnosticsCallback([this] (const DiagnosticsChangedMessage &message)
                                                      {
                                                          client()->diagnosticsChanged(message);
                                                      });

    sendDiagnosticsTimer.setInterval(1000);
    QObject::connect(&sendDiagnosticsTimer,
                     &QTimer::timeout,
                     [this] () { translationUnits.sendChangedDiagnostics(); });

    QObject::connect(translationUnits.clangFileSystemWatcher(),
                     &ClangFileSystemWatcher::fileChanged,
                     [this] () { sendDiagnosticsTimer.start(); });
}

void ClangIpcServer::end()
{
    QCoreApplication::exit();
}

void ClangIpcServer::registerTranslationUnitsForCodeCompletion(const ClangBackEnd::RegisterTranslationUnitForCodeCompletionMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::registerTranslationUnitsForCodeCompletion");

    try {
        const auto newerFileContainers = translationUnits.newerFileContainers(message.fileContainers());
        if (newerFileContainers.size() > 0) {
            unsavedFiles.createOrUpdate(newerFileContainers);
            translationUnits.createOrUpdate(newerFileContainers);
            sendDiagnosticsTimer.start();
        }
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::registerTranslationUnitsForCodeCompletion:" << exception.what();
    }
}

void ClangIpcServer::unregisterTranslationUnitsForCodeCompletion(const ClangBackEnd::UnregisterTranslationUnitsForCodeCompletionMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::unregisterTranslationUnitsForCodeCompletion");

    try {
        unsavedFiles.remove(message.fileContainers());
        translationUnits.remove(message.fileContainers());
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::unregisterTranslationUnitsForCodeCompletion:" << exception.what();
    }
}

void ClangIpcServer::registerProjectPartsForCodeCompletion(const RegisterProjectPartsForCodeCompletionMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::registerProjectPartsForCodeCompletion");

    try {
        projects.createOrUpdate(message.projectContainers());
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::registerProjectPartsForCodeCompletion:" << exception.what();
    }
}

void ClangIpcServer::unregisterProjectPartsForCodeCompletion(const UnregisterProjectPartsForCodeCompletionMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::unregisterProjectPartsForCodeCompletion");

    try {
        projects.remove(message.projectPartIds());
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::unregisterProjectPartsForCodeCompletion:" << exception.what();
    }
}

void ClangIpcServer::completeCode(const ClangBackEnd::CompleteCodeMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::completeCode");

    try {
        CodeCompleter codeCompleter(translationUnits.translationUnit(message.filePath(), message.projectPartId()));

        const auto codeCompletions = codeCompleter.complete(message.line(), message.column());

        client()->codeCompleted(CodeCompletedMessage(codeCompletions, message.ticketNumber()));
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::completeCode:" << exception.what();
    }
}

void ClangIpcServer::requestDiagnostics(const RequestDiagnosticsMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::requestDiagnostics");

    try {
        auto translationUnit = translationUnits.translationUnit(message.file().filePath(),
                                                                message.file().projectPartId());

        client()->diagnosticsChanged(DiagnosticsChangedMessage(translationUnit.fileContainer(),
                                                               translationUnit.diagnostics().toDiagnosticContainers()));
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::requestDiagnostics:" << exception.what();
    }
}

}
