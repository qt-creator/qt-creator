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

#include "clangipcserver.h"

#include "clangfilesystemwatcher.h"
#include "codecompleter.h"
#include "diagnosticset.h"
#include "highlightinginformations.h"
#include "projectpartsdonotexistexception.h"
#include "skippedsourceranges.h"
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
#include <highlightingchangedmessage.h>
#include <registerunsavedfilesforeditormessage.h>
#include <requestdiagnosticsmessage.h>
#include <requesthighlightingmessage.h>
#include <projectpartsdonotexistmessage.h>
#include <translationunitdoesnotexistmessage.h>
#include <unregisterunsavedfilesforeditormessage.h>
#include <updatetranslationunitsforeditormessage.h>
#include <updatevisibletranslationunitsmessage.h>

#include <QCoreApplication>
#include <QDebug>

namespace ClangBackEnd {

namespace {

int getIntervalFromEnviromentVariable()
{
    const QByteArray userIntervalAsByteArray = qgetenv("QTC_CLANG_DELAYED_REPARSE_TIMEOUT");

    bool isConversionOk = false;
    const int intervalAsInt = userIntervalAsByteArray.toInt(&isConversionOk);

    if (isConversionOk)
        return intervalAsInt;
    else
        return -1;
}

int delayedDocumentAnnotationsTimerInterval()
{
    static const int defaultInterval = 1500;
    static const int userDefinedInterval = getIntervalFromEnviromentVariable();
    static const int interval = userDefinedInterval >= 0 ? userDefinedInterval : defaultInterval;

    return interval;
}

}

ClangIpcServer::ClangIpcServer()
    : translationUnits(projects, unsavedFiles)
{
    const auto sendDocumentAnnotations
        = [this] (const DiagnosticsChangedMessage &diagnosticsMessage,
                  const HighlightingChangedMessage &highlightingsMessage) {
        client()->diagnosticsChanged(diagnosticsMessage);
        client()->highlightingChanged(highlightingsMessage);
    };

    const auto sendDelayedDocumentAnnotations = [this] () {
        try {
            auto sendState = translationUnits.sendDocumentAnnotations();
            if (sendState == DocumentAnnotationsSendState::MaybeThereAreDocumentAnnotations)
                sendDocumentAnnotationsTimer.setInterval(0);
            else
                sendDocumentAnnotationsTimer.stop();
        } catch (const std::exception &exception) {
            qWarning() << "Error in ClangIpcServer::sendDelayedDocumentAnnotationsTimer:" << exception.what();
        }
    };

    const auto onFileChanged = [this] (const Utf8String &filePath) {
        startDocumentAnnotationsTimerIfFileIsNotATranslationUnit(filePath);
    };

    translationUnits.setSendDocumentAnnotationsCallback(sendDocumentAnnotations);

    QObject::connect(&sendDocumentAnnotationsTimer,
                     &QTimer::timeout,
                     sendDelayedDocumentAnnotations);

    QObject::connect(translationUnits.clangFileSystemWatcher(),
                     &ClangFileSystemWatcher::fileChanged,
                     onFileChanged);
}

void ClangIpcServer::end()
{
    QCoreApplication::exit();
}

void ClangIpcServer::registerTranslationUnitsForEditor(const ClangBackEnd::RegisterTranslationUnitForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::registerTranslationUnitsForEditor");

    try {
        auto createdTranslationUnits = translationUnits.create(message.fileContainers());
        unsavedFiles.createOrUpdate(message.fileContainers());
        translationUnits.setUsedByCurrentEditor(message.currentEditorFilePath());
        translationUnits.setVisibleInEditors(message.visibleEditorFilePaths());
        startDocumentAnnotations();
        reparseVisibleDocuments(createdTranslationUnits);
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
            sendDocumentAnnotationsTimer.start(delayedDocumentAnnotationsTimerInterval());
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
        translationUnits.setTranslationUnitsDirtyIfProjectPartChanged();
        sendDocumentAnnotationsTimer.start(0);
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
        sendDocumentAnnotationsTimer.start(delayedDocumentAnnotationsTimerInterval());
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

        client()->codeCompleted(CodeCompletedMessage(codeCompletions,
                                                     codeCompleter.neededCorrection(),
                                                     message.ticketNumber()));
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

void ClangIpcServer::requestHighlighting(const RequestHighlightingMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::requestHighlighting");

    try {
        auto translationUnit = translationUnits.translationUnit(message.fileContainer().filePath(),
                                                                message.fileContainer().projectPartId());

        client()->highlightingChanged(HighlightingChangedMessage(translationUnit.fileContainer(),
                                                                 translationUnit.highlightingInformations().toHighlightingMarksContainers(),
                                                                 translationUnit.skippedSourceRanges().toSourceRangeContainers()));
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::requestHighlighting:" << exception.what();
    }
}

void ClangIpcServer::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    TIME_SCOPE_DURATION("ClangIpcServer::updateVisibleTranslationUnits");

    try {
        translationUnits.setUsedByCurrentEditor(message.currentEditorFilePath());
        translationUnits.setVisibleInEditors(message.visibleEditorFilePaths());
        sendDocumentAnnotationsTimer.start(0);
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangIpcServer::updateVisibleTranslationUnits:" << exception.what();
    }
}

const TranslationUnits &ClangIpcServer::translationUnitsForTestOnly() const
{
    return translationUnits;
}

void ClangIpcServer::startDocumentAnnotationsTimerIfFileIsNotATranslationUnit(const Utf8String &filePath)
{
    if (!translationUnits.hasTranslationUnit(filePath))
        sendDocumentAnnotationsTimer.start(0);
}

void ClangIpcServer::startDocumentAnnotations()
{
    DocumentAnnotationsSendState sendState = DocumentAnnotationsSendState::MaybeThereAreDocumentAnnotations;

    while (sendState == DocumentAnnotationsSendState::MaybeThereAreDocumentAnnotations)
        sendState = translationUnits.sendDocumentAnnotations();
}

void ClangIpcServer::reparseVisibleDocuments(std::vector<TranslationUnit> &translationUnits)
{
    for (TranslationUnit &translationUnit : translationUnits)
        if (translationUnit.isVisibleInEditor())
            translationUnit.reparse();
}

}
