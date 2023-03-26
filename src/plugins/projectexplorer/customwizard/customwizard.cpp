// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customwizard.h"
#include "customwizardparameters.h"
#include "customwizardpage.h"
#include "customwizardscriptgenerator.h"

#include <projectexplorer/baseprojectwizarddialog.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfiguration.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFile>
#include <QMap>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <QCoreApplication>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {

const char templatePathC[] = "templates/wizards";
const char configFileC[] = "wizard.xml";

static bool enableLoadTemplateFiles()
{
#ifdef WITH_TESTS
    static bool value = qtcEnvironmentVariableIsEmpty("QTC_DISABLE_LOAD_TEMPLATES_FOR_TEST");
#else
    static bool value = true;
#endif
    return value;
}

static QList<ICustomWizardMetaFactory *> g_customWizardMetaFactories;

ICustomWizardMetaFactory::ICustomWizardMetaFactory(const QString &klass, IWizardFactory::WizardKind kind) :
    m_klass(klass), m_kind(kind)
{
    g_customWizardMetaFactories.append(this);
}

ICustomWizardMetaFactory::~ICustomWizardMetaFactory()
{
    g_customWizardMetaFactories.removeOne(this);
}

namespace Internal {
/*!
    \class ProjectExplorer::ICustomWizardFactory
    \brief The ICustomWizardFactory class implements a factory for creating
    custom wizards extending the base classes: CustomWizard and
    CustomProjectWizard.

    The factory can be registered under a name in CustomWizard. The name can
    be specified in the  \c <wizard class=''...> attribute in the \c wizard.xml file
    and thus allows for specifying a C++ derived wizard class.
    For example, this is currently used in Qt4ProjectManager to get Qt-specific
    aspects into the custom wizard.

    \sa ProjectExplorer::CustomWizard, ProjectExplorer::CustomProjectWizard
*/

class CustomWizardPrivate {
public:
    CustomWizardPrivate() : m_context(new CustomWizardContext) {}

    QSharedPointer<CustomWizardParameters> m_parameters;
    QSharedPointer<CustomWizardContext> m_context;
    static int verbose;
};

int CustomWizardPrivate::verbose = 0;

} // namespace Internal

using namespace ProjectExplorer::Internal;

/*!
    \class ProjectExplorer::CustomWizard

    \brief The CustomWizard class is a base class for custom wizards based on
    file templates and an XML
    configuration file (\c share/qtcreator/templates/wizards).

    Presents CustomWizardDialog (fields page containing path control) for wizards
    of type "class" or "file". Serves as base class for project wizards.
*/

CustomWizard::CustomWizard()
    : d(new CustomWizardPrivate)
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
    QTC_ASSERT(p, return);

    d->m_parameters = p;

    setId(p->id);
    setSupportedProjectTypes((p->kind == IWizardFactory::FileWizard)
                             ?  QSet<Id>() : QSet<Id>() << "UNKNOWN_PROJECT");
    setIcon(p->icon);
    setDescription(p->description);
    setDisplayName(p->displayName);
    setCategory(p->category);
    setDisplayCategory(p->displayCategory);
    setRequiredFeatures(p->requiredFeatures);
    setFlags(p->flags);
}

BaseFileWizard *CustomWizard::create(QWidget *parent, const WizardDialogParameters &p) const
{
    QTC_ASSERT(!d->m_parameters.isNull(), return nullptr);
    auto wizard = new BaseFileWizard(this, p.extraValues(), parent);

    d->m_context->reset();
    auto customPage = new CustomWizardPage(d->m_context, parameters());
    customPage->setFilePath(p.defaultPath());
    if (parameters()->firstPageId >= 0)
        wizard->setPage(parameters()->firstPageId, customPage);
    else
        wizard->addPage(customPage);
    const QList<QWizardPage *> pages = wizard->extensionPages();
    for (QWizardPage *ep : pages)
        wizard->addPage(ep);
    if (CustomWizardPrivate::verbose)
        qDebug() << "initWizardDialog" << wizard << wizard->pageIds();

    return wizard;
}

