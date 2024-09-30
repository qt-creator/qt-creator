// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakemodificationfile.h"

#include "cocopluginconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>

namespace Coco::Internal {

using namespace ProjectExplorer;

static const char flagsSetting[] = "set(coverage_flags_list\n";
static const char tweaksLine[] = "# User-supplied settings follow here:\n";

CMakeModificationFile::CMakeModificationFile(Project *project)
    : m_project{project}
{}

QString CMakeModificationFile::fileName() const
{
    return QString(Constants::PROFILE_NAME) + ".cmake";
}

void CMakeModificationFile::setProjectDirectory(const Utils::FilePath &projectDirectory)
{
    setFilePath(projectDirectory.pathAppended(fileName()));
}

QStringList CMakeModificationFile::defaultModificationFile() const
{
    return contentOf(":/cocoplugin/files/cocoplugin.cmake");
}

void CMakeModificationFile::read()
{
    clear();
    QStringList file = currentModificationFile();

    {
        QStringList options;
        int i = file.indexOf(flagsSetting);
        if (i != -1) {
            i++;
            while (i < file.size() && !file[i].startsWith(')')) {
                options += file[i].trimmed();
                i++;
            }
        }
        setOptions(options);
    }
    {
        QStringList tweaks;
        int i = file.indexOf(tweaksLine);
        if (i != -1) {
            i++;
            while (i < file.size()) {
                tweaks += file[i].chopped(1);
                i++;
            }
        }
        setTweaks(tweaks);
    }
}

void CMakeModificationFile::write() const
{
    QFile out(nativePath());
    out.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream outStream(&out);
    for (QString &line : defaultModificationFile()) {
        outStream << line;

        if (line.startsWith(flagsSetting)) {
            for (const QString &option : options()) {
                QString line = "    " + option + '\n';
                outStream << line;
            }
        }
    }
    for (const QString &line : tweaks())
        outStream << line << "\n";

    out.close();
}

} // namespace Coco::Internal
