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

#include "clangcodemodelserver.h"

#include "clangfilesystemwatcher.h"
#include "codecompleter.h"
#include "diagnosticset.h"
#include "highlightingmarks.h"
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
#include <documentannotationschangedmessage.h>
#include <registerunsavedfilesforeditormessage.h>
#include <requestdocumentannotations.h>
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

ClangCodeModelServer::ClangCodeModelServer()
    : translationUnits(projects, unsavedFiles)
{
    const auto sendDocumentAnnotations
        = [this] (const DocumentAnnotationsChangedMessage &documentAnnotationsChangedMessage) {
        client()->documentAnnotationsChanged(documentAnnotationsChangedMessage);
    };

    const auto sendDelayedDocumentAnnotations = [this] () {
        try {
            auto sendState = translationUnits.sendDocumentAnnotations();
            if (sendState == DocumentAnnotationsSendState::MaybeThereAreDocumentAnnotations)
                sendDocumentAnnotationsTimer.setInterval(0);
            else
                sendDocumentAnnotationsTimer.stop();
        } catch (const std::exception &exception) {
            qWarning() << "Error in ClangCodeModelServer::sendDelayedDocumentAnnotationsTimer:" << exception.what();
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

void ClangCodeModelServer::end()
{
    QCoreApplication::exit();
}

void ClangCodeModelServer::registerTranslationUnitsForEditor(const ClangBackEnd::RegisterTranslationUnitForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::registerTranslationUnitsForEditor");

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
        qWarning() << "Error in ClangCodeModelServer::registerTranslationUnitsForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::updateTranslationUnitsForEditor");

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
        qWarning() << "Error in ClangCodeModelServer::updateTranslationUnitsForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::unregisterTranslationUnitsForEditor(const ClangBackEnd::UnregisterTranslationUnitsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::unregisterTranslationUnitsForEditor");

    try {
        translationUnits.remove(message.fileContainers());
        unsavedFiles.remove(message.fileContainers());
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::unregisterTranslationUnitsForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::registerProjectPartsForEditor");

    try {
        projects.createOrUpdate(message.projectContainers());
        translationUnits.setTranslationUnitsDirtyIfProjectPartChanged();
        sendDocumentAnnotationsTimer.start(0);
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::registerProjectPartsForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::unregisterProjectPartsForEditor");

    try {
        projects.remove(message.projectPartIds());
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::unregisterProjectPartsForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::registerUnsavedFilesForEditor");

    try {
        unsavedFiles.createOrUpdate(message.fileContainers());
        translationUnits.updateTranslationUnitsWithChangedDependencies(message.fileContainers());
        sendDocumentAnnotationsTimer.start(delayedDocumentAnnotationsTimerInterval());
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::registerUnsavedFilesForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::unregisterUnsavedFilesForEditor");

    try {
        unsavedFiles.remove(message.fileContainers());
        translationUnits.updateTranslationUnitsWithChangedDependencies(message.fileContainers());
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    } catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::unregisterUnsavedFilesForEditor:" << exception.what();
    }
}

void ClangCodeModelServer::completeCode(const ClangBackEnd::CompleteCodeMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::completeCode");

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
        qWarning() << "Error in ClangCodeModelServer::completeCode:" << exception.what();
    }
}

void ClangCodeModelServer::requestDocumentAnnotations(const RequestDocumentAnnotationsMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::requestDocumentAnnotations");

    try {
        auto translationUnit = translationUnits.translationUnit(message.fileContainer().filePath(),
                                                                message.fileContainer().projectPartId());

        client()->documentAnnotationsChanged(DocumentAnnotationsChangedMessage(translationUnit.fileContainer(),
                                                                               translationUnit.mainFileDiagnostics(),
                                                                               translationUnit.highlightingMarks().toHighlightingMarksContainers(),
                                                                               translationUnit.skippedSourceRanges().toSourceRangeContainers()));
    } catch (const TranslationUnitDoesNotExistException &exception) {
        client()->translationUnitDoesNotExist(TranslationUnitDoesNotExistMessage(exception.fileContainer()));
    } catch (const ProjectPartDoNotExistException &exception) {
        client()->projectPartsDoNotExist(ProjectPartsDoNotExistMessage(exception.projectPartIds()));
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::requestDocumentAnnotations:" << exception.what();
    }
}

void ClangCodeModelServer::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    TIME_SCOPE_DURATION("ClangCodeModelServer::updateVisibleTranslationUnits");

    try {
        translationUnits.setUsedByCurrentEditor(message.currentEditorFilePath());
        translationUnits.setVisibleInEditors(message.visibleEditorFilePaths());
        sendDocumentAnnotationsTimer.start(0);
    }  catch (const std::exception &exception) {
        qWarning() << "Error in ClangCodeModelServer::updateVisibleTranslationUnits:" << exception.what();
    }
}

const TranslationUnits &ClangCodeModelServer::translationUnitsForTestOnly() const
{
    return translationUnits;
}

void ClangCodeModelServer::startDocumentAnnotationsTimerIfFileIsNotATranslationUnit(const Utf8String &filePath)
{
    if (!translationUnits.hasTranslationUnit(filePath))
        sendDocumentAnnotationsTimer.start(0);
}

void ClangCodeModelServer::startDocumentAnnotations()
{
    DocumentAnnotationsSendState sendState = DocumentAnnotationsSendState::MaybeThereAreDocumentAnnotations;

    while (sendState == DocumentAnnotationsSendState::MaybeThereAreDocumentAnnotations)
        sendState = translationUnits.sendDocumentAnnotations();
}

void ClangCodeModelServer::reparseVisibleDocuments(std::vector<TranslationUnit> &translationUnits)
{
    for (TranslationUnit &translationUnit : translationUnits)
        if (translationUnit.isVisibleInEditor())
            translationUnit.reparse();
}

}
