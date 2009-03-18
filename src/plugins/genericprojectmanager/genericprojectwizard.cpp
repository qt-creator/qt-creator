#include "genericprojectwizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/pathchooser.h>

#include <QtCore/QDir>
#include <QtCore/QtDebug>

#include <QtGui/QWizard>
#include <QtGui/QFormLayout>
#include <QtGui/QListView>
#include <QtGui/QTreeView>
#include <QtGui/QDirModel>

using namespace GenericProjectManager::Internal;
using namespace Core::Utils;

namespace {

class DirModel: public QDirModel
{
public:
    DirModel(QObject *parent)
        : QDirModel(parent)
    { setFilter(QDir::Dirs | QDir::NoDotAndDotDot); }

    virtual ~DirModel()
    { }

public:
    virtual int columnCount(const QModelIndex &) const
    { return 1; }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const
    { return QDirModel::flags(index) | Qt::ItemIsUserCheckable; }

    virtual QVariant data(const QModelIndex &index, int role) const
    {
        if (index.column() == 0 && role == Qt::CheckStateRole) {
            if (_selectedPaths.contains(index))
                return Qt::Checked;

            return Qt::Unchecked;
        }

        return QDirModel::data(index, role);
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (index.column() == 0 && role == Qt::CheckStateRole) {
            if (value.toBool())
                _selectedPaths.insert(index);
            else
                _selectedPaths.remove(index);

            return true;
        }

        return QDirModel::setData(index, value, role);
    }

    void clearSelectedPaths()
    { _selectedPaths.clear(); }

    QSet<QString> selectedPaths() const
    {
        QSet<QString> paths;

        foreach (const QModelIndex &index, _selectedPaths)
            paths.insert(filePath(index));

        return paths;
    }

private:
    QSet<QModelIndex> _selectedPaths;
};

} // end of anonymous namespace


//////////////////////////////////////////////////////////////////////////////
// GenericProjectWizardDialog
//////////////////////////////////////////////////////////////////////////////


GenericProjectWizardDialog::GenericProjectWizardDialog(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Import Existing Project"));

    // first page
    QWizardPage *firstPage = new QWizardPage;
    firstPage->setTitle(tr("Project"));

    QFormLayout *layout = new QFormLayout(firstPage);
    _pathChooser = new PathChooser;
    layout->addRow(tr("Source Directory:"), _pathChooser);

    _firstPageId = addPage(firstPage);

#if 0
    // second page
    QWizardPage *secondPage = new QWizardPage;
    secondPage->setTitle(tr("Second Page Title"));

    QFormLayout *secondPageLayout = new QFormLayout(secondPage);

    _dirView = new QTreeView;
    _dirModel = new DirModel(this);
    _dirView->setModel(_dirModel);

    Core::ICore *core = Core::ICore::instance();
    Core::MimeDatabase *mimeDatabase = core->mimeDatabase();

    const QStringList suffixes = mimeDatabase->suffixes();

    QStringList nameFilters;
    foreach (const QString &suffix, suffixes) {
        QString nameFilter;
        nameFilter.append(QLatin1String("*."));
        nameFilter.append(suffix);
        nameFilters.append(nameFilter);
    }

    _filesView = new QListView;
    _filesModel = new QDirModel(this);
    _filesModel->setNameFilters(nameFilters);
    _filesModel->setFilter(QDir::Files);

    connect(_dirView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateFilesView(QModelIndex,QModelIndex)));

    secondPageLayout->addRow(_dirView);
    secondPageLayout->addRow(_filesView);

    _secondPageId = addPage(secondPage);
#endif
}

GenericProjectWizardDialog::~GenericProjectWizardDialog()
{ }

QString GenericProjectWizardDialog::path() const
{ return _pathChooser->path(); }

void GenericProjectWizardDialog::updateFilesView(const QModelIndex &current,
                                                 const QModelIndex &)
{
    if (! current.isValid())
        _filesView->setModel(0);

    else {
        const QString selectedPath = _dirModel->filePath(current);

        if (! _filesView->model())
            _filesView->setModel(_filesModel);

        _filesView->setRootIndex(_filesModel->index(selectedPath));
    }
}

void GenericProjectWizardDialog::initializePage(int id)
{
#if 0
    if (id == _secondPageId) {
        using namespace Core::Utils;

        const QString projectPath = _pathChooser->path();

        QDirModel *dirModel = qobject_cast<QDirModel *>(_dirView->model());
        _dirView->setRootIndex(dirModel->index(projectPath));
    }
#endif
}

bool GenericProjectWizardDialog::validateCurrentPage()
{
    using namespace Core::Utils;

    if (currentId() == _firstPageId) {
        return ! _pathChooser->path().isEmpty();

    } else if (currentId() == _secondPageId) {
        return true;
    }

    return QWizard::validateCurrentPage();
}

