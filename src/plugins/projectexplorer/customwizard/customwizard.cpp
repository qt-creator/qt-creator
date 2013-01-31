/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "customwizard.h"
#include "customwizardparameters.h"
#include "customwizardpage.h"
#include "projectexplorer.h"
#include "baseprojectwizarddialog.h"
#include "customwizardscriptgenerator.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFile>
#include <QMap>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <QCoreApplication>

static const char templatePathC[] = "templates/wizards";
static const char configFileC[] = "wizard.xml";

namespace ProjectExplorer {

/*!
    \class ProjectExplorer::ICustomWizardFactory
    \brief Factory for creating custom wizards extending the base class
           (CustomWizard or CustomProjectWizard)

    The factory can be registered under a name in CustomWizard. The name can
    be specified in the  \c <wizard class=''...> attribute in the \c wizard.xml file
    and thus allows for specifying a C++ derived wizard class.
    For example, this is currently used in Qt4ProjectManager to get Qt-specific
    aspects into the custom wizard.

    \sa ProjectExplorer::CustomWizard, ProjectExplorer::CustomProjectWizard
*/

struct CustomWizardPrivate {
    CustomWizardPrivate() : m_context(new Internal::CustomWizardContext) {}

    QSharedPointer<Internal::CustomWizardParameters> m_parameters;
    QSharedPointer<Internal::CustomWizardContext> m_context;
    static int verbose;
};

int CustomWizardPrivate::verbose = 0;

/*!
    \class ProjectExplorer::CustomWizard

    \brief Base classes for custom wizards based on file templates and a XML
    configuration file (\c share/qtcreator/templates/wizards).

    Presents CustomWizardDialog (fields page containing path control) for wizards
    of type "class" or "file". Serves as base class for project wizards.
*/

CustomWizard::CustomWizard(const Core::BaseFileWizardParameters& baseFileParameters,
                           QObject *parent) :
    Core::BaseFileWizard(baseFileParameters, parent),
    d(new CustomWizardPrivate)
{
}

CustomWizard::~CustomWizard()
{
    delete d;
}

void CustomWizard::setVerbose(int v)
{
    CustomWizardPrivate::verbose = v;
}

int CustomWizard::verbose()
{
    return CustomWizardPrivate::verbose;
}

void CustomWizard::setParameters(const CustomWizardParametersPtr &p)
{
    d->m_parameters = p;
}

// Add a wizard page with an id, visibly warn if something goes wrong.
static inline void addWizardPage(Utils::Wizard *w, QWizardPage *p, int id)
{
    int addedPageId = 0;
    if (id == -1) {
        addedPageId = w->addPage(p);
    } else {
        if (w->pageIds().contains(id)) {
            qWarning("Page %d already present in custom wizard dialog, defaulting to add.", id);
            addedPageId = w->addPage(p);
        } else {
            w->setPage(id, p);
            addedPageId = id;
        }
    }
    w->wizardProgress()->item(addedPageId)->setTitle(QCoreApplication::translate("ProjectExplorer::CustomWizard", "Details", "Default short title for custom wizard page to be shown in the progress pane of the wizard."));
}

// Initialize a wizard with a custom file page.
void CustomWizard::initWizardDialog(Utils::Wizard *wizard, const QString &defaultPath,
                                    const WizardPageList &extensionPages) const
{
    QTC_ASSERT(!parameters().isNull(), return);

    d->m_context->reset();
    Internal::CustomWizardPage *customPage = new Internal::CustomWizardPage(d->m_context, parameters());
    customPage->setPath(defaultPath);
    addWizardPage(wizard, customPage, parameters()->firstPageId);
    if (!parameters()->fieldPageTitle.isEmpty())
        customPage->setTitle(parameters()->fieldPageTitle);
    foreach (QWizardPage *ep, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(wizard, wizard->addPage(ep));
    Core::BaseFileWizard::setupWizard(wizard);
    if (CustomWizardPrivate::verbose)
        qDebug() << "initWizardDialog" << wizard << wizard->pageIds();
}

QWizard *CustomWizard::createWizardDialog(QWidget *parent,
                                          const Core::WizardDialogParameters &wizardDialogParameters) const
{
    QTC_ASSERT(!d->m_parameters.isNull(), return 0);
    Utils::Wizard *wizard = new Utils::Wizard(parent);
    initWizardDialog(wizard, wizardDialogParameters.defaultPath(), wizardDialogParameters.extensionPages());
    return wizard;
}

// Read out files and store contents with field contents replaced.
static inline bool createFile(Internal::CustomWizardFile cwFile,
                              const QString &sourceDirectory,
                              const QString &targetDirectory,
                              const CustomProjectWizard::FieldReplacementMap &fm,
                              Core::GeneratedFiles *files,
                              QString *errorMessage)
{
    const QChar slash =  QLatin1Char('/');
    const QString sourcePath = sourceDirectory + slash + cwFile.source;
    // Field replacement on target path
    Internal::CustomWizardContext::replaceFields(fm, &cwFile.target);
    const QString targetPath = targetDirectory + slash + cwFile.target;
    if (CustomWizardPrivate::verbose)
        qDebug() << "generating " << targetPath << sourcePath << fm;

    // Read contents of source file
    const QFile::OpenMode openMode
            = cwFile.binary ? QIODevice::ReadOnly : (QIODevice::ReadOnly|QIODevice::Text);
    Utils::FileReader reader;
    if (!reader.fetch(sourcePath, openMode, errorMessage))
        return false;

    Core::GeneratedFile generatedFile;
    generatedFile.setPath(targetPath);
    if (cwFile.binary) {
        // Binary file: Set data.
        generatedFile.setBinary(true);
        generatedFile.setBinaryContents(reader.data());
    } else {
        // Template file: Preprocess.
        const QString contentsIn = QString::fromLocal8Bit(reader.data());
        generatedFile.setContents(Internal::CustomWizardContext::processFile(fm, contentsIn));
    }

    Core::GeneratedFile::Attributes attributes = 0;
    if (cwFile.openEditor)
        attributes |= Core::GeneratedFile::OpenEditorAttribute;
    if (cwFile.openProject)
        attributes |= Core::GeneratedFile::OpenProjectAttribute;
    generatedFile.setAttributes(attributes);
    files->push_back(generatedFile);
    return true;
}

// Helper to find a specific wizard page of a wizard by type.
template <class WizardPage>
        WizardPage *findWizardPage(const QWizard *w)
{
    foreach (int pageId, w->pageIds())
        if (WizardPage *wp = qobject_cast<WizardPage *>(w->page(pageId)))
            return wp;
    return 0;
}

// Determine where to run the generator script. The user may specify
// an expression subject to field replacement, default is the target path.
static inline QString scriptWorkingDirectory(const QSharedPointer<Internal::CustomWizardContext> &ctx,
                                             const QSharedPointer<Internal::CustomWizardParameters> &p)
{
    if (p->filesGeneratorScriptWorkingDirectory.isEmpty())
        return ctx->targetPath;
    QString path = p->filesGeneratorScriptWorkingDirectory;
    Internal::CustomWizardContext::replaceFields(ctx->replacements, &path);
    return path;
}

Core::GeneratedFiles CustomWizard::generateFiles(const QWizard *dialog, QString *errorMessage) const
{
    // Look for the Custom field page to find the path
    const Internal::CustomWizardPage *cwp = findWizardPage<Internal::CustomWizardPage>(dialog);
    QTC_ASSERT(cwp, return Core::GeneratedFiles());

    CustomWizardContextPtr ctx = context();
    ctx->path = ctx->targetPath = cwp->path();
    ctx->replacements = replacementMap(dialog);
    if (CustomWizardPrivate::verbose) {
        QString logText;
        QTextStream str(&logText);
        str << "CustomWizard::generateFiles: " << ctx->targetPath << '\n';
        const FieldReplacementMap::const_iterator cend = context()->replacements.constEnd();
        for (FieldReplacementMap::const_iterator it = context()->replacements.constBegin(); it != cend; ++it)
            str << "  '" << it.key() << "' -> '" << it.value() << "'\n";
        qWarning("%s", qPrintable(logText));
    }
    return generateWizardFiles(errorMessage);
}

Core::FeatureSet CustomWizard::requiredFeatures() const
{
    return baseFileWizardParameters().requiredFeatures();
}

bool CustomWizard::writeFiles(const Core::GeneratedFiles &files, QString *errorMessage)
{
    if (!Core::BaseFileWizard::writeFiles(files, errorMessage))
        return false;
    if (d->m_parameters->filesGeneratorScript.isEmpty())
        return true;
    // Prepare run of the custom script to generate. In the case of a
    // project wizard that is entirely created by a script,
    // the target project directory might not exist.
    // Known issue: By nature, the script does not honor
    // Core::GeneratedFile::KeepExistingFileAttribute.
    const CustomWizardContextPtr ctx = context();
    const QString scriptWorkingDir = scriptWorkingDirectory(ctx, d->m_parameters);
    const QDir scriptWorkingDirDir(scriptWorkingDir);
    if (!scriptWorkingDirDir.exists()) {
        if (CustomWizardPrivate::verbose)
            qDebug("Creating directory %s", qPrintable(scriptWorkingDir));
        if (!scriptWorkingDirDir.mkpath(scriptWorkingDir)) {
            *errorMessage = QString::fromLatin1("Unable to create the target directory '%1'").arg(scriptWorkingDir);
            return false;
        }
    }
    // Run the custom script to actually generate the files.
    if (!Internal::runCustomWizardGeneratorScript(scriptWorkingDir,
                                                  d->m_parameters->filesGeneratorScript,
                                                  d->m_parameters->filesGeneratorScriptArguments,
                                                  ctx->replacements, errorMessage))
        return false;
    // Paranoia: Check on the files generated by the script:
    foreach (const Core::GeneratedFile &generatedFile, files)
        if (generatedFile.attributes() & Core::GeneratedFile::CustomGeneratorAttribute)
            if (!QFileInfo(generatedFile.path()).isFile()) {
                *errorMessage = QString::fromLatin1("%1 failed to generate %2").
                        arg(d->m_parameters->filesGeneratorScript.back(), generatedFile.path());
                return false;
            }
    return true;
}

Core::GeneratedFiles CustomWizard::generateWizardFiles(QString *errorMessage) const
{
    Core::GeneratedFiles rc;
    const CustomWizardContextPtr ctx = context();

    QTC_ASSERT(!ctx->targetPath.isEmpty(), return rc);

    if (CustomWizardPrivate::verbose)
        qDebug() << "CustomWizard::generateWizardFiles: in "
                 << ctx->targetPath << ", using: " << ctx->replacements;

    // If generator script is non-empty, do a dry run to get it's files.
    if (!d->m_parameters->filesGeneratorScript.isEmpty()) {
        rc += Internal::dryRunCustomWizardGeneratorScript(scriptWorkingDirectory(ctx, d->m_parameters),
                                                          d->m_parameters->filesGeneratorScript,
                                                          d->m_parameters->filesGeneratorScriptArguments,
                                                          ctx->replacements,
                                                          errorMessage);
        if (rc.isEmpty())
            return rc;
    }
    // Add the template files specified by the <file> elements.
    foreach (const Internal::CustomWizardFile &file, d->m_parameters->files)
        if (!createFile(file, d->m_parameters->directory, ctx->targetPath, context()->replacements, &rc, errorMessage))
            return Core::GeneratedFiles();
    return rc;
}

// Create a replacement map of static base fields + wizard dialog fields
CustomWizard::FieldReplacementMap CustomWizard::replacementMap(const QWizard *w) const
{
    return Internal::CustomWizardFieldPage::replacementMap(w, context(), d->m_parameters->fields);
}

CustomWizard::CustomWizardParametersPtr CustomWizard::parameters() const
{
    return d->m_parameters;
}

CustomWizard::CustomWizardContextPtr CustomWizard::context() const
{
    return d->m_context;
}

// Static factory map
typedef QMap<QString, QSharedPointer<ICustomWizardFactory> > CustomWizardFactoryMap;
Q_GLOBAL_STATIC(CustomWizardFactoryMap, customWizardFactoryMap)

void CustomWizard::registerFactory(const QString &name, const ICustomWizardFactoryPtr &f)
{
    customWizardFactoryMap()->insert(name, f);
}

CustomWizard *CustomWizard::createWizard(const CustomWizardParametersPtr &p, const Core::BaseFileWizardParameters &b)
{
    CustomWizard * rc = 0;
    if (p->klass.isEmpty()) {
        // Use defaults for empty class names
        switch (b.kind()) {
            case Core::IWizard::ProjectWizard:
                rc = new CustomProjectWizard(b);
                break;
            case Core::IWizard::FileWizard:
            case Core::IWizard::ClassWizard:
                rc = new CustomWizard(b);
                break;
            }
    } else {
        // Look up class name in map
        const CustomWizardFactoryMap::const_iterator it = customWizardFactoryMap()->constFind(p->klass);
        if (it != customWizardFactoryMap()->constEnd())
            rc = it.value()->create(b);
    }
    if (!rc) {
        qWarning("Unable to create custom wizard for class %s.", qPrintable(p->klass));
        return 0;
    }
    rc->setParameters(p);
    return rc;
}

// Format all wizards for display
static QString listWizards()
{
    typedef QMultiMap<QString, const Core::IWizard *> CategoryWizardMap;

    // Sort by category via multimap
    QString rc;
    QTextStream str(&rc);
    CategoryWizardMap categoryWizardMap;
    foreach (const Core::IWizard *w, Core::IWizard::allWizards())
        categoryWizardMap.insert(w->category(), w);
    str << "### Registered wizards (" << categoryWizardMap.size() << ")\n";
    // Format
    QString lastCategory;
    const CategoryWizardMap::const_iterator cend = categoryWizardMap.constEnd();
    for (CategoryWizardMap::const_iterator it = categoryWizardMap.constBegin(); it != cend; ++it) {
        const Core::IWizard *wizard = it.value();
        if (it.key() != lastCategory) {
            lastCategory = it.key();
            str << "\nCategory: '" << lastCategory << "' / '" << wizard->displayCategory() << "'\n";
        }
        str << "  Id: '" << wizard->id() << "' / '" << wizard->displayName() << "' Kind: "
                << wizard->kind() << "\n  Class: " << wizard->metaObject()->className()
                << " Description: '" << wizard->description() << "'\n";
    }
    return rc;
}

/*!
    \brief Reads \c share/qtcreator/templates/wizards and creates all custom wizards.

    As other plugins might register factories for derived
    classes, call it in extensionsInitialized().

    Scans the subdirectories of the template directory for directories
    containing valid configuration files and parse them into wizards.
*/

QList<CustomWizard*> CustomWizard::createWizards()
{
    QList<CustomWizard*> rc;
    QString errorMessage;
    QString verboseLog;
    const QString templateDirName = Core::ICore::resourcePath() +
                                    QLatin1Char('/') + QLatin1String(templatePathC);


    const QString userTemplateDirName = Core::ICore::userResourcePath() +
                                        QLatin1Char('/') + QLatin1String(templatePathC);


    const QDir templateDir(templateDirName);
    if (CustomWizardPrivate::verbose)
        verboseLog = QString::fromLatin1("### CustomWizard: Checking '%1'\n").arg(templateDirName);
    if (!templateDir.exists()) {
        if (CustomWizardPrivate::verbose)
           qWarning("Custom project template path %s does not exist.", qPrintable(templateDir.absolutePath()));
        return rc;
    }

    const QDir userTemplateDir(userTemplateDirName);
    if (CustomWizardPrivate::verbose)
        verboseLog = QString::fromLatin1("### CustomWizard: Checking '%1'\n").arg(userTemplateDirName);

    const QDir::Filters filters = QDir::Dirs|QDir::Readable|QDir::NoDotAndDotDot;
    const QDir::SortFlags sortflags = QDir::Name|QDir::IgnoreCase;
    QList<QFileInfo> dirs = templateDir.entryInfoList(filters, sortflags);
    if (userTemplateDir.exists()) {
        if (CustomWizardPrivate::verbose)
            verboseLog = QString::fromLatin1("### CustomWizard: userTemplateDir '%1' found, adding\n").arg(userTemplateDirName);
        dirs += userTemplateDir.entryInfoList(filters, sortflags);
    }

    const QString configFile = QLatin1String(configFileC);
    // Check and parse config file in each directory.

    foreach (const QFileInfo &dirFi, dirs) {
        const QDir dir(dirFi.absoluteFilePath());
        if (CustomWizardPrivate::verbose)
            verboseLog += QString::fromLatin1("CustomWizard: Scanning %1\n").arg(dirFi.absoluteFilePath());
        if (dir.exists(configFile)) {
            CustomWizardParametersPtr parameters(new Internal::CustomWizardParameters);
            Core::BaseFileWizardParameters baseFileParameters;
            switch (parameters->parse(dir.absoluteFilePath(configFile), &baseFileParameters, &errorMessage)) {
            case Internal::CustomWizardParameters::ParseOk:
                parameters->directory = dir.absolutePath();
                if (CustomWizardPrivate::verbose)
                    QTextStream(&verboseLog)
                            << "\n### Adding: " << baseFileParameters.id() << " / " << baseFileParameters.displayName() << '\n'
                            << baseFileParameters.category() << " / " <<baseFileParameters.displayCategory() << '\n'
                            << "  (" <<   baseFileParameters.description() << ")\n"
                            << parameters->toString();
                if (CustomWizard *w = createWizard(parameters, baseFileParameters))
                    rc.push_back(w);
                else
                    qWarning("Custom wizard factory function failed for %s", qPrintable(baseFileParameters.id()));
                break;
            case Internal::CustomWizardParameters::ParseDisabled:
                if (CustomWizardPrivate::verbose)
                    qWarning("Ignoring disabled wizard %s...", qPrintable(dir.absolutePath()));
                break;
            case Internal::CustomWizardParameters::ParseFailed:
                qWarning("Failed to initialize custom project wizard in %s: %s",
                         qPrintable(dir.absolutePath()), qPrintable(errorMessage));
                break;
            }
        } else {
            if (CustomWizardPrivate::verbose)
                if (CustomWizardPrivate::verbose)
                    verboseLog += QString::fromLatin1("CustomWizard: '%1' not found\n").arg(configFile);
        }
    }
    if (CustomWizardPrivate::verbose) { // Print to output pane for Windows.
        verboseLog += listWizards();
        qWarning("%s", qPrintable(verboseLog));
        Core::ICore::messageManager()->printToOutputPanePopup(verboseLog);
    }
    return rc;
}

/*!
    \class ProjectExplorer::CustomProjectWizard
    \brief A custom project wizard.

    Presents a CustomProjectWizardDialog (Project intro page and fields page)
    for wizards of type "project".
    Overwrites postGenerateFiles() to open the project files according to the
    file attributes. Also inserts \c '%ProjectName%' into the base
    replacement map once the intro page is left to have it available
    for QLineEdit-type fields' default text.
*/

CustomProjectWizard::CustomProjectWizard(const Core::BaseFileWizardParameters& baseFileParameters,
                                         QObject *parent) :
    CustomWizard(baseFileParameters, parent)
{
}

/*!
    \brief Can be reimplemented to create custom project wizards.

    initProjectWizardDialog() needs to be called.
*/

    QWizard *CustomProjectWizard::createWizardDialog(QWidget *parent,
                                                     const Core::WizardDialogParameters &wizardDialogParameters) const
{
    QTC_ASSERT(!parameters().isNull(), return 0);
    BaseProjectWizardDialog *projectDialog = new BaseProjectWizardDialog(parent, wizardDialogParameters);
    initProjectWizardDialog(projectDialog,
                            wizardDialogParameters.defaultPath(),
                            wizardDialogParameters.extensionPages());
    return projectDialog;
}

void CustomProjectWizard::initProjectWizardDialog(BaseProjectWizardDialog *w,
                                                  const QString &defaultPath,
                                                  const WizardPageList &extensionPages) const
{
    const CustomWizardParametersPtr pa = parameters();
    QTC_ASSERT(!pa.isNull(), return);

    const CustomWizardContextPtr ctx = context();
    ctx->reset();

    if (!displayName().isEmpty())
        w->setWindowTitle(displayName());

    if (!pa->fields.isEmpty()) {
        Internal::CustomWizardFieldPage *cp = new Internal::CustomWizardFieldPage(ctx, pa);
        addWizardPage(w, cp, parameters()->firstPageId);
        if (!pa->fieldPageTitle.isEmpty())
            cp->setTitle(pa->fieldPageTitle);
    }
    foreach (QWizardPage *ep, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(w, w->addPage(ep));
    w->setPath(defaultPath);
    w->setProjectName(BaseProjectWizardDialog::uniqueProjectName(defaultPath));

    connect(w, SIGNAL(projectParametersChanged(QString,QString)), this, SLOT(projectParametersChanged(QString,QString)));

    if (CustomWizardPrivate::verbose)
        qDebug() << "initProjectWizardDialog" << w << w->pageIds();
}

Core::GeneratedFiles CustomProjectWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    const BaseProjectWizardDialog *dialog = qobject_cast<const BaseProjectWizardDialog *>(w);
    QTC_ASSERT(dialog, return Core::GeneratedFiles());
    // Add project name as macro. Path is here under project directory
    CustomWizardContextPtr ctx = context();
    ctx->path = dialog->path();
    ctx->targetPath = ctx->path + QLatin1Char('/') + dialog->projectName();
    FieldReplacementMap fieldReplacementMap = replacementMap(dialog);
    fieldReplacementMap.insert(QLatin1String("ProjectName"), dialog->projectName());
    ctx->replacements = fieldReplacementMap;
    if (CustomWizardPrivate::verbose)
        qDebug() << "CustomProjectWizard::generateFiles" << dialog << ctx->targetPath << ctx->replacements;
    const Core::GeneratedFiles generatedFiles = generateWizardFiles(errorMessage);
    return generatedFiles;
}

/*!
    \brief Utility to open the projects and editors for the files that have
    the respective attributes set.
*/

bool CustomProjectWizard::postGenerateOpen(const Core::GeneratedFiles &l, QString *errorMessage)
{
    // Post-Generate: Open the project and the editors as desired
    foreach (const Core::GeneratedFile &file, l) {
        if (file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
            if (!ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(file.path(), errorMessage))
                return false;
        }
    }
    return BaseFileWizard::postGenerateOpenEditors(l, errorMessage);
}

bool CustomProjectWizard::postGenerateFiles(const QWizard *, const Core::GeneratedFiles &l, QString *errorMessage)
{
    if (CustomWizardPrivate::verbose)
        qDebug() << "CustomProjectWizard::postGenerateFiles()";
    return CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

void CustomProjectWizard::projectParametersChanged(const QString &project, const QString & path)
{
    // Make '%ProjectName%' available in base replacements.
    context()->baseReplacements.insert(QLatin1String("ProjectName"), project);

    emit projectLocationChanged(path + QLatin1Char('/') + project);
}

} // namespace ProjectExplorer
