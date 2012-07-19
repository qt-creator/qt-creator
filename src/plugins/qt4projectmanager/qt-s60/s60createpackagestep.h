/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef S60CREATEPACKAGESTEP_H
#define S60CREATEPACKAGESTEP_H

#include "ui_s60createpackagestep.h"

#include <projectexplorer/buildstep.h>
#include <qt4projectmanager/makestep.h>

#include <QMutex>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {
class CheckableMessageBox;
} // namespace Utils

namespace Qt4ProjectManager {
namespace Internal {

class S60CreatePackageParser;

class S60CreatePackageStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    explicit S60CreatePackageStepFactory(QObject *parent = 0);
    ~S60CreatePackageStepFactory();

    // used to show the list of possible additons to a target, returns a list of types
    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    // used to translate the types to names to display to the user
    QString displayNameForId(const Core::Id id) const;

    bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    // used to recreate the runConfigurations when restoring settings
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product);

    bool canHandle(ProjectExplorer::BuildStepList *parent) const;
};


class S60CreatePackageStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class S60CreatePackageStepFactory;

public:
    enum SigningMode {
        SignSelf = 0,
        SignCustom = 1,
        NotSigned = 2
    };

    explicit S60CreatePackageStep(ProjectExplorer::BuildStepList *bsl);
    virtual ~S60CreatePackageStep();

    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;

    QVariantMap toMap() const;

    SigningMode signingMode() const;
    void setSigningMode(SigningMode mode);
    QString customSignaturePath() const;
    void setCustomSignaturePath(const QString &path);
    QString customKeyPath() const;
    void setCustomKeyPath(const QString &path);
    QString passphrase() const   ;
    void setPassphrase(const QString &passphrase);
    QString keyId() const;
    void setKeyId(const QString &keyId);
    bool createsSmartInstaller() const;
    void setCreatesSmartInstaller(bool value);

    void resetPassphrases();

signals:
    void badPassphrase();
    void warnAboutPatching();

protected:
    S60CreatePackageStep(ProjectExplorer::BuildStepList *bsl, S60CreatePackageStep *bs);
    S60CreatePackageStep(ProjectExplorer::BuildStepList *bsl, const Core::Id id);
    bool fromMap(const QVariantMap &map);

    Qt4BuildConfiguration *qt4BuildConfiguration() const;

private slots:
    void packageWarningDialogDone();
    void packageDone(int, QProcess::ExitStatus);
    void processReadyReadStdOutput();
    void processReadyReadStdError();
    void checkForCancel();
    void definePassphrase();

    void packageWasPatched(const QString &, const QStringList &);
    void handleWarnAboutPatching();

private:
    void stdOutput(const QString &line);
    void stdError(const QString &line);

    void reportPackageStepIssue(const QString &message, bool isError );
    void setupProcess();
    bool createOnePackage();
    bool validateCustomSigningResources(const QStringList &capabilitiesInPro);

    QString generateKeyId(const QString &keyPath) const;
    QString loadPassphraseForKey(const QString &keyId);
    void savePassphraseForKey(const QString &keyId, const QString &passphrase);
    QString elucidatePassphrase(QByteArray obfuscatedPassphrase, const QString &key) const;
    QByteArray obfuscatePassphrase(const QString &passphrase, const QString &key) const;

    QStringList m_workingDirectories;

    QString m_makeCmd;
    Utils::Environment m_environment;
    QStringList m_args;

    void ctor_package();

    SigningMode m_signingMode;
    QString m_customSignaturePath;
    QString m_customKeyPath;
    QString m_passphrase;
    QString m_keyId;
    bool m_createSmartInstaller;
    ProjectExplorer::IOutputParser *m_outputParserChain;

    QProcess *m_process;
    QTimer *m_timer;
    QEventLoop *m_eventLoop;
    QFutureInterface<bool> *m_futureInterface;

    QWaitCondition m_waitCondition;
    QMutex m_mutex;

    bool m_cancel;

    QSettings *m_passphrases;
    S60CreatePackageParser *m_parser;
    QList<QPair<QString, QStringList> > m_packageChanges;

    bool m_suppressPatchWarningDialog;
    Utils::CheckableMessageBox *m_patchWarningDialog;
    bool m_isBuildWithSymbianSbsV2;
};

class S60CreatePackageStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    S60CreatePackageStepConfigWidget(S60CreatePackageStep *signStep);
    QString displayName() const;
    QString summaryText() const;

private slots:
    void updateUi();
    void updateFromUi();
    void resetPassphrases();
    void signatureChanged(QString certFile);
    void displayCertificateDetails();

private:
    S60CreatePackageStep *m_signStep;

    Ui::S60CreatePackageStepWidget m_ui;
};

} // Internal
} // Qt4ProjectManager

#endif // S60CREATEPACKAGESTEP_H