GenericProjectWizard::GenericProjectWizard()
    : Core::BaseFileWizard(parameters())
{ }

GenericProjectWizard::~GenericProjectWizard()
{ }

Core::BaseFileWizardParameters GenericProjectWizard::parameters()
{
    static Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(":/wizards/images/console.png"));
    parameters.setName(tr("Existing Project"));
    parameters.setDescription(tr("Import Existing Project"));
    parameters.setCategory(QLatin1String("Projects"));
    parameters.setTrCategory(tr("Projects"));
    return parameters;
}

QWizard *GenericProjectWizard::createWizardDialog(QWidget *parent,
                                                  const QString &defaultPath,
                                                  const WizardPageList &extensionPages) const
{
    GenericProjectWizardDialog *wizard = new GenericProjectWizardDialog(parent);
    setupWizard(wizard);

    foreach (QWizardPage *p, extensionPages)
        wizard->addPage(p);

    return wizard;
}

void GenericProjectWizard::getFileList(const QDir &dir, const QString &projectRoot,
                                       const QStringList &suffixes,
                                       QStringList *files, QStringList *paths) const
{
    const QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files |
                                                         QDir::Dirs |
                                                         QDir::NoDotAndDotDot |
                                                         QDir::NoSymLinks);

    foreach (const QFileInfo &fileInfo, fileInfoList) {
        QString filePath = fileInfo.absoluteFilePath();
        filePath = filePath.mid(projectRoot.length() + 1);

        if (fileInfo.isDir() && isValidDir(fileInfo)) {
            getFileList(QDir(fileInfo.absoluteFilePath()), projectRoot,
                        suffixes, files, paths);

            if (! paths->contains(filePath))
                paths->append(filePath);
        }

        else if (suffixes.contains(fileInfo.suffix()))
            files->append(filePath);
    }
}

bool GenericProjectWizard::isValidDir(const QFileInfo &fileInfo) const
{
    const QString fileName = fileInfo.fileName();
    const QString suffix = fileInfo.suffix();

    if (fileName.startsWith(QLatin1Char('.')))
        return false;

    else if (fileName == QLatin1String("CVS"))
        return false;    

    // ### user include/exclude

    return true;
}

Core::GeneratedFiles GenericProjectWizard::generateFiles(const QWizard *w,
                                                         QString *errorMessage) const
{
    const GenericProjectWizardDialog *wizard = qobject_cast<const GenericProjectWizardDialog *>(w);
    const QString projectPath = wizard->path();
    const QDir dir(projectPath);
    const QString projectName = QFileInfo(projectPath).baseName();
    const QString creatorFileName = QFileInfo(dir, projectName + QLatin1String(".creator")).absoluteFilePath();
    const QString filesFileName = QFileInfo(dir, projectName + QLatin1String(".files")).absoluteFilePath();
    const QString includesFileName = QFileInfo(dir, projectName + QLatin1String(".includes")).absoluteFilePath();
    const QString configFileName = QFileInfo(dir, projectName + QLatin1String(".config")).absoluteFilePath();

    Core::ICore *core = Core::ICore::instance();
    Core::MimeDatabase *mimeDatabase = core->mimeDatabase();

    const QStringList suffixes = mimeDatabase->suffixes();

    QStringList sources, paths;
    getFileList(dir, projectPath, suffixes, &sources, &paths);

    Core::MimeType headerTy = mimeDatabase->findByType(QLatin1String("text/x-chdr"));

    QStringList nameFilters;
    foreach (const QRegExp &rx, headerTy.globPatterns())
        nameFilters.append(rx.pattern());

    QStringList includePaths;
    foreach (const QString &path, paths) {
        QFileInfo fileInfo(dir, path);
        QDir thisDir(fileInfo.absoluteFilePath());

        if (! thisDir.entryList(nameFilters, QDir::Files).isEmpty())
            includePaths.append(path);
    }

    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(QLatin1String("[General]\n"));

    Core::GeneratedFile generatedFilesFile(filesFileName);
    generatedFilesFile.setContents(sources.join(QLatin1String("\n")));

    Core::GeneratedFile generatedIncludesFile(includesFileName);
    generatedIncludesFile.setContents(includePaths.join(QLatin1String("\n")));

    Core::GeneratedFile generatedConfigFile(configFileName);
    generatedConfigFile.setContents(QLatin1String("// ADD PREDEFINED MACROS HERE!\n"));

    Core::GeneratedFiles files;
    files.append(generatedFilesFile);
    files.append(generatedIncludesFile);
    files.append(generatedConfigFile);
    files.append(generatedCreatorFile);

    return files;
}

bool GenericProjectWizard::postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage)
{
    // Post-Generate: Open the project
    const QString proFileName = l.back().path();
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFileName)) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

