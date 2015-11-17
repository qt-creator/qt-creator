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
#include <cmbregisterprojectsforeditormessage.h>
#include <cmbregistertranslationunitsforeditormessage.h>
#include <cmbunregisterprojectsforeditormessage.h>
#include <cmbunregistertranslationunitsforeditormessage.h>
#include <diagnosticschangedmessage.h>
#include <registerunsavedfilesforeditormessage.h>
#include <requestdiagnosticsmessage.h>
#include <projectpartsdonotexistmessage.h>
#include <translationunitdoesnotexistmessage.h>
#include <unregisterunsavedfilesforeditormessage.h>
#include <updatetranslationunitsforeditormessage.h>

#include <QCoreApplication>
#include <QDebug>

namespace ClangBackEnd {

namespace {
const int sendDiagnosticsTimerInterval = 300;
}

ClangIpcServer::ClangIpcServer()
    : translationUnits(projects, unsavedFiles)
{
    translationUnits.setSendChangeDiagnosticsCallback([this] (const DiagnosticsChangedMessage &message)
                                                      {
                                                          client()->diagnosticsChanged(message);
                                                      });

    QObject::connect(&sendDiagnosticsTimer,
                     &QTimer::timeout,
                     [this] () {
                         try {
                             auto diagnostSendState = translationUnits.sendChangedDiagnostics();
                             if (diagnostSendState == DiagnosticSendState::MaybeThereAreMoreDiagnostics)
                                 sendDiagnosticsTimer.setInterval(0);
                             else
                                 sendDiagnosticsTimer.stop();
                         } catch (const std::exception &exception) {
                             qWarning() << "Error in ClangIpcServer::sendDiagnosticsTimer:" << exception.what();
                         }
                     });

    QObject::connect(translationUnits.clangFileSystemWatcher(),
                     &ClangFileSystemWatcher::fileChanged,
                     [this] (const Utf8String &filePath)
                     {
                         startSendDiagnosticTimerIfFileIsNotATranslationUnit(filePath);
                     });
}

void ClangIpcServer::end()
{
    QCoreApplication::exit();
}

void ClangIpcServer::registerTranslationUnitsForEditor(const ClangBackEnd::RegisterTranslationUnitForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::registerTranslationUnitsForEditor");

    try {
        translationUnits.create(message.fileContainers());
        unsavedFiles.createOrUpdate(message.fileContainers());
        sendDiagnosticsTimer.start(0);
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::registerTranslationUnitsForEditor:" << exception.what();
    }
}

void ClangIpcServer::updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::updateTranslationUnitsForEditor");

    try {
        const auto newerFileContainers = translationUnits.newerFileContainers(message.fileContainers());
        if (newerFileContainers.size() > 0) {
            translationUnits.update(newerFileContainers);
            unsavedFiles.createOrUpdate(newerFileContainers);
            sendDiagnosticsTimer.start(sendDiagnosticsTimerInterval);
        }
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::updateTranslationUnitsForEditor:" << exception.what();
    }
}

void ClangIpcServer::unregisterTranslationUnitsForEditor(const ClangBackEnd::UnregisterTranslationUnitsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::unregisterTranslationUnitsForEditor");

    try {
        translationUnits.remove(message.fileContainers());
        unsavedFiles.remove(message.fileContainers());
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::unregisterTranslationUnitsForEditor:" << exception.what();
    }
}

void ClangIpcServer::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::registerProjectPartsForEditor");

    try {
        projects.createOrUpdate(message.projectContainers());
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::registerProjectPartsForEditor:" << exception.what();
    }
}

void ClangIpcServer::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::unregisterProjectPartsForEditor");

    try {
        projects.remove(message.projectPartIds());
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::unregisterProjectPartsForEditor:" << exception.what();
    }
}

void ClangIpcServer::registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::registerUnsavedFilesForEditor");

    try {
        unsavedFiles.createOrUpdate(message.fileContainers());
        translationUnits.updateTranslationUnitsWithChangedDependencies(message.fileContainers());
        sendDiagnosticsTimer.start(sendDiagnosticsTimerInterval);
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::registerUnsavedFilesForEditor:" << exception.what();
    }
}

void ClangIpcServer::unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::unregisterUnsavedFilesForEditor");

    try {
        unsavedFiles.remove(message.fileContainers());
        translationUnits.updateTranslationUnitsWithChangedDependencies(message.fileContainers());
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::unregisterUnsavedFilesForEditor:" << exception.what();
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
                                                               translationUnit.mainFileDiagnostics()));
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::requestDiagnostics:" << exception.what();
    }
}

const TranslationUnits &ClangIpcServer::translationUnitsForTestOnly() const
{
    return translationUnits;
}

void ClangIpcServer::startSendDiagnosticTimerIfFileIsNotATranslationUnit(const Utf8String &filePath)
{
    if (!translationUnits.hasTranslationUnit(filePath))
        sendDiagnosticsTimer.start(0);
}

}
