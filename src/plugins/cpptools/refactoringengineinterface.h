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

#pragma once

#include "cpptools_global.h"
#include "cursorineditor.h"
#include "usages.h"

#include <utils/fileutils.h>
#include <utils/smallstring.h>

#include <clangsupport/sourcelocationscontainer.h>
#include <clangsupport/refactoringclientinterface.h>

namespace TextEditor {
class TextEditorWidget;
} // namespace TextEditor

namespace CppTools {

class ProjectPart;

enum class CallType
{
    Synchronous,
    Asynchronous
};

// NOTE: This interface is not supposed to be owned as an interface pointer
class CPPTOOLS_EXPORT RefactoringEngineInterface
{
public:
    using RenameCallback = ClangBackEnd::RefactoringClientInterface::RenameCallback;

    virtual ~RefactoringEngineInterface() {}

    virtual void startLocalRenaming(const CursorInEditor &data,
                                    CppTools::ProjectPart *projectPart,
                                    RenameCallback &&renameSymbolsCallback) = 0;
    virtual void globalRename(const CursorInEditor &data,
                              UsagesCallback &&renameCallback,
                              const QString &replacement) = 0;
    virtual void findUsages(const CppTools::CursorInEditor &data,
                            UsagesCallback &&showUsagesCallback) const = 0;
    virtual bool isRefactoringEngineAvailable() const { return true; }
};

} // namespace CppTools
