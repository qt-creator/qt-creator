/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CMAKEOPENPROJECTWIZARD_H
#define CMAKEOPENPROJECTWIZARD_H

#include <QtCore/QProcess>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QWizard>
#include <QtGui/QPlainTextEdit>

namespace Core {
    namespace Utils {
        class PathChooser;
    }
}

namespace CMakeProjectManager {
namespace Internal {

class CMakeManager;

class CMakeOpenProjectWizard : public QWizard
{
    Q_OBJECT
public:
    enum PageId {
        InSourcePageId,
        ShadowBuildPageId,
        XmlFileUpToDatePageId,
        CMakeRunPageId
    };

    // used at importing a project without a .user file
    CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory);
    // used to update if we have already a .user file
    CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory, const QStringList &needToCreate, const QStringList &needToUpdate);
    // used to change the build directory of one buildconfiguration
    CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory, const QString &oldBuildDirectory);

    virtual int nextId() const;
    QString buildDirectory() const;
    QString sourceDirectory() const;
    void setBuildDirectory(const QString &directory);
    CMakeManager *cmakeManager() const;
    QStringList arguments() const;
    void setArguments(const QStringList &args);
private:
    bool existsUpToDateXmlFile() const;
    bool hasInSourceBuild() const;
    CMakeManager *m_cmakeManager;
    QString m_buildDirectory;
    QString m_sourceDirectory;
    QStringList m_arguments;
    bool m_creatingCbpFiles;
};

class InSourceBuildPage : public QWizardPage
{
    Q_OBJECT
public:
    InSourceBuildPage(CMakeOpenProjectWizard *cmakeWizard);
private:
    CMakeOpenProjectWizard *m_cmakeWizard;
};


class XmlFileUpToDatePage : public QWizardPage
{
    Q_OBJECT
public:
    XmlFileUpToDatePage(CMakeOpenProjectWizard *cmakeWizard);
private:
    CMakeOpenProjectWizard *m_cmakeWizard;
};


class ShadowBuildPage : public QWizardPage
{
    Q_OBJECT
public:
    ShadowBuildPage(CMakeOpenProjectWizard *cmakeWizard, bool change = false);
private slots:
    void buildDirectoryChanged();
private:
    CMakeOpenProjectWizard *m_cmakeWizard;
    Core::Utils::PathChooser *m_pc;
};

class CMakeRunPage : public QWizardPage
{
    Q_OBJECT
public:
    enum Mode { Initial, Update, Recreate, Change };
    CMakeRunPage(CMakeOpenProjectWizard *cmakeWizard, Mode mode = Initial, const QString &buildDirectory = QString());

    virtual void initializePage();
    virtual void cleanupPage();
    virtual bool isComplete() const;
private slots:
    void runCMake();
    void cmakeFinished();
    void cmakeReadyRead();
private:
    void initWidgets();
    CMakeOpenProjectWizard *m_cmakeWizard;
    QPlainTextEdit *m_output;
    QPushButton *m_runCMake;
    QProcess *m_cmakeProcess;
    QLineEdit *m_argumentsLineEdit;
    QLabel *m_descriptionLabel;
    bool m_complete;
    Mode m_mode;
    QString m_buildDirectory;
};

}
}

#endif // CMAKEOPENPROJECTWIZARD_H