// Read out files and store contents with field contents replaced.
static bool createFile(CustomWizardFile cwFile,
                       const QString &sourceDirectory,
                       const QString &targetDirectory,
                       const CustomProjectWizard::FieldReplacementMap &fm,
                       GeneratedFiles *files,
                       QString *errorMessage)
{
    const QChar slash =  QLatin1Char('/');
    const QString sourcePath = sourceDirectory + slash + cwFile.source;
    // Field replacement on target path
    CustomWizardContext::replaceFields(fm, &cwFile.target);
    const QString targetPath = targetDirectory + slash + cwFile.target;
    if (CustomWizardPrivate::verbose)
        qDebug() << "generating " << targetPath << sourcePath << fm;

    // Read contents of source file
    const QFile::OpenMode openMode
            = cwFile.binary ? QIODevice::ReadOnly : (QIODevice::ReadOnly|QIODevice::Text);
    FileReader reader;
    if (!reader.fetch(FilePath::fromString(sourcePath), openMode, errorMessage))
        return false;

    GeneratedFile generatedFile;
    generatedFile.setFilePath(FilePath::fromString(targetPath).cleanPath());
    if (cwFile.binary) {
        // Binary file: Set data.
        generatedFile.setBinary(true);
        generatedFile.setBinaryContents(reader.data());
    } else {
        // Template file: Preprocess.
        const QString contentsIn = QString::fromLocal8Bit(reader.data());
        generatedFile.setContents(CustomWizardContext::processFile(fm, contentsIn));
    }

    GeneratedFile::Attributes attributes;
    if (cwFile.openEditor)
        attributes |= GeneratedFile::OpenEditorAttribute;
    if (cwFile.openProject)
        attributes |= GeneratedFile::OpenProjectAttribute;
    generatedFile.setAttributes(attributes);
    files->push_back(generatedFile);
    return true;
}

// Helper to find a specific wizard page of a wizard by type.
template <class WizardPage>
        WizardPage *findWizardPage(const QWizard *w)
{
    const QList<int> ids = w->pageIds();
    for (const int pageId : ids)
        if (auto wp = qobject_cast<WizardPage *>(w->page(pageId)))
            return wp;
    return nullptr;
}

// Determine where to run the generator script. The user may specify
// an expression subject to field replacement, default is the target path.
static inline QString scriptWorkingDirectory(const QSharedPointer<CustomWizardContext> &ctx,
                                             const QSharedPointer<CustomWizardParameters> &p)
{
    if (p->filesGeneratorScriptWorkingDirectory.isEmpty())
        return ctx->targetPath.toString();
    QString path = p->filesGeneratorScriptWorkingDirectory;
    CustomWizardContext::replaceFields(ctx->replacements, &path);
    return path;
}

