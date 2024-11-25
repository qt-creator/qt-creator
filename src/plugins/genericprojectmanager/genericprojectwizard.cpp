// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericprojectwizard.h"

#include "genericprojectconstants.h"
#include "genericprojectmanagertr.h"

#include <coreplugin/basefilewizard.h>
#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>

#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/selectablefilesmodel.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/filewizardpage.h>
#include <utils/mimeutils.h>
#include <utils/wizard.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWizardPage>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace GenericProjectManager::Internal {

const char ConfigFileTemplate[] =
        "// Add predefined macros for your project here. For example:\n"
        "// #define THE_ANSWER 42\n";

class GenericProjectWizard;

class FilesSelectionWizardPage final : public QWizardPage
{
public:
    explicit FilesSelectionWizardPage(GenericProjectWizard *genericProjectWizard)
        : m_genericProjectWizardDialog(genericProjectWizard),
          m_filesWidget(new SelectableFilesWidget(this))
    {
        auto layout = new QVBoxLayout(this);

        layout->addWidget(m_filesWidget);
        m_filesWidget->enableFilterHistoryCompletion
            (ProjectExplorer::Constants::ADD_FILES_DIALOG_FILTER_HISTORY_KEY);
        m_filesWidget->setBaseDirEditable(false);
        connect(m_filesWidget, &SelectableFilesWidget::selectedFilesChanged,
                this, &FilesSelectionWizardPage::completeChanged);

        setProperty(SHORT_TITLE_PROPERTY, Tr::tr("Files"));
    }

    bool isComplete() const final
    {
        return m_filesWidget->hasFilesSelected();
    }

    void initializePage() final;

    void cleanupPage() final
    {
        m_filesWidget->cancelParsing();
    }

    FilePaths selectedFiles() const
    {
        return m_filesWidget->selectedFiles();
    }

    FilePaths selectedPaths() const
    {
        return m_filesWidget->selectedPaths();
    }

private:
    GenericProjectWizard *m_genericProjectWizardDialog;
    SelectableFilesWidget *m_filesWidget;
};

//////////////////////////////////////////////////////////////////////////////
//
// GenericProjectWizard
//
//////////////////////////////////////////////////////////////////////////////

class GenericProjectWizard final : public BaseFileWizard
{
    Q_OBJECT

public:
    GenericProjectWizard(const BaseFileWizardFactory *factory, QWidget *parent)
        : BaseFileWizard(factory, QVariantMap(), parent)
    {
        setWindowTitle(Tr::tr("Import Existing Project"));

        // first page
        m_firstPage = new FileWizardPage;
        m_firstPage->setTitle(Tr::tr("Project Name and Location"));
        m_firstPage->setFileNameLabel(Tr::tr("Project name:"));
        m_firstPage->setPathLabel(Tr::tr("Location:"));
        addPage(m_firstPage);

        // second page
        m_secondPage = new FilesSelectionWizardPage(this);
        m_secondPage->setTitle(Tr::tr("File Selection"));
        addPage(m_secondPage);
    }

    FilePath filePath() const
    {
        return m_firstPage->filePath();
    }

    void setFilePath(const FilePath &path)
    {
        m_firstPage->setFilePath(path);
    }

    FilePaths selectedFiles() const
    {
        return m_secondPage->selectedFiles();
    }

    FilePaths selectedPaths() const
    {
        return m_secondPage->selectedPaths();
    }

    QString projectName() const
    {
        return m_firstPage->fileName();
    }

    FileWizardPage *m_firstPage;
    FilesSelectionWizardPage *m_secondPage;
};

//////////////////////////////////////////////////////////////////////////////
//
// GenericProjectWizardFactory
//
//////////////////////////////////////////////////////////////////////////////

class GenericProjectWizardFactory final : public BaseFileWizardFactory
{
    Q_OBJECT

public:
    GenericProjectWizardFactory()
    {
        setSupportedProjectTypes({Constants::GENERICPROJECT_ID});
        setIcon(ProjectExplorer::Icons::WIZARD_IMPORT_AS_PROJECT.icon());
        setDisplayName(Tr::tr("Import Existing Project"));
        setId("Z.Makefile");
        setDescription(
            Tr::tr("Imports existing projects that do not use qmake, CMake, Qbs, Meson, or Autotools. "
                   "This allows you to use %1 as a code editor.")
                .arg(QGuiApplication::applicationDisplayName()));
        setCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY));
        setDisplayCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY_DISPLAY));
        setFlags(IWizardFactory::PlatformIndependent);
    }

