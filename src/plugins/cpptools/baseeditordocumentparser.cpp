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

#include "baseeditordocumentparser.h"
#include "baseeditordocumentprocessor.h"

#include "cppmodelmanager.h"
#include "cppprojectpartchooser.h"
#include "editordocumenthandle.h"

namespace CppTools {

/*!
    \class CppTools::BaseEditorDocumentParser

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

BaseEditorDocumentParser::BaseEditorDocumentParser(const QString &filePath)
    : m_filePath(filePath)
{
    static int meta = qRegisterMetaType<ProjectPartInfo>("CppTools::ProjectPartInfo");
    Q_UNUSED(meta);
}

BaseEditorDocumentParser::~BaseEditorDocumentParser()
{
}

QString BaseEditorDocumentParser::filePath() const
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
    QFutureInterface<void> dummy;
    update(dummy, updateParams);
}

void BaseEditorDocumentParser::update(const QFutureInterface<void> &future,
                                      const UpdateParams &updateParams)
{
    QMutexLocker locker(&m_updateIsRunning);
    updateImpl(future, updateParams);
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

BaseEditorDocumentParser::Ptr BaseEditorDocumentParser::get(const QString &filePath)
{
    CppModelManager *cmmi = CppModelManager::instance();
    if (CppEditorDocumentHandle *cppEditorDocument = cmmi->cppEditorDocument(filePath)) {
        if (BaseEditorDocumentProcessor *processor = cppEditorDocument->processor())
            return processor->parser();
    }
    return BaseEditorDocumentParser::Ptr();
}

ProjectPartInfo BaseEditorDocumentParser::determineProjectPart(
        const QString &filePath,
        const QString &preferredProjectPartId,
        const ProjectPartInfo &currentProjectPartInfo,
        const ProjectExplorer::Project *activeProject,
        Language languagePreference,
        bool projectsUpdated)
{
    Internal::ProjectPartChooser chooser;
    chooser.setFallbackProjectPart([](){
        return CppModelManager::instance()->fallbackProjectPart();
    });
    chooser.setProjectPartsForFile([](const QString &filePath) {
        return CppModelManager::instance()->projectPart(filePath);
    });
    chooser.setProjectPartsFromDependenciesForFile([&](const QString &filePath) {
        const auto fileName = Utils::FileName::fromString(filePath);
        return CppModelManager::instance()->projectPartFromDependencies(fileName);
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

} // namespace CppTools
