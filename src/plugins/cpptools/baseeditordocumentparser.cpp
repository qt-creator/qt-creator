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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baseeditordocumentparser.h"
#include "baseeditordocumentprocessor.h"

#include "cpptoolsreuse.h"
#include "editordocumenthandle.h"

namespace CppTools {

/*!
    \class CppTools::BaseEditorDocumentParser

    \brief The BaseEditorDocumentParser class parses a source text as
           precisely as possible.

    It's meant to be used in the C++ editor to get precise results by using
    the "best" project part for a file.

    Derived classes are expected to implement updateHelper() this way:

    \list
        \li Get a copy of the configuration and the last state.
        \li Work on the data and do whatever is necessary. At least, update
            the project part with the help of determineProjectPart().
        \li Ensure the new state is set before updateHelper() returns.
    \endlist
*/

BaseEditorDocumentParser::BaseEditorDocumentParser(const QString &filePath)
    : m_filePath(filePath)
{
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

void BaseEditorDocumentParser::update(const InMemoryInfo &info)
{
    QMutexLocker locker(&m_updateIsRunning);
    updateHelper(info);
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

ProjectPart::Ptr BaseEditorDocumentParser::projectPart() const
{
    return state().projectPart;
}

BaseEditorDocumentParser *BaseEditorDocumentParser::get(const QString &filePath)
{
    CppModelManager *cmmi = CppModelManager::instance();
    if (CppEditorDocumentHandle *cppEditorDocument = cmmi->cppEditorDocument(filePath)) {
        if (BaseEditorDocumentProcessor *processor = cppEditorDocument->processor())
            return processor->parser();
    }
    return 0;
}

ProjectPart::Ptr BaseEditorDocumentParser::determineProjectPart(const QString &filePath,
                                                                const Configuration &config,
                                                                const State &state)
{
    if (config.manuallySetProjectPart)
        return config.manuallySetProjectPart;

    ProjectPart::Ptr projectPart = state.projectPart;

    CppModelManager *cmm = CppModelManager::instance();
    QList<ProjectPart::Ptr> projectParts = cmm->projectPart(filePath);
    if (projectParts.isEmpty()) {
        if (projectPart && config.stickToPreviousProjectPart)
            // File is not directly part of any project, but we got one before. We will re-use it,
            // because re-calculating this can be expensive when the dependency table is big.
            return projectPart;

        // Fall-back step 1: Get some parts through the dependency table:
        projectParts = cmm->projectPartFromDependencies(Utils::FileName::fromString(filePath));
        if (projectParts.isEmpty())
            // Fall-back step 2: Use fall-back part from the model manager:
            projectPart = cmm->fallbackProjectPart();
        else
            projectPart = projectParts.first();
    } else {
        if (!projectParts.contains(projectPart))
            // Apparently the project file changed, so update our project part.
            projectPart = projectParts.first();
    }

    return projectPart;
}

BaseEditorDocumentParser::InMemoryInfo::InMemoryInfo(bool withModifiedFiles)
    : workingCopy(CppTools::CppModelManager::instance()->workingCopy())
{
    if (withModifiedFiles)
        modifiedFiles = CppTools::modifiedFiles();
}

} // namespace CppTools
