#ifndef GENERICPROJECTWIZARD_H
#define GENERICPROJECTWIZARD_H

#include <coreplugin/basefilewizard.h>

#include <QtGui/QWizard>

QT_BEGIN_NAMESPACE
class QDir;
class QDirModel;
class QFileInfo;
class QListView;
class QModelIndex;
class QStringList;
class QTreeView;
QT_END_NAMESPACE

namespace Core {
namespace Utils {

class FileWizardPage;

} // namespace Utils
} // namespace Core

namespace GenericProjectManager {
namespace Internal {

class GenericProjectWizardDialog : public QWizard
{
    Q_OBJECT

public:
    GenericProjectWizardDialog(QWidget *parent = 0);
    virtual ~GenericProjectWizardDialog();

    QString path() const;
    QString projectName() const;

private Q_SLOTS:
    void updateFilesView(const QModelIndex &current,
                         const QModelIndex &previous);

protected:
    virtual void initializePage(int id);
    virtual bool validateCurrentPage();

private:
    int m_secondPageId;

    Core::Utils::FileWizardPage *m_firstPage;

    QTreeView *m_dirView;
    QDirModel *m_dirModel;

    QListView *m_filesView;
    QDirModel *m_filesModel;
};

class GenericProjectWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    GenericProjectWizard();
    virtual ~GenericProjectWizard();

    static Core::BaseFileWizardParameters parameters();

protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;

    virtual bool postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage);

    bool isValidDir(const QFileInfo &fileInfo) const;

    void getFileList(const QDir &dir, const QString &projectRoot,
                     const QStringList &suffixes,
                     QStringList *files,
                     QStringList *paths) const;
};

} // end of namespace Internal
} // end of namespace GenericProjectManager

#endif // GENERICPROJECTWIZARD_H