GeneratedFiles CustomWizard::generateFiles(const QWizard *dialog, QString *errorMessage) const
{
    // Look for the Custom field page to find the path
    const CustomWizardPage *cwp = findWizardPage<CustomWizardPage>(dialog);
    QTC_ASSERT(cwp, return {});

    CustomWizardContextPtr ctx = context();
    ctx->path = ctx->targetPath = cwp->filePath();
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

bool CustomWizard::writeFiles(const GeneratedFiles &files, QString *errorMessage) const
{
    if (!BaseFileWizardFactory::writeFiles(files, errorMessage))
        return false;
    if (d->m_parameters->filesGeneratorScript.isEmpty())
        return true;
    // Prepare run of the custom script to generate. In the case of a
    // project wizard that is entirely created by a script,
    // the target project directory might not exist.
    // Known issue: By nature, the script does not honor
    // GeneratedFile::KeepExistingFileAttribute.
    const CustomWizardContextPtr ctx = context();
    const QString scriptWorkingDir = scriptWorkingDirectory(ctx, d->m_parameters);
    const QDir scriptWorkingDirDir(scriptWorkingDir);
    if (!scriptWorkingDirDir.exists()) {
        if (CustomWizardPrivate::verbose)
            qDebug("Creating directory %s", qPrintable(scriptWorkingDir));
        if (!scriptWorkingDirDir.mkpath(scriptWorkingDir)) {
            *errorMessage = QString::fromLatin1("Unable to create the target directory \"%1\"").arg(scriptWorkingDir);
            return false;
        }
    }
    // Run the custom script to actually generate the files.
    if (!runCustomWizardGeneratorScript(scriptWorkingDir,
                                                  d->m_parameters->filesGeneratorScript,
                                                  d->m_parameters->filesGeneratorScriptArguments,
                                                  ctx->replacements, errorMessage))
        return false;
    // Paranoia: Check on the files generated by the script:
    for (const GeneratedFile &generatedFile : files) {
        if (generatedFile.attributes() & GeneratedFile::CustomGeneratorAttribute)
            if (!generatedFile.filePath().isFile()) {
                *errorMessage = QString::fromLatin1("%1 failed to generate %2").
                        arg(d->m_parameters->filesGeneratorScript.back()).
                        arg(generatedFile.filePath().toString());
                return false;
            }
    }
    return true;
}

GeneratedFiles CustomWizard::generateWizardFiles(QString *errorMessage) const
{
    GeneratedFiles rc;
    const CustomWizardContextPtr ctx = context();

    QTC_ASSERT(!ctx->targetPath.isEmpty(), return rc);

    if (CustomWizardPrivate::verbose)
        qDebug() << "CustomWizard::generateWizardFiles: in "
                 << ctx->targetPath << ", using: " << ctx->replacements;

    // If generator script is non-empty, do a dry run to get it's files.
    if (!d->m_parameters->filesGeneratorScript.isEmpty()) {
        rc += dryRunCustomWizardGeneratorScript(scriptWorkingDirectory(ctx, d->m_parameters),
                                                          d->m_parameters->filesGeneratorScript,
                                                          d->m_parameters->filesGeneratorScriptArguments,
                                                          ctx->replacements,
                                                          errorMessage);
        if (rc.isEmpty())
            return rc;
    }
    // Add the template files specified by the <file> elements.
    for (const CustomWizardFile &file : std::as_const(d->m_parameters->files))
        if (!createFile(file, d->m_parameters->directory, ctx->targetPath.toString(), context()->replacements,
                        &rc, errorMessage))
            return {};

    return rc;
}

// Create a replacement map of static base fields + wizard dialog fields
CustomWizard::FieldReplacementMap CustomWizard::replacementMap(const QWizard *w) const
{
    return CustomWizardFieldPage::replacementMap(w, context(), d->m_parameters->fields);
}

CustomWizard::CustomWizardParametersPtr CustomWizard::parameters() const
{
    return d->m_parameters;
}

CustomWizard::CustomWizardContextPtr CustomWizard::context() const
{
    return d->m_context;
}

CustomWizard *CustomWizard::createWizard(const CustomProjectWizard::CustomWizardParametersPtr &p)
{
    ICustomWizardMetaFactory *factory = Utils::findOrDefault(g_customWizardMetaFactories,
        [&p](ICustomWizardMetaFactory *factory) {
            return p->klass.isEmpty() ? (p->kind == factory->kind()) : (p->klass == factory->klass());
        });

    CustomWizard *rc = nullptr;
    if (factory)
        rc = factory->create();

    if (!rc) {
        qWarning("Unable to create custom wizard for class %s.", qPrintable(p->klass));
        return nullptr;
    }

    rc->setParameters(p);
    return rc;
}

/*!
    Reads \c share/qtcreator/templates/wizards and creates all custom wizards.

    As other plugins might register factories for derived
    classes, call it in extensionsInitialized().

    Scans the subdirectories of the template directory for directories
    containing valid configuration files and parse them into wizards.
*/

void CustomWizard::createWizards()
{
    QString errorMessage;
    QString verboseLog;

    const QString templateDirName = ICore::resourcePath(templatePathC).toString();
    const QString userTemplateDirName = ICore::userResourcePath(templatePathC).toString();

    const QDir templateDir(templateDirName);
    if (CustomWizardPrivate::verbose)
        verboseLog += QString::fromLatin1("### CustomWizard: Checking \"%1\"\n").arg(templateDirName);
    if (!templateDir.exists()) {
        if (CustomWizardPrivate::verbose)
           qWarning("Custom project template path %s does not exist.", qPrintable(templateDir.absolutePath()));
        return;
    }

    const QDir userTemplateDir(userTemplateDirName);
    if (CustomWizardPrivate::verbose)
        verboseLog += QString::fromLatin1("### CustomWizard: Checking \"%1\"\n").arg(userTemplateDirName);

    const QDir::Filters filters = QDir::Dirs|QDir::Readable|QDir::NoDotAndDotDot;
    const QDir::SortFlags sortflags = QDir::Name|QDir::IgnoreCase;
    QFileInfoList dirs;
    if (userTemplateDir.exists()) {
        if (CustomWizardPrivate::verbose)
            verboseLog += QString::fromLatin1("### CustomWizard: userTemplateDir \"%1\" found, adding\n").arg(userTemplateDirName);
        dirs += userTemplateDir.entryInfoList(filters, sortflags);
    }
    dirs += templateDir.entryInfoList(filters, sortflags);

    const QString configFile = QLatin1String(configFileC);
    // Check and parse config file in each directory.

    QList<CustomWizardParametersPtr> toCreate;

    while (enableLoadTemplateFiles() && !dirs.isEmpty()) {
        const QFileInfo dirFi = dirs.takeFirst();
        const QDir dir(dirFi.absoluteFilePath());
        if (CustomWizardPrivate::verbose)
            verboseLog += QString::fromLatin1("CustomWizard: Scanning %1\n").arg(dirFi.absoluteFilePath());
        if (dir.exists(configFile)) {
            CustomWizardParametersPtr parameters(new CustomWizardParameters);
            switch (parameters->parse(dir.absoluteFilePath(configFile), &errorMessage)) {
            case CustomWizardParameters::ParseOk:
                if (!Utils::contains(toCreate, [parameters](CustomWizardParametersPtr p) { return parameters->id == p->id; })) {
                    toCreate.append(parameters);
                    parameters->directory = dir.absolutePath();
                    IWizardFactory::registerFactoryCreator([parameters] { return createWizard(parameters); });
                } else {
                    verboseLog += QString::fromLatin1("Customwizard: Ignoring wizard in %1 due to duplicate Id %2.\n")
                            .arg(dir.absolutePath()).arg(parameters->id.toString());
                }
                break;
            case CustomWizardParameters::ParseDisabled:
                if (CustomWizardPrivate::verbose)
                    qWarning("Ignoring disabled wizard %s...", qPrintable(dir.absolutePath()));
                break;
            case CustomWizardParameters::ParseFailed:
                qWarning("Failed to initialize custom project wizard in %s: %s",
                         qPrintable(dir.absolutePath()), qPrintable(errorMessage));
                break;
            }
        } else {
            QFileInfoList subDirs = dir.entryInfoList(filters, sortflags);
            if (!subDirs.isEmpty()) {
                // There is no QList::prepend(QList)...
                dirs.swap(subDirs);
                dirs.append(subDirs);
            } else if (CustomWizardPrivate::verbose) {
                verboseLog += QString::fromLatin1("CustomWizard: \"%1\" not found\n").arg(configFile);
            }
        }
    }
}

/*!
    \class ProjectExplorer::CustomProjectWizard
    \brief The CustomProjectWizard class provides a custom project wizard.

    Presents a CustomProjectWizardDialog (Project intro page and fields page)
    for wizards of type "project".
    Overwrites postGenerateFiles() to open the project files according to the
    file attributes. Also inserts \c '%ProjectName%' into the base
    replacement map once the intro page is left to have it available
    for QLineEdit-type fields' default text.
*/

CustomProjectWizard::CustomProjectWizard() = default;

/*!
    Can be reimplemented to create custom project wizards.

    initProjectWizardDialog() needs to be called.
*/

BaseFileWizard *CustomProjectWizard::create(QWidget *parent,
                                            const WizardDialogParameters &parameters) const
{
    auto projectDialog = new BaseProjectWizardDialog(this, parent, parameters);
    initProjectWizardDialog(projectDialog,
                            parameters.defaultPath(),
                            projectDialog->extensionPages());
    return projectDialog;
}

void CustomProjectWizard::initProjectWizardDialog(BaseProjectWizardDialog *w,
                                                  const FilePath &defaultPath,
                                                  const QList<QWizardPage *> &extensionPages) const
{
    const CustomWizardParametersPtr pa = parameters();
    QTC_ASSERT(!pa.isNull(), return);

    const CustomWizardContextPtr ctx = context();
    ctx->reset();

    if (!displayName().isEmpty())
        w->setWindowTitle(displayName());

    if (!pa->fields.isEmpty()) {
        if (parameters()->firstPageId >= 0)
            w->setPage(parameters()->firstPageId, new CustomWizardFieldPage(ctx, pa));
        else
            w->addPage(new CustomWizardFieldPage(ctx, pa));
    }
    for (QWizardPage *ep : extensionPages)
        w->addPage(ep);
    w->setFilePath(defaultPath);
    w->setProjectName(BaseProjectWizardDialog::uniqueProjectName(defaultPath));

    connect(w, &BaseProjectWizardDialog::projectParametersChanged,
            this, &CustomProjectWizard::handleProjectParametersChanged);

    if (CustomWizardPrivate::verbose)
        qDebug() << "initProjectWizardDialog" << w << w->pageIds();
}

GeneratedFiles CustomProjectWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    const auto *dialog = qobject_cast<const BaseProjectWizardDialog *>(w);
    QTC_ASSERT(dialog, return {});
    // Add project name as macro. Path is here under project directory
    CustomWizardContextPtr ctx = context();
    ctx->path = dialog->filePath();
    ctx->targetPath = ctx->path.pathAppended(dialog->projectName());
    FieldReplacementMap fieldReplacementMap = replacementMap(dialog);
    fieldReplacementMap.insert(QLatin1String("ProjectName"), dialog->projectName());
    ctx->replacements = fieldReplacementMap;
    if (CustomWizardPrivate::verbose)
        qDebug() << "CustomProjectWizard::generateFiles" << dialog << ctx->targetPath << ctx->replacements;
    const GeneratedFiles generatedFiles = generateWizardFiles(errorMessage);
    return generatedFiles;
}

