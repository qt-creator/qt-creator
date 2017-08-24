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

#include "clangsupportdebugutils.h"

#include "filecontainer.h"

#include <utf8string.h>

#include <QDir>
#include <QLoggingCategory>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>

namespace {

Q_LOGGING_CATEGORY(timersLog, "qtc.clangbackend.timers");

class DebugInspectionDir : public QTemporaryDir
{
public:
    DebugInspectionDir()
        : QTemporaryDir(QDir::tempPath() + QLatin1String("/qtc-clangsupport-XXXXXX"))
    {
        setAutoRemove(false); // Keep around for later inspection.
    }
};

class DebugInspectionFile : public QTemporaryFile
{
public:
    DebugInspectionFile(const QString &directoryPath,
                        const Utf8String &id,
                        const Utf8String &fileContent)
        : QTemporaryFile(directoryPath + QString::fromUtf8("/%1-XXXXXX").arg(id.toString()))
    {
        setAutoRemove(false); // Keep around for later inspection.
        m_isValid = open() && write(fileContent.constData(), fileContent.byteSize());
    }

    bool isValid() const
    {
        return m_isValid;
    }

private:
    bool m_isValid = false;
};

}

namespace ClangBackEnd {

Utf8String debugWriteFileForInspection(const Utf8String &fileContent, const Utf8String &id)
{
    static DebugInspectionDir debugInspectionDir;
    if (!debugInspectionDir.isValid())
        return Utf8String();

    DebugInspectionFile file(debugInspectionDir.path(), id, fileContent);
    if (file.isValid())
        return Utf8String::fromString(file.fileName());
    return Utf8String();
}

Utf8String debugId(const FileContainer &fileContainer)
{
    const Utf8String filePath = fileContainer.filePath();
    Utf8String id(Utf8StringLiteral("unsavedfilecontent-"));
    id.append(QFileInfo(filePath).fileName());
    return id;
}

VerboseScopeDurationTimer::VerboseScopeDurationTimer(const char *id)
    : m_id(id)
{
    if (timersLog().isDebugEnabled())
        m_timer.start();
}

VerboseScopeDurationTimer::~VerboseScopeDurationTimer()
{
    qCDebug(timersLog) << m_id << "needed" << m_timer.elapsed() << "ms";
}

} // namespace ClangBackEnd
