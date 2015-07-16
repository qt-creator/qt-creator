/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLDESIGNER_PUPPETCREATOR_H
#define QMLDESIGNER_PUPPETCREATOR_H

#include <QString>
#include <QProcessEnvironment>

#include <designersettings.h>

#include <coreplugin/id.h>

namespace ProjectExplorer {
class Kit;
}

namespace QmlDesigner {

class PuppetBuildProgressDialog;
class Model;

class PuppetCreator
{
public:
    enum PuppetType {
        FallbackPuppet,
        UserSpacePuppet
    };
    enum QmlPuppetVersion {
        Qml1Puppet,
        Qml2Puppet
    };

    PuppetCreator(ProjectExplorer::Kit *kit, const QString &qtCreatorVersion, const Model *model, QmlPuppetVersion puppetVersion);
    ~PuppetCreator();

    void createPuppetExecutableIfMissing();

    QProcess *createPuppetProcess(const QString &puppetMode,
                                  const QString &socketToken,
                                  QObject *handlerObject,
                                  const char *outputSlot,
                                  const char *finishSlot) const;

    QString compileLog() const;

    void setQrcMappingString(const QString qrcMapping);

    static QString defaultPuppetToplevelBuildDirectory();
    static QString defaultPuppetFallbackDirectory();
protected:
    bool build(const QString &qmlPuppetProjectFilePath) const;

    void createQml1PuppetExecutableIfMissing();
    void createQml2PuppetExecutableIfMissing();

    QString qmlPuppetToplevelBuildDirectory() const;
    QString qmlPuppetFallbackDirectory() const;
    QString qmlPuppetDirectory(PuppetType puppetPathType) const;
    QString qml2PuppetPath(PuppetType puppetType) const;
    QString qmlPuppetPath(PuppetType puppetPathType) const;

    bool startBuildProcess(const QString &buildDirectoryPath,
                           const QString &command,
                           const QStringList &processArguments = QStringList(),
                           PuppetBuildProgressDialog *progressDialog = 0) const;
    static QString puppetSourceDirectoryPath();
    static QString qml2PuppetProjectFile();
    static QString qmlPuppetProjectFile();

    bool checkPuppetIsReady(const QString &puppetPath) const;
    bool checkQml2PuppetIsReady() const;
    bool checkQmlpuppetIsReady() const;
    bool qtIsSupported() const;
    static bool checkPuppetVersion(const QString &qmlPuppetPath);
    QProcess *puppetProcess(const QString &puppetPath,
                            const QString &workingDirectory,
                            const QString &puppetMode,
                            const QString &socketToken,
                            QObject *handlerObject,
                            const char *outputSlot,
                            const char *finishSlot) const;

    QProcessEnvironment processEnvironment() const;

    QString buildCommand() const;
    QString qmakeCommand() const;

    QByteArray qtHash() const;
    QDateTime qtLastModified() const;
    QDateTime puppetSourceLastModified() const;

    bool useOnlyFallbackPuppet() const;

private:
    QString m_qtCreatorVersion;
    mutable QString m_compileLog;
    ProjectExplorer::Kit *m_kit;
    PuppetType m_availablePuppetType;
    static QHash<Core::Id, PuppetType> m_qml1PuppetForKitPuppetHash;
    static QHash<Core::Id, PuppetType> m_qml2PuppetForKitPuppetHash;
    const Model *m_model;
    const DesignerSettings m_designerSettings;
    QString m_qrcMapping;
    QmlPuppetVersion m_puppetVersion;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_PUPPETCREATOR_H
