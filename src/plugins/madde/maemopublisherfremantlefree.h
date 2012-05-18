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
#ifndef MAEMOPUBLISHERFREMANTLEFREE_H
#define MAEMOPUBLISHERFREMANTLEFREE_H

#include <ssh/sshconnection.h>

#include <QObject>
#include <QProcess>

namespace ProjectExplorer {
class Project;
}

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
}

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace Madde {
namespace Internal {

class MaemoPublisherFremantleFree : public QObject
{
    Q_OBJECT
public:
    enum OutputType {
        StatusOutput, ErrorOutput, ToolStatusOutput, ToolErrorOutput
    };

    explicit MaemoPublisherFremantleFree(const ProjectExplorer::Project *project,
        QObject *parent = 0);
    ~MaemoPublisherFremantleFree();

    void publish();
    void cancel();

    void setBuildConfiguration(const Qt4ProjectManager::Qt4BuildConfiguration *buildConfig) { m_buildConfig = buildConfig; }
    void setDoUpload(bool doUpload) { m_doUpload = doUpload; }
    void setSshParams(const QString &hostName, const QString &userName,
        const QString &keyFile, const QString &remoteDir);

    QString resultString() const { return m_resultString; }

signals:
    void progressReport(const QString &text,
        MaemoPublisherFremantleFree::OutputType = StatusOutput);
    void finished();

private slots:
    void handleProcessFinished();
    void handleProcessStdOut();
    void handleProcessStdErr();
    void handleProcessError(QProcess::ProcessError error);
    void handleScpStarted();
    void handleConnectionError();
    void handleUploadJobFinished(int exitStatus);
    void handleScpStdOut(const QByteArray &output);

private:
    enum State {
        Inactive, CopyingProjectDir, RunningQmake, RunningMakeDistclean,
        BuildingPackage, StartingScp, PreparingToUploadFile, UploadingFile
    };

    void setState(State newState);
    void createPackage();
    void uploadPackage();
    bool copyRecursively(const QString &srcFilePath,
        const QString &tgtFilePath);
    bool fixNewlines();
    void handleProcessFinished(bool failedToStart);
    void runDpkgBuildPackage();
    QString tmpDirContainer() const;
    void prepareToSendFile();
    void sendFile();
    void finishWithFailure(const QString &progressMsg, const QString &resultMsg);
    bool updateDesktopFiles(QString *error = 0) const;
    bool addOrReplaceDesktopFileValue(QByteArray &fileContent,
        const QByteArray &key, const QByteArray &newValue) const;
    QStringList findProblems() const;

    const ProjectExplorer::Project * const m_project;
    bool m_doUpload;
    const Qt4ProjectManager::Qt4BuildConfiguration *m_buildConfig;
    State m_state;
    QString m_tmpProjectDir;
    QProcess *m_process;
    QSsh::SshConnectionParameters m_sshParams;
    QString m_remoteDir;
    QSsh::SshRemoteProcessRunner *m_uploader;
    QByteArray m_scpOutput;
    QList<QString> m_filesToUpload;
    QString m_resultString;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOPUBLISHERFREMANTLEFREE_H
