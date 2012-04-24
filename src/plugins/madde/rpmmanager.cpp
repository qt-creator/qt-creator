/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "rpmmanager.h"

#include "maddedevice.h"
#include "maemoconstants.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"

#include <coreplugin/documentmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/qtprofileinformation.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

#include <QBuffer>
#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QProcess>

#include <QMessageBox>

// -----------------------------------------------------------------------
// Helpers:
// -----------------------------------------------------------------------

namespace {

const QLatin1String PackagingDirName("qtc_packaging");
const QByteArray NameTag("Name");
const QByteArray SummaryTag("Summary");
const QByteArray VersionTag("Version");
const QByteArray ReleaseTag("Release");

bool adaptTagValue(QByteArray &document, const QByteArray &fieldName,
                   const QByteArray &newFieldValue, bool caseSensitive)
{
    QByteArray adaptedLine = fieldName + ": " + newFieldValue;
    const QByteArray completeTag = fieldName + ':';
    const int lineOffset = caseSensitive ? document.indexOf(completeTag)
        : document.toLower().indexOf(completeTag.toLower());
    if (lineOffset == -1) {
        document.append(adaptedLine).append('\n');
        return true;
    }

    int newlineOffset = document.indexOf('\n', lineOffset);
    bool updated = false;
    if (newlineOffset == -1) {
        newlineOffset = document.length();
        adaptedLine += '\n';
        updated = true;
    }
    const int replaceCount = newlineOffset - lineOffset;
    if (!updated && document.mid(lineOffset, replaceCount) != adaptedLine)
        updated = true;
    if (updated)
        document.replace(lineOffset, replaceCount, adaptedLine);
    return updated;
}

QByteArray valueForTag(const Utils::FileName &spec, const QByteArray &tag, QString *error)
{
    Utils::FileReader reader;
    if (!reader.fetch(spec.toString(), error))
        return QByteArray();
    const QByteArray &content = reader.data();
    const QByteArray completeTag = tag.toLower() + ':';
    int index = content.toLower().indexOf(completeTag);
    if (index == -1)
        return QByteArray();
    index += completeTag.count();
    int endIndex = content.indexOf('\n', index);
    if (endIndex == -1)
        endIndex = content.count();
    return content.mid(index, endIndex - index).trimmed();
}

bool setValueForTag(const Utils::FileName &spec, const QByteArray &tag, const QByteArray &value, QString *error)
{
    Utils::FileReader reader;
    if (!reader.fetch(spec.toString(), error))
        return false;
    QByteArray content = reader.data();
    if (adaptTagValue(content, tag, value, false)) {
        Utils::FileSaver saver(spec.toString());
        saver.write(content);
        return saver.finalize(error);
    }
    return true;
}

} // namespace

namespace Madde {
namespace Internal {

// -----------------------------------------------------------------------
// RpmManager:
// -----------------------------------------------------------------------

RpmManager *RpmManager::m_instance = 0;

RpmManager::RpmManager(QObject *parent) :
    QObject(parent),
    m_watcher(new Utils::FileSystemWatcher(this))
{
    m_instance = this;

    m_watcher->setObjectName("Madde::RpmManager");
    connect(m_watcher, SIGNAL(fileChanged(QString)),
            this, SLOT(specFileWasChanged(QString)));
}

RpmManager::~RpmManager()
{ }

RpmManager *RpmManager::instance()
{
    return m_instance;
}

void RpmManager::monitor(const Utils::FileName &spec)
{
    QFileInfo fi = spec.toFileInfo();
    if (!fi.isFile())
        return;

    if (!m_watches.contains(spec)) {
        m_watches.insert(spec, 1);
        m_watcher->addFile(spec.toString(), Utils::FileSystemWatcher::WatchAllChanges);
    }
}

void RpmManager::ignore(const Utils::FileName &spec)
{
    int count = m_watches.value(spec, 0) - 1;
    if (count < 0)
        return;
    if (count > 0) {
        m_watches[spec] = 0;
    } else {
        m_watches.remove(spec);
        m_watcher->removeFile(spec.toString());
    }
}

QString RpmManager::projectVersion(const Utils::FileName &spec, QString *error)
{
    return QString::fromUtf8(valueForTag(spec, VersionTag, error));
}

bool RpmManager::setProjectVersion(const Utils::FileName &spec, const QString &version, QString *error)
{
    return setValueForTag(spec, VersionTag, version.toUtf8(), error);
}

QString RpmManager::packageName(const Utils::FileName &spec)
{
    return QString::fromUtf8(valueForTag(spec, NameTag, 0));
}

bool RpmManager::setPackageName(const Utils::FileName &spec, const QString &packageName)
{
    return setValueForTag(spec, NameTag, packageName.toUtf8(), 0);
}

QString RpmManager::shortDescription(const Utils::FileName &spec)
{
    return QString::fromUtf8(valueForTag(spec, SummaryTag, 0));
}

bool RpmManager::setShortDescription(const Utils::FileName &spec, const QString &description)
{
    return setValueForTag(spec, SummaryTag, description.toUtf8(), 0);
}

Utils::FileName RpmManager::packageFileName(const Utils::FileName &spec, ProjectExplorer::Target *t)
{
    QtSupport::BaseQtVersion *lqt = QtSupport::QtProfileInformation::qtVersion(t->profile());
    if (!lqt)
        return Utils::FileName();
    return Utils::FileName::fromString(packageName(spec)
                                       + QLatin1Char('-') + projectVersion(spec)
                                       + QLatin1Char('-') + QString::fromUtf8(valueForTag(spec, ReleaseTag, 0))
                                       + QLatin1Char('.') + MaemoGlobal::architecture(lqt->qmakeCommand().toString())
                                       + QLatin1String(".rpm"));
}

Utils::FileName RpmManager::specFile(ProjectExplorer::Target *target)
{
    Utils::FileName path = Utils::FileName::fromString(target->project()->projectDirectory());
    path.appendPath(PackagingDirName);
    Core::Id deviceType = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(target->profile());
    if (deviceType == Core::Id(MeeGoOsType))
        path.appendPath(QLatin1String("meego.spec"));
    else
        path.clear();
    return path;
}

void RpmManager::specFileWasChanged(const QString &path)
{
    Utils::FileName fn = Utils::FileName::fromString(path);
    QTC_ASSERT(m_watches.contains(fn), return);
    emit specFileChanged(fn);
}

} // namespace Internal
} // namespace Madde