/*!
    Opens the projects and editors for the files that have
    the respective attributes set.
*/

bool CustomProjectWizard::postGenerateOpen(const GeneratedFiles &l, QString *errorMessage)
{
    // Post-Generate: Open the project and the editors as desired
    for (const GeneratedFile &file : l) {
        if (file.attributes() & GeneratedFile::OpenProjectAttribute) {
            ProjectExplorerPlugin::OpenProjectResult result
                    = ProjectExplorerPlugin::openProject(file.filePath());
            if (!result) {
                if (errorMessage)
                    *errorMessage = result.errorMessage();
                return false;
            }
        }
    }
    return BaseFileWizardFactory::postGenerateOpenEditors(l, errorMessage);
}

bool CustomProjectWizard::postGenerateFiles(const QWizard *, const GeneratedFiles &l, QString *errorMessage) const
{
    if (CustomWizardPrivate::verbose)
        qDebug() << "CustomProjectWizard::postGenerateFiles()";
    return CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

void CustomProjectWizard::handleProjectParametersChanged(const QString &name,
                                                         const Utils::FilePath &path)
{
    // Make '%ProjectName%' available in base replacements.
    context()->baseReplacements.insert(QLatin1String("ProjectName"), name);

    emit projectLocationChanged(path / name);
}

} // namespace ProjectExplorer
