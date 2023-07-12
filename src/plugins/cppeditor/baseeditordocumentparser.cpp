// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baseeditordocumentparser.h"
#include "baseeditordocumentprocessor.h"

#include "cppmodelmanager.h"
#include "cppprojectpartchooser.h"
#include "editordocumenthandle.h"

#include <QPromise>

using namespace Utils;

namespace CppEditor {

/*!
    \class CppEditor::BaseEditorDocumentParser

    \brief The BaseEditorDocumentParser class parses a source text as
           precisely as possible.

    It's meant to be used in the C++ editor to get precise results by using
    the "best" project part for a file.

    Derived classes are expected to implement updateImpl() this way:

    \list
        \li Get a copy of the configuration and the last state.
        \li Work on the data and do whatever is necessary. At least, update
            the project part with the help of determineProjectPart().
        \li Ensure the new state is set before updateImpl() returns.
    \endlist
*/

BaseEditorDocumentParser::BaseEditorDocumentParser(const FilePath &filePath)
    : m_filePath(filePath)
{
    static int meta = qRegisterMetaType<ProjectPartInfo>("ProjectPartInfo");
    Q_UNUSED(meta)
}

BaseEditorDocumentParser::~BaseEditorDocumentParser() = default;

const FilePath &BaseEditorDocumentParser::filePath() const
{
    return m_filePath;
}

BaseEditorDocumentParser::Configuration BaseEditorDocumentParser::configuration() const
{
    QMutexLocker locker(&m_stateAndConfigurationMutex);
    return m_configuration;
}

void BaseEditorDocumentParser::setConfiguration(const Configuration &configuration)
{
    QMutexLocker locker(&m_stateAndConfigurationMutex);
    m_configuration = configuration;
}

void BaseEditorDocumentParser::update(const UpdateParams &updateParams)
{
    QPromise<void> dummy;
    dummy.start();
    update(dummy, updateParams);
}

void BaseEditorDocumentParser::update(const QPromise<void> &promise,
                                      const UpdateParams &updateParams)
{
    QMutexLocker locker(&m_updateIsRunning);
    updateImpl(promise, updateParams);
}

BaseEditorDocumentParser::State BaseEditorDocumentParser::state() const
{
    QMutexLocker locker(&m_stateAndConfigurationMutex);
    return m_state;
}

void BaseEditorDocumentParser::setState(const State &state)
{
    QMutexLocker locker(&m_stateAndConfigurationMutex);
    m_state = state;
}

ProjectPartInfo BaseEditorDocumentParser::projectPartInfo() const
{
    return state().projectPartInfo;
}

BaseEditorDocumentParser::Ptr BaseEditorDocumentParser::get(const FilePath &filePath)
{
    if (CppEditorDocumentHandle *cppEditorDocument = CppModelManager::cppEditorDocument(filePath)) {
        if (BaseEditorDocumentProcessor *processor = cppEditorDocument->processor())
            return processor->parser();
    }
    return BaseEditorDocumentParser::Ptr();
}

ProjectPartInfo BaseEditorDocumentParser::determineProjectPart(const QString &filePath,
        const QString &preferredProjectPartId,
        const ProjectPartInfo &currentProjectPartInfo,
        const Utils::FilePath &activeProject,
        Utils::Language languagePreference,
        bool projectsUpdated)
{
    Internal::ProjectPartChooser chooser;
    chooser.setFallbackProjectPart([](){
        return CppModelManager::fallbackProjectPart();
    });
    chooser.setProjectPartsForFile([](const QString &filePath) {
        return CppModelManager::projectPart(filePath);
    });
    chooser.setProjectPartsFromDependenciesForFile([&](const QString &filePath) {
        const auto fileName = Utils::FilePath::fromString(filePath);
        return CppModelManager::projectPartFromDependencies(fileName);
    });

    const ProjectPartInfo chooserResult
            = chooser.choose(filePath,
                             currentProjectPartInfo,
                             preferredProjectPartId,
                             activeProject,
                             languagePreference,
                             projectsUpdated);

    return chooserResult;
}

} // namespace CppEditor
