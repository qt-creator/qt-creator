/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/
#include "clangstaticanalyzerpathchooser.h"

#include "clangstaticanalyzerutils.h"

namespace ClangStaticAnalyzer {
namespace Internal {

PathChooser::PathChooser(QWidget *parent) : Utils::PathChooser(parent)
{
    setExpectedKind(Utils::PathChooser::ExistingCommand);
    setHistoryCompleter(QLatin1String("ClangStaticAnalyzer.ClangCommand.History"));
    setPromptDialogTitle(tr("Clang Command"));
}

bool PathChooser::validatePath(const QString &path, QString *errorMessage)
{
    if (!Utils::PathChooser::validatePath(path, errorMessage))
        return false;
    return isClangExecutableUsable(fileName().toString(), errorMessage);
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
