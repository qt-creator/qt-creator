// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonfilepage.h"

#include "jsonwizard.h"

#include <utils/filepath.h>

using namespace Utils;

namespace ProjectExplorer {

JsonFilePage::JsonFilePage(QWidget *parent)
    : FileWizardPage(parent)
{
    setAllowDirectoriesInFileSelector(true);
}

void JsonFilePage::initializePage()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    if (!wiz)
        return;

    if (fileName().isEmpty())
        setFileName(wiz->stringValue(QLatin1String("InitialFileName")));
    if (filePath().isEmpty())
        setPath(wiz->stringValue(QLatin1String("InitialPath")));
    setDefaultSuffix(wiz->stringValue("DefaultSuffix"));
}

bool JsonFilePage::validatePage()
{
    if (filePath().isEmpty() || fileName().isEmpty())
        return false;

    const FilePath dir = filePath();
    if (dir.exists() && !dir.isDir())
        return false;

    const FilePath target = dir.resolvePath(fileName());

    wizard()->setProperty("TargetPath", target.toString());
    return true;
}

} // namespace ProjectExplorer
