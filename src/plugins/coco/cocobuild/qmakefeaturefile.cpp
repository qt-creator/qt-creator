// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakefeaturefile.h"

#include "cocopluginconstants.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace Coco::Internal {

static const char assignment[] = "COVERAGE_OPTIONS = \\\n";
static const char tweaksLine[] = "# User-supplied settings follow here:\n";

static void cutTail(QStringList &list)
{
    while (!list.isEmpty() && list.last().trimmed().isEmpty())
        list.removeLast();
}

QMakeFeatureFile::QMakeFeatureFile() {}

QString QMakeFeatureFile::fileName() const
{
    return QString(Constants::PROFILE_NAME) + ".prf";
}

void QMakeFeatureFile::setProjectDirectory(const Utils::FilePath &projectDirectory)
{
    setFilePath(projectDirectory.pathAppended(fileName()));
}

QString QMakeFeatureFile::fromFileLine(const QString &line) const
{
    return line.chopped(2).trimmed().replace("\\\"", "\"");
}

QString QMakeFeatureFile::toFileLine(const QString &option) const
{
    QString line = option.trimmed().replace("\"", "\\\"");
    return "    " + line + " \\\n";
}

void QMakeFeatureFile::read()
{
    clear();
    QStringList file = currentModificationFile();

    {
        QStringList options;
        int i = file.indexOf(assignment);
        if (i != -1) {
            i++;
            while (i < file.size() && file[i].endsWith("\\\n")) {
                options += fromFileLine(file[i]);
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

void QMakeFeatureFile::write() const
{
    QFile out(nativePath());
    out.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream outStream(&out);
    for (QString &line : defaultModificationFile()) {
        outStream << line;

        if (line.startsWith(assignment)) {
            for (const QString &option : options()) {
                QString line = toFileLine(option);
                if (!line.isEmpty())
                    outStream << line;
            }
        }
    }
    for (const QString &line : tweaks())
        outStream << line << "\n";

    out.close();
}

QStringList QMakeFeatureFile::defaultModificationFile() const
{
    return contentOf(":/cocoplugin/files/cocoplugin.prf");
}

} // namespace Coco::Internal
