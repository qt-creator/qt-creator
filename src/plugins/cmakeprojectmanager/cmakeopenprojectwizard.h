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
    CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory);
    CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory, const QStringList &needToCreate, const QStringList &needToUpdate);
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
    ShadowBuildPage(CMakeOpenProjectWizard *cmakeWizard);
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
    CMakeRunPage(CMakeOpenProjectWizard *cmakeWizard);
    CMakeRunPage(CMakeOpenProjectWizard *cmakeWizard, const QString &buildDirectory, bool update);
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
    QString m_buildDirectory;
};

}
}

#endif // CMAKEOPENPROJECTWIZARD_H
