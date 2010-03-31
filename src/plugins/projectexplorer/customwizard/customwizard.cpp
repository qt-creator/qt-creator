/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "customwizard.h"
#include "customwizardparameters.h"
#include "customwizardpage.h"
#include "projectexplorer.h"
#include "baseprojectwizarddialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QMap>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>

static const char templatePathC[] = "templates/wizards";
static const char configFileC[] = "wizard.xml";

namespace ProjectExplorer {

struct CustomWizardPrivate {
    CustomWizardPrivate() : m_context(new Internal::CustomWizardContext) {}

    QSharedPointer<Internal::CustomWizardParameters> m_parameters;
    QSharedPointer<Internal::CustomWizardContext> m_context;
    static int verbose;
};

int CustomWizardPrivate::verbose = 0;

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
    Internal::CustomWizardPage *customPage = new Internal::CustomWizardPage(d->m_context, parameters()->fields);
    customPage->setPath(defaultPath);
    addWizardPage(wizard, customPage, parameters()->firstPageId);
    if (!parameters()->fieldPageTitle.isEmpty())
        customPage->setTitle(parameters()->fieldPageTitle);
    foreach(QWizardPage *ep, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(wizard, wizard->addPage(ep));
    Core::BaseFileWizard::setupWizard(wizard);
    if (CustomWizardPrivate::verbose)
        qDebug() << "initWizardDialog" << wizard << wizard->pageIds();
}

QWizard *CustomWizard::createWizardDialog(QWidget *parent,
                                                 const QString &defaultPath,
                                                 const WizardPageList &extensionPages) const
{
    QTC_ASSERT(!d->m_parameters.isNull(), return 0);
    Utils::Wizard *wizard = new Utils::Wizard(parent);
    initWizardDialog(wizard, defaultPath, extensionPages);
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
    const QString targetPath = QDir::toNativeSeparators(targetDirectory + slash + cwFile.target);
    if (CustomWizardPrivate::verbose)
        qDebug() << "generating " << targetPath << sourcePath << fm;
    QFile file(sourcePath);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        *errorMessage = QString::fromLatin1("Cannot open %1: %2").arg(sourcePath, file.errorString());
        return false;
    }
    // Field replacement on contents
    QString contents = QString::fromLocal8Bit(file.readAll());
    if (!contents.isEmpty() && !fm.isEmpty())
        Internal::CustomWizardContext::replaceFields(fm, &contents);
    Core::GeneratedFile generatedFile;
    generatedFile.setContents(contents);
    generatedFile.setPath(targetPath);
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

Core::GeneratedFiles CustomWizard::generateFiles(const QWizard *dialog, QString *errorMessage) const
{
    // Look for the Custom field page to find the path
    const Internal::CustomWizardPage *cwp = findWizardPage<Internal::CustomWizardPage>(dialog);
    QTC_ASSERT(cwp, return Core::GeneratedFiles())
    QString path = cwp->path();
    const FieldReplacementMap fieldMap = replacementMap(dialog);
    if (CustomWizardPrivate::verbose) {
        QString logText;
        QTextStream str(&logText);
        str << "CustomWizard::generateFiles: " << path << '\n';
        const FieldReplacementMap::const_iterator cend = fieldMap.constEnd();
        for (FieldReplacementMap::const_iterator it = fieldMap.constBegin(); it != cend; ++it)
            str << "  '" << it.key() << "' -> '" << it.value() << "'\n";
        qWarning("%s", qPrintable(logText));
    }
    return generateWizardFiles(path, fieldMap, errorMessage);
}

Core::GeneratedFiles CustomWizard::generateWizardFiles(const QString &targetPath,
                                                       const FieldReplacementMap &fieldReplacementMap,
                                                       QString *errorMessage) const
{
    if (CustomWizardPrivate::verbose)
        qDebug() << "Replacements" << fieldReplacementMap;
    // Create files
    Core::GeneratedFiles rc;
    foreach(const Internal::CustomWizardFile &file, d->m_parameters->files)
        if (!createFile(file, d->m_parameters->directory, targetPath, fieldReplacementMap, &rc, errorMessage))
            return Core::GeneratedFiles();
    return rc;
}

// Create a replacement map of static base fields + wizard dialog fields
CustomWizard::FieldReplacementMap CustomWizard::replacementMap(const QWizard *w) const
{
    FieldReplacementMap fieldReplacementMap = d->m_context->baseReplacements;
    foreach(const Internal::CustomWizardField &field, d->m_parameters->fields) {
        const QString value = w->field(field.name).toString();
        fieldReplacementMap.insert(field.name, value);
    }
    return fieldReplacementMap;
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
    foreach(const  Core::IWizard *w, Core::IWizard::allWizards())
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

// Scan the subdirectories of the template directory for directories
// containing valid configuration files and parse them into wizards.
QList<CustomWizard*> CustomWizard::createWizards()
{
    QList<CustomWizard*> rc;
    QString errorMessage;
    QString verboseLog;
    const QString templateDirName = Core::ICore::instance()->resourcePath() +
                                    QLatin1Char('/') + QLatin1String(templatePathC);

    const QDir templateDir(templateDirName);
    if (CustomWizardPrivate::verbose)
        verboseLog = QString::fromLatin1("### CustomWizard: Checking '%1'\n").arg(templateDirName);
    if (!templateDir.exists()) {
        if (CustomWizardPrivate::verbose)
           qWarning("Custom project template path %s does not exist.", qPrintable(templateDir.absolutePath()));
        return rc;
    }

    const QList<QFileInfo> dirs = templateDir.entryInfoList(QDir::Dirs|QDir::Readable|QDir::NoDotAndDotDot,
                                                            QDir::Name|QDir::IgnoreCase);
    const QString configFile = QLatin1String(configFileC);
    // Check and parse config file in each directory.

    foreach(const QFileInfo &dirFi, dirs) {
        const QDir dir(dirFi.absoluteFilePath());
        if (CustomWizardPrivate::verbose)
            verboseLog += QString::fromLatin1("CustomWizard: Scanning %1\n").arg(dirFi.absoluteFilePath());
        if (dir.exists(configFile)) {
            CustomWizardParametersPtr parameters(new Internal::CustomWizardParameters);
            Core::BaseFileWizardParameters baseFileParameters;
            if (parameters->parse(dir.absoluteFilePath(configFile), &baseFileParameters, &errorMessage)) {
                parameters->directory = dir.absolutePath();
                if (CustomWizardPrivate::verbose)
                    verboseLog += parameters->toString();
                if (CustomWizard *w = createWizard(parameters, baseFileParameters))
                    rc.push_back(w);
            } else {
                qWarning("Failed to initialize custom project wizard in %s: %s",
                         qPrintable(dir.absolutePath()), qPrintable(errorMessage));
            }
        } else {
            if (CustomWizardPrivate::verbose)
                if (CustomWizardPrivate::verbose)
                    verboseLog += QString::fromLatin1("CustomWizard: '%1' not found\n").arg(qPrintable(configFile));
        }
    }
    if (CustomWizardPrivate::verbose) { // Print to output pane for Windows.
        verboseLog += listWizards();
        qWarning("%s", qPrintable(verboseLog));
        Core::ICore::instance()->messageManager()->printToOutputPanePopup(verboseLog);
    }
    return rc;
}

// --------------- CustomProjectWizard

CustomProjectWizard::CustomProjectWizard(const Core::BaseFileWizardParameters& baseFileParameters,
                                         QObject *parent) :
    CustomWizard(baseFileParameters, parent)
{
}

QWizard *CustomProjectWizard::createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const
{
    QTC_ASSERT(!parameters().isNull(), return 0);
    BaseProjectWizardDialog *projectDialog = new BaseProjectWizardDialog(parent);
    initProjectWizardDialog(projectDialog, defaultPath, extensionPages);
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
        Internal::CustomWizardFieldPage *cp = new Internal::CustomWizardFieldPage(ctx, pa->fields);
        addWizardPage(w, cp, parameters()->firstPageId);
        if (!pa->fieldPageTitle.isEmpty())
            cp->setTitle(pa->fieldPageTitle);
    }
    foreach(QWizardPage *ep, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(w, w->addPage(ep));
    w->setPath(defaultPath);
    w->setProjectName(BaseProjectWizardDialog::uniqueProjectName(defaultPath));

    connect(w, SIGNAL(introPageLeft(QString,QString)), this, SLOT(introPageLeft(QString,QString)));

    if (CustomWizardPrivate::verbose)
        qDebug() << "initProjectWizardDialog" << w << w->pageIds();
}

Core::GeneratedFiles CustomProjectWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    const BaseProjectWizardDialog *dialog = qobject_cast<const BaseProjectWizardDialog *>(w);
    QTC_ASSERT(dialog, return Core::GeneratedFiles())
    const QString targetPath = dialog->path() + QLatin1Char('/') + dialog->projectName();
    // Add project name as macro.
    FieldReplacementMap fieldReplacementMap = replacementMap(dialog);
    fieldReplacementMap.insert(QLatin1String("ProjectName"), dialog->projectName());
    if (CustomWizardPrivate::verbose)
        qDebug() << "CustomProjectWizard::generateFiles" << dialog << targetPath << fieldReplacementMap;
    return generateWizardFiles(targetPath, fieldReplacementMap, errorMessage);
}

bool CustomProjectWizard::postGenerateFiles(const QWizard *, const Core::GeneratedFiles &l, QString *errorMessage)
{
    // Post-Generate: Open the project
    const QString proFileName = l.back().path();
    const bool opened = ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFileName);
    if (CustomWizardPrivate::verbose)
        qDebug() << "CustomProjectWizard::postGenerateFiles: opened " << proFileName << opened;
    if (opened) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

void CustomProjectWizard::introPageLeft(const QString &project, const QString & /* path */)
{
    // Make '%ProjectName%' available in base replacements.
    context()->baseReplacements.insert(QLatin1String("ProjectName"), project);
}

} // namespace ProjectExplorer
