/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef RPMMANAGER_H
#define RPMMANAGER_H

#include <coreplugin/id.h>
#include <utils/fileutils.h>

#include <QObject>
#include <QHash>

namespace Utils { class FileSystemWatcher; }
namespace ProjectExplorer { class Target; }
namespace Qt4ProjectManager { class Qt4BuildConfiguration; }

namespace Madde {
namespace Internal {
class MaddePlugin;

class RpmManager : public QObject
{
    Q_OBJECT
public:
    ~RpmManager();

    static RpmManager *instance();

    // ref counted:
    void monitor(const Utils::FileName &specFile);
    void ignore(const Utils::FileName &specFile);

    static QString projectVersion(const Utils::FileName &spec, QString *error = 0);
    static bool setProjectVersion(const Utils::FileName &spec, const QString &version, QString *error = 0);
    static QString packageName(const Utils::FileName &spec);
    static bool setPackageName(const Utils::FileName &spec, const QString &packageName);
    static QString shortDescription(const Utils::FileName &spec);
    static bool setShortDescription(const Utils::FileName &spec, const QString &description);

    static Utils::FileName packageFileName(const Utils::FileName &spec, ProjectExplorer::Target *t);

    static Utils::FileName specFile(ProjectExplorer::Target *target);

signals:
    void specFileChanged(const Utils::FileName &spec);

private slots:
    void specFileWasChanged(const QString &path);

private:
    explicit RpmManager(QObject *parent = 0);

    Utils::FileSystemWatcher *m_watcher;
    QHash<Utils::FileName, int> m_watches;
    static RpmManager *m_instance;

    friend class MaddePlugin;
};

} // namespace Internal
} // namespace Madde

#endif // DEBIANMANAGER_H
