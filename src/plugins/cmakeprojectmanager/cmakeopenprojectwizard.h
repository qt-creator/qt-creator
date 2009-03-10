#ifndef CMAKEOPENPROJECTWIZARD_H
#define CMAKEOPENPROJECTWIZARD_H

#include <QtCore/QProcess>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>
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
    virtual void cleanupPage();
    virtual bool isComplete() const;
private slots:
    void runCMake();
    void cmakeFinished();
    void cmakeReadyRead();
private:
    CMakeOpenProjectWizard *m_cmakeWizard;
    QPlainTextEdit *m_output;
    QPushButton *m_runCMake;
    QProcess *m_cmakeProcess;
    QLineEdit *m_argumentsLineEdit;
    bool m_complete;
};

}
}

#endif // CMAKEOPENPROJECTWIZARD_H
