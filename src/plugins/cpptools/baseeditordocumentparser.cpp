/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "baseeditordocumentparser.h"

#include "editordocumenthandle.h"

namespace CppTools {

/*!
    \class CppTools::BaseEditorDocumentParser

    \brief The BaseEditorDocumentParser class parses a source text as
           precisely as possible.

    It's meant to be used in the C++ editor to get precise results by using
    the "best" project part for a file.

    Derived classes are expected to implement update() by using the protected
    mutex, updateProjectPart() and by respecting the options set by the client.
*/

BaseEditorDocumentParser::BaseEditorDocumentParser(const QString &filePath)
    : m_mutex(QMutex::Recursive)
    , m_filePath(filePath)
    , m_usePrecompiledHeaders(false)
    , m_editorDefinesChangedSinceLastUpdate(false)
{
}

BaseEditorDocumentParser::~BaseEditorDocumentParser()
{
}

QString BaseEditorDocumentParser::filePath() const
{
    return m_filePath;
}

ProjectPart::Ptr BaseEditorDocumentParser::projectPart() const
{
    QMutexLocker locker(&m_mutex);
    return m_projectPart;
}

void BaseEditorDocumentParser::setProjectPart(ProjectPart::Ptr projectPart)
{
    QMutexLocker locker(&m_mutex);
    m_manuallySetProjectPart = projectPart;
}

bool BaseEditorDocumentParser::usePrecompiledHeaders() const
{
    QMutexLocker locker(&m_mutex);
    return m_usePrecompiledHeaders;
}

void BaseEditorDocumentParser::setUsePrecompiledHeaders(bool usePrecompiledHeaders)
{
    QMutexLocker locker(&m_mutex);
    m_usePrecompiledHeaders = usePrecompiledHeaders;
}

QByteArray BaseEditorDocumentParser::editorDefines() const
{
    QMutexLocker locker(&m_mutex);
    return m_editorDefines;
}

void BaseEditorDocumentParser::setEditorDefines(const QByteArray &editorDefines)
{
    QMutexLocker locker(&m_mutex);

    if (editorDefines != m_editorDefines) {
        m_editorDefines = editorDefines;
        m_editorDefinesChangedSinceLastUpdate = true;
    }
}

BaseEditorDocumentParser *BaseEditorDocumentParser::get(const QString &filePath)
{
    CppModelManager *cmmi = CppModelManager::instance();
    if (EditorDocumentHandle *editorDocument = cmmi->editorDocument(filePath)) {
        if (BaseEditorDocumentProcessor *processor = editorDocument->processor())
            return processor->parser();
    }
    return 0;
}

void BaseEditorDocumentParser::updateProjectPart()
{
    if (m_manuallySetProjectPart) {
        m_projectPart = m_manuallySetProjectPart;
        return;
    }

    CppModelManager *cmm = CppModelManager::instance();
    QList<ProjectPart::Ptr> projectParts = cmm->projectPart(m_filePath);
    if (projectParts.isEmpty()) {
        if (m_projectPart)
            // File is not directly part of any project, but we got one before. We will re-use it,
            // because re-calculating this can be expensive when the dependency table is big.
            return;

        // Fall-back step 1: Get some parts through the dependency table:
        projectParts = cmm->projectPartFromDependencies(m_filePath);
        if (projectParts.isEmpty())
            // Fall-back step 2: Use fall-back part from the model manager:
            m_projectPart = cmm->fallbackProjectPart();
        else
            m_projectPart = projectParts.first();
    } else {
        if (!projectParts.contains(m_projectPart))
            // Apparently the project file changed, so update our project part.
            m_projectPart = projectParts.first();
    }
}

bool BaseEditorDocumentParser::editorDefinesChanged() const
{
    return m_editorDefinesChangedSinceLastUpdate;
}

void BaseEditorDocumentParser::resetEditorDefinesChanged()
{
    m_editorDefinesChangedSinceLastUpdate = false;
}

} // namespace CppTools
