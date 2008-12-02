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
#ifndef PROFILECACHE_H
#define PROFILECACHE_H

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class ProFile;
QT_END_NAMESPACE

namespace Core {
class IFile;
}

namespace Qt4ProjectManager {

//class Qt4Project;
class Qt4Manager;

namespace Internal {

class Qt4ProFileNode;
class ManagedProjectFile;

class ProFileCache : public QObject
{
    Q_OBJECT

public:
    ProFileCache(Qt4Manager *manager);
    ~ProFileCache();

    // does not result in a reparse or resolve
    void notifyModifiedChanged(ProFile *profile);

    // if external is true we need to reparse, it it's false we only resolve
    void notifyChanged(const QSet<ProFile *> &profiles, bool external = false);

    void updateDependencies(const QSet<ProFile *> &files, Qt4ProFileNode *projectNode);
    QStringList dependencies(Qt4ProFileNode *projectNode) const;

    ProFile *proFile(const QString &filename) const;
    QSet<ProFile *> proFiles(const QStringList &files) const;
    QSet<ProFile *> proFiles(Qt4ProFileNode *projectNode) const;

    Core::IFile *fileInterface(const QString &filename);
    inline Qt4Manager *manager() const { return m_manager; }

protected slots:
    void removeProject(QObject *obj);

private:
    void removeFile(const QString &profile, Qt4ProFileNode *projectNode);

    Qt4Manager *m_manager;
    QMultiMap<QString, Qt4ProFileNode *> m_projects;
    QMap<QString, ManagedProjectFile *> m_profiles;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILECACHE_H
