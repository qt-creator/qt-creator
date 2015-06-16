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

#ifndef CLANGBACKEND_TRANSLATIONUNITS_H
#define CLANGBACKEND_TRANSLATIONUNITS_H

#include "translationunit.h"

#include <filecontainer.h>

#include <QVector>

#include <vector>

namespace ClangBackEnd {

class ProjectParts;
class UnsavedFiles;

class TranslationUnits
{
public:
    TranslationUnits(ProjectParts &projects, UnsavedFiles &unsavedFiles);

    void createOrUpdate(const QVector<FileContainer> &fileContainers);
    void remove(const QVector<FileContainer> &fileContainers);

    const TranslationUnit &translationUnit(const Utf8String &filePath, const Utf8String &projectPartId) const;

    const std::vector<TranslationUnit> &translationUnits() const;

private:
    void createOrUpdateTranslationUnit(const FileContainer &fileContainer);
    std::vector<TranslationUnit>::iterator findTranslationUnit(const FileContainer &fileContainer);
    std::vector<TranslationUnit>::const_iterator findTranslationUnit(const Utf8String &filePath, const Utf8String &projectPartId) const;
    void checkIfProjectPartExists(const Utf8String &projectFileName) const;
    void checkIfProjectPartsExists(const QVector<FileContainer> &fileContainers) const;


private:
    std::vector<TranslationUnit> translationUnits_;
    ProjectParts &projects;
    UnsavedFiles &unsavedFiles;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_TRANSLATIONUNITS_H
