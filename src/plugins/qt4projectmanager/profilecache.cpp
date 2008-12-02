/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "qt4project.h"
#include "qt4nodes.h"
#include "qt4projectmanager.h"
#include "profilecache.h"
#include "proparser/prowriter.h"
#include "proitems.h"
#include "qt4projectmanagerconstants.h"

#include <coreplugin/filemanager.h>
#include <utils/reloadpromptutils.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtGui/QMainWindow>
#include <QtCore/QDir>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
bool debug = false;
}

namespace Qt4ProjectManager {
namespace Internal {

class ManagedProjectFile
    : public Core::IFile
{
    Q_OBJECT

public:
    ManagedProjectFile(ProFileCache *parent, ProFile *profile);
    virtual ~ManagedProjectFile();

    ProFile *profile();
    void setProfile(ProFile *profile);
    bool isOutOfSync() { return m_outOfSync; }
    void setOutOfSync(bool state) { m_outOfSync = state; }

    // Core::IFile
    bool save(const QString &fileName = QString());
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;

    virtual QString mimeType() const;

    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;

    void modified(ReloadBehavior *behavior);

signals:
    void changed();

private:
    const QString m_mimeType;
    ProFile *m_profile;
    ProFileCache *m_cache;
    bool m_outOfSync;
};

} // namespace Internal
} // namespace Qt4ProFileNodemanager

ManagedProjectFile::ManagedProjectFile(ProFileCache *parent, ProFile *profile) :
    Core::IFile(parent),
    m_mimeType(QLatin1String(Qt4ProjectManager::Constants::PROFILE_MIMETYPE)),
    m_profile(profile),
    m_cache(parent),
    m_outOfSync(false)
{

}

ManagedProjectFile::~ManagedProjectFile()
{
    delete m_profile;
}

ProFile *ManagedProjectFile::profile()
{
    return m_profile;
}

void ManagedProjectFile::setProfile(ProFile *profile)
{
    m_outOfSync = false;
    if (profile == m_profile)
        return;
    delete m_profile;
    m_profile = profile;
}

bool ManagedProjectFile::save(const QString &)
{
    if (!m_profile)
        return false;

    ProWriter pw;
    bool ok = pw.write(m_profile, m_profile->fileName());
    m_profile->setModified(false);
    m_cache->notifyModifiedChanged(m_profile);

    return ok;
}

QString ManagedProjectFile::fileName() const
{
    return QDir::cleanPath(m_profile->fileName());
}

QString ManagedProjectFile::defaultPath() const
{
    QFileInfo fi(fileName());
    return fi.absolutePath();
}

QString ManagedProjectFile::suggestedFileName() const
{
    QFileInfo fi(fileName());
    return fi.fileName();
}

bool ManagedProjectFile::isModified() const
{
    return m_profile->isModified();
}

bool ManagedProjectFile::isReadOnly() const
{
    QFileInfo fi(fileName());
    return !fi.isWritable();
}

bool ManagedProjectFile::isSaveAsAllowed() const
{
    return false;
}

void ManagedProjectFile::modified(ReloadBehavior *behavior)
{

    switch (*behavior) {
    case Core::IFile::ReloadNone:
    case Core::IFile::ReloadPermissions:
        return;
    case Core::IFile::ReloadAll:
        m_cache->notifyChanged(QSet<ProFile *>() << m_profile, true);
        return;
    case Core::IFile::AskForReload:
        break;
    }

    const QString prompt = tr("The project file %1 has changed outside Qt Creator. Do you want to reload it?").arg(m_profile->fileName());
    switch (const Core::Utils::ReloadPromptAnswer answer = Core::Utils::reloadPrompt(tr("File Changed"), prompt, m_cache->manager()->core()->mainWindow())) {
    case Core::Utils::ReloadAll:
    case Core::Utils::ReloadCurrent:
        m_cache->notifyChanged(QSet<ProFile *>() << m_profile, true);
        if (answer == Core::Utils::ReloadAll)
            *behavior = Core::IFile::ReloadAll;
        break;
    case Core::Utils::ReloadSkipCurrent:
        break;
    case Core::Utils::ReloadNone:
        *behavior = Core::IFile::ReloadNone;
        break;
    }
}

QString ManagedProjectFile::mimeType() const
{
    return m_mimeType;
}


#include "profilecache.moc"

ProFileCache::ProFileCache(Qt4Manager *manager)
    : QObject(manager)
{
    m_manager = manager;
}

