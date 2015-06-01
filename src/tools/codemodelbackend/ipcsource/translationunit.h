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

#ifndef CODEMODELBACKEND_TRANSLATIONUNIT_H
#define CODEMODELBACKEND_TRANSLATIONUNIT_H

#include <clang-c/Index.h>

#include <chrono>
#include <memory>

#include <QtGlobal>

class Utf8String;

namespace CodeModelBackEnd {

class TranslationUnitData;
class CodeCompleter;
class UnsavedFiles;
class ProjectPart;

using time_point = std::chrono::steady_clock::time_point;

class TranslationUnit
{
public:
    enum FileExistsCheck {
        CheckIfFileExists,
        DoNotCheckIfFileExists
    };

    TranslationUnit() = default;
    TranslationUnit(const Utf8String &filePath,
                    const UnsavedFiles &unsavedFiles,
                    const ProjectPart &projectPart,
                    FileExistsCheck fileExistsCheck = CheckIfFileExists);
    ~TranslationUnit();

    TranslationUnit(const TranslationUnit &cxTranslationUnit);
    TranslationUnit &operator =(const TranslationUnit &cxTranslationUnit);

    TranslationUnit(TranslationUnit &&cxTranslationUnit);
    TranslationUnit &operator =(TranslationUnit &&cxTranslationUnit);

    bool isNull() const;

    void reset();

    CXIndex index() const;
    CXTranslationUnit cxTranslationUnit() const;
    CXUnsavedFile * cxUnsavedFiles() const;
    uint unsavedFilesCount() const;

    const Utf8String &filePath() const;
    const Utf8String &projectPartId() const;

    const time_point &lastChangeTimePoint() const;

private:
    void checkIfNull() const;
    void checkIfFileExists() const;
    void updateLastChangeTimePoint() const;
    void removeOutdatedTranslationUnit() const;
    void createTranslationUnitIfNeeded() const;
    void checkTranslationUnitErrorCode(CXErrorCode errorCode) const;

private:
    mutable std::shared_ptr<TranslationUnitData> d;
};

bool operator ==(const TranslationUnit &first, const TranslationUnit &second);

} // namespace CodeModelBackEnd

#endif // CODEMODELBACKEND_TRANSLATIONUNIT_H
