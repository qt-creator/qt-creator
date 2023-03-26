// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonprojectpage.h"

#include "jsonwizard.h"
#include "../projectexplorertr.h"

#include <coreplugin/documentmanager.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QVariant>

using namespace Utils;

namespace ProjectExplorer {

JsonProjectPage::JsonProjectPage(QWidget *parent) : ProjectIntroPage(parent)
{ }

void JsonProjectPage::initializePage()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);
    setFilePath(FilePath::fromString(wiz->stringValue(QLatin1String("InitialPath"))));

    setProjectName(uniqueProjectName(filePath().toString()));
}

bool JsonProjectPage::validatePage()
{
    if (isComplete() && useAsDefaultPath()) {
        // Store the path as default path for new projects if desired.
        Core::DocumentManager::setProjectsDirectory(filePath());
        Core::DocumentManager::setUseProjectsDirectory(true);
    }

    const FilePath target = filePath().pathAppended(projectName());

    wizard()->setProperty("ProjectDirectory", target.toString());
    wizard()->setProperty("TargetPath", target.toString());

    return Utils::ProjectIntroPage::validatePage();
}

QString JsonProjectPage::uniqueProjectName(const QString &path)
{
    const QDir pathDir(path);
    //: File path suggestion for a new project. If you choose
    //: to translate it, make sure it is a valid path name without blanks
    //: and using only ascii chars.
    const QString prefix = Tr::tr("untitled");
    for (unsigned i = 0; ; ++i) {
        QString name = prefix;
        if (i)
            name += QString::number(i);
        if (!pathDir.exists(name))
            return name;
    }
}

} // namespace ProjectExplorer