ProFileCache::~ProFileCache()
{
    qDeleteAll(m_profiles.values());
    m_profiles.clear();
    m_projects.clear();
}

void ProFileCache::notifyModifiedChanged(ProFile *profile)
{
    QList<Qt4ProFileNode *> pros = m_projects.values(profile->fileName());
    for (int i=0; i<pros.count(); ++i) {
        Qt4ProFileNode *pro = pros.at(i);
        pro->update();
    }
}

void ProFileCache::notifyChanged(const QSet<ProFile *> &profiles, bool external)
{
    QList<Qt4ProFileNode *> notifyProjects;

    foreach (ProFile *profile, profiles) {
        QList<Qt4ProFileNode *> pros = m_projects.values(profile->fileName());

        if (external) {
            ManagedProjectFile *file = m_profiles.value(profile->fileName());
            if (file)
                file->setOutOfSync(true);
        }

        QList<Qt4ProFileNode *>::const_iterator i = pros.constBegin();
        for (; i != pros.constEnd(); ++i) {
            if (!notifyProjects.contains(*i))
                notifyProjects << *i;
        }
    }

    QList<Qt4ProFileNode *>::const_iterator i = notifyProjects.constBegin();
    while (i != notifyProjects.constEnd()) {
        (*i)->update();
        ++i;
    }
}

void ProFileCache::updateDependencies(const QSet<ProFile *> &files, Qt4ProFileNode *project)
{
    // just remove and add files that have changed
    const QSet<QString> &oldFiles = m_projects.keys(project).toSet();
    QSet<QString> newFiles;

    QList<Core::IFile *> addedFiles;
    foreach (ProFile *file, files) {
        newFiles << file->fileName();
        if (!m_profiles.contains(file->fileName())) {
            ManagedProjectFile *profile = new ManagedProjectFile(this, file);
            m_profiles.insert(file->fileName(), profile);
            if (debug)
                qDebug() << "ProFileCache - inserting file " << file->fileName();
            addedFiles << profile;
        } else {
            ManagedProjectFile *profile = m_profiles.value(file->fileName());
            profile->setProfile(file);
        }
    }
    m_manager->core()->fileManager()->addFiles(addedFiles);

    if (oldFiles.isEmpty()) {
        connect(project, SIGNAL(destroyed(QObject *)),
            this, SLOT(removeProject(QObject *)));
    }

    foreach (const QString &profile, (oldFiles - newFiles).toList()) {
        removeFile(profile, project);
    }

    foreach (const QString &profile, (newFiles - oldFiles).toList()) {
        m_projects.insertMulti(profile, project);
    }
}

QStringList ProFileCache::dependencies(Qt4ProFileNode *project) const
{
    return m_projects.keys(project);
}

ProFile *ProFileCache::proFile(const QString &file) const
{
    QSet<ProFile *> profiles = proFiles(QStringList(file));
    if (profiles.isEmpty())
        return 0;
    return profiles.toList().first();
}

QSet<ProFile *> ProFileCache::proFiles(const QStringList &files) const
{
    QSet<ProFile *> results;
    foreach (const QString &file, files) {
        ManagedProjectFile *managedFile = m_profiles.value(file, 0);
        if (managedFile && !managedFile->isOutOfSync()) {
                results << managedFile->profile();
        }
    }
    return results;
}

QSet<ProFile *> ProFileCache::proFiles(Qt4ProFileNode *project) const
{
    return proFiles(m_projects.keys(project));
}

Core::IFile *ProFileCache::fileInterface(const QString &file)
{
    return m_profiles.value(file);
}

void ProFileCache::removeFile(const QString &file, Qt4ProFileNode *proj)
{
    QList<Qt4ProFileNode*> projects = m_projects.values(file);
    m_projects.remove(file);
    projects.removeAll(proj);
    if (!projects.isEmpty()) {
        foreach (Qt4ProFileNode *p, projects) {
            m_projects.insert(file, p);
        }
    } else {
        ManagedProjectFile *mp = m_profiles.value(file, 0);
        if (debug)
            qDebug() << "ProFileCache - removing file " << file;
        m_manager->core()->fileManager()->removeFile(mp);
        m_profiles.remove(file);
        delete mp;
    }
}

void ProFileCache::removeProject(QObject *obj)
{
    // Cannot use qobject_cast here because
    // it is triggered by destroyed signal
    Qt4ProFileNode *proj = static_cast<Qt4ProFileNode*>(obj);
    QStringList files = m_projects.keys(proj);
    foreach (const QString &file, files) {
        removeFile(file, proj);
    }
}
