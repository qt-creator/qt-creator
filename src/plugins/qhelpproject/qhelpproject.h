/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef QHELPPROJECT_H
#define QHELPPROJECT_H

#include <projectexplorer/ProjectExplorerInterfaces>

#include <QtCore/QObject>

namespace QHelpProjectPlugin {
namespace Internal {

class QHelpProjectManager;
class QHelpProjectFolder;

class QHelpProject : public QObject,
    public ProjectExplorer::ProjectInterface
{
    Q_OBJECT
    Q_INTERFACES(ProjectExplorer::ProjectInterface
                 ProjectExplorer::ProjectItemInterface
                 QWorkbench::FileInterface)

public:
    QHelpProject(QHelpProjectManager *manager);
    ~QHelpProject();

    bool open(const QString &fileName);

    //QWorkbench::FileInterface
    bool save(const QString &fileName = QString());
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString fileFilter() const;
    QString fileExtension() const;

    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;

    void modified(QWorkbench::FileInterface::ReloadBehavior *behavior);

    //ProjectExplorer::ProjectInterface
    ProjectExplorer::IProjectManager *projectManager() const;

    QVariant projectProperty(const QString &key) const;
    void setProjectProperty(const QString &key, const QVariant &value) const;

    void executeProjectCommand(int cmd);
    bool supportsProjectCommand(int cmd);

    bool hasProjectProperty(ProjectProperty property) const;
    QList<QWorkbench::FileInterface *> dependencies();

    void setExtraApplicationRunArguments(const QStringList &args);
    void setCustomApplicationOutputHandler(QObject *handler);

    QStringList files(const QList<QRegExp> &fileMatcher);


    //ProjectExplorer::ProjectItemInterface
    ProjectItemKind kind() const;

    QString name() const;
    QIcon icon() const;

signals:
    void buildFinished(const QString &error);
    void changed();

private:
    QHelpProjectManager *m_manager;
    QHelpProjectFolder *m_filesFolder;
    QString m_projectFile;
    QString m_name;
    QStringList m_files;
};

} // namespace Internal
} // namespace QHelpProject

#endif // QHELPPROJECT_H