protected:
    BaseFileWizard *create(QWidget *parent, const WizardDialogParameters &parameters) const final
    {
        auto wizard = new GenericProjectWizard(this, parent);
        wizard->setFilePath(parameters.defaultPath());
        const QList<QWizardPage *> pages = wizard->extensionPages();
        for (QWizardPage *p : pages)
            wizard->addPage(p);

        return wizard;
    }

    GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const final
    {
        Q_UNUSED(errorMessage)

        auto wizard = qobject_cast<const GenericProjectWizard *>(w);
        const FilePath projectPath = wizard->filePath();
        const QString projectName = wizard->projectName();
        const FilePath creatorFileName = projectPath.pathAppended(projectName + ".creator");
        const FilePath filesFileName = projectPath.pathAppended(projectName + ".files");
        const FilePath includesFileName = projectPath.pathAppended(projectName + ".includes");
        const FilePath configFileName = projectPath.pathAppended(projectName + ".config");
        const FilePath cxxflagsFileName = projectPath.pathAppended(projectName + ".cxxflags");
        const FilePath cflagsFileName = projectPath.pathAppended(projectName + ".cflags");
        const QStringList paths = Utils::transform(wizard->selectedPaths(), &FilePath::toString);

        MimeType headerTy = Utils::mimeTypeForName(QLatin1String("text/x-chdr"));

        QStringList nameFilters = headerTy.globPatterns();

        QStringList includePaths;
        const QDir dir(projectPath.toString());
        for (const QString &path : paths) {
            QFileInfo fileInfo(path);
            if (fileInfo.fileName() != "include")
                continue;
            QDir thisDir(fileInfo.absoluteFilePath());

            if (! thisDir.entryList(nameFilters, QDir::Files).isEmpty()) {
                QString relative = dir.relativeFilePath(path);
                if (relative.isEmpty())
                    relative = QLatin1Char('.');
                includePaths.append(relative);
            }
        }
        includePaths.append(QString()); // ensure newline at EOF

        GeneratedFile generatedCreatorFile(creatorFileName);
        generatedCreatorFile.setContents(QLatin1String("[General]\n"));
        generatedCreatorFile.setAttributes(GeneratedFile::OpenProjectAttribute);

        QStringList sources = Utils::transform(wizard->selectedFiles(), &FilePath::toString);
        for (int i = 0; i < sources.length(); ++i)
            sources[i] = dir.relativeFilePath(sources[i]);
        Utils::sort(sources);
        sources.append(QString()); // ensure newline at EOF

        GeneratedFile generatedFilesFile(filesFileName);
        generatedFilesFile.setContents(sources.join(QLatin1Char('\n')));

        GeneratedFile generatedIncludesFile(includesFileName);
        generatedIncludesFile.setContents(includePaths.join(QLatin1Char('\n')));

        GeneratedFile generatedConfigFile(configFileName);
        generatedConfigFile.setContents(QLatin1String(ConfigFileTemplate));

        GeneratedFile generatedCxxFlagsFile(cxxflagsFileName);
        generatedCxxFlagsFile.setContents(
            QLatin1String(Constants::GENERICPROJECT_CXXFLAGS_FILE_TEMPLATE));

        GeneratedFile generatedCFlagsFile(cflagsFileName);
        generatedCFlagsFile.setContents(QLatin1String(Constants::GENERICPROJECT_CFLAGS_FILE_TEMPLATE));

        GeneratedFiles files;
        files.append(generatedFilesFile);
        files.append(generatedIncludesFile);
        files.append(generatedConfigFile);
        files.append(generatedCreatorFile);
        files.append(generatedCxxFlagsFile);
        files.append(generatedCFlagsFile);

        return files;
    }

    bool postGenerateFiles(const QWizard *w, const GeneratedFiles &l,
                           QString *errorMessage) const final
    {
        Q_UNUSED(w)
        return CustomProjectWizard::postGenerateOpen(l, errorMessage);
    }
};

void FilesSelectionWizardPage::initializePage()
{
    m_filesWidget->resetModel(m_genericProjectWizardDialog->filePath(), FilePaths());
}

void setupGenericProjectWizard()
{
    IWizardFactory::registerFactoryCreator([] { return new GenericProjectWizardFactory; });
}

} // GenericProjectManager::Internal

#include "genericprojectwizard.moc"
