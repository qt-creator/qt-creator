/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLDESIGNER_PUPPETCREATOR_H
#define QMLDESIGNER_PUPPETCREATOR_H

#include <QString>
#include <QProcessEnvironment>

namespace ProjectExplorer {
class Kit;
}

namespace QmlDesigner {

class PuppetBuildProgressDialog;

class PuppetCreator
{
    enum Puppetype {
        FallbackPuppet,
        UserSpacePuppet
    };

public:
    enum QmlPuppetVersion {
        Qml1Puppet,
        Qml2Puppet
    };

    PuppetCreator(ProjectExplorer::Kit *kit, const QString &qtCreatorVersion);
    ~PuppetCreator();

    QProcess *createPuppetProcess(QmlPuppetVersion puppetVersion,
                                  const QString &puppetMode,
                                  const QString &socketToken,
                                  QObject *handlerObject,
                                  const char *outputSlot,
                                  const char *finishSlot) const;

    QString compileLog() const;

protected:
    bool build(const QString &qmlPuppetProjectFilePath) const;

    QString qmlpuppetDirectory(Puppetype puppetPathType) const;
    QString qmlpuppetFallbackDirectory() const;
    QString qml2puppetPath(Puppetype puppetType) const;
    QString qmlpuppetPath(Puppetype puppetPathType) const;

    bool startBuildProcess(const QString &buildDirectoryPath,
                           const QString &command,
                           const QStringList &processArguments = QStringList(),
                           PuppetBuildProgressDialog *progressDialog = 0) const;
    static QString puppetSourceDirectoryPath();
    static QString qml2puppetProjectFile();
    static QString qmlpuppetProjectFile();

    bool checkPuppetIsReady(const QString &puppetPath) const;
    bool checkQml2puppetIsReady() const;
    bool checkQmlpuppetIsReady() const;
    bool qtIsSupported() const;
    static bool checkPuppetVersion(const QString &qmlPuppetPath);
    QProcess *puppetProcess(const QString &puppetPath,
                            const QString &puppetMode,
                            const QString &socketToken,
                            QObject *handlerObject,
                            const char *outputSlot,
                            const char *finishSlot) const;

    QProcess *qmlpuppetProcess(const QString &puppetMode,
                               const QString &socketToken,
                               QObject *handlerObject,
                               const char *outputSlot,
                               const char *finishSlot) const;

    QProcess *qml2puppetProcess(const QString &puppetMode,
                                const QString &socketToken,
                                QObject *handlerObject,
                                const char *outputSlot,
                                const char *finishSlot) const;

    QProcessEnvironment processEnvironment() const;

    QString buildCommand() const;
    QString qmakeCommand() const;

    QByteArray qtHash() const;
    QDateTime qtLastModified() const;

private:
    QString m_qtCreatorVersion;
    mutable QString m_compileLog;
    ProjectExplorer::Kit *m_kit;
    static bool m_useOnlyFallbackPuppet;

};

} // namespace QmlDesigner

#endif // QMLDESIGNER_PUPPETCREATOR_H
