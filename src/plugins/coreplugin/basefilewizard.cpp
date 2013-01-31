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

#include "basefilewizard.h"

#include "coreconstants.h"
#include "icore.h"
#include "ifilewizardextension.h"
#include "mimedatabase.h"
#include "editormanager/editormanager.h"
#include "dialogs/promptoverwritedialog.h"
#include <extensionsystem/pluginmanager.h>
#include <utils/filewizarddialog.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVector>
#include <QDebug>
#include <QSharedData>
#include <QEventLoop>
#include <QSharedPointer>
#include <QScopedPointer>

#include <QMessageBox>
#include <QWizard>
#include <QIcon>

enum { debugWizard = 0 };

namespace Core {

static int indexOfFile(const GeneratedFiles &f, const QString &path)
{
    const int size = f.size();
    for (int i = 0; i < size; ++i)
        if (f.at(i).path() == path)
            return i;
    return -1;
}

// ------------ BaseFileWizardParameterData
class BaseFileWizardParameterData : public QSharedData
{
public:
    explicit BaseFileWizardParameterData(IWizard::WizardKind kind = IWizard::FileWizard);
    void clear();

    IWizard::WizardKind kind;
    QIcon icon;
    QString description;
    QString displayName;
    QString id;
    QString category;
    QString displayCategory;
    Core::FeatureSet requiredFeatures;
    Core::IWizard::WizardFlags flags;
    QString descriptionImage;
};

BaseFileWizardParameterData::BaseFileWizardParameterData(IWizard::WizardKind k) :
    kind(k)
{
}

void BaseFileWizardParameterData::clear()
{
    kind = IWizard::FileWizard;
    icon = QIcon();
    description.clear();
    displayName.clear();
    id.clear();
    category.clear();
    displayCategory.clear();
}

/*!
    \class Core::BaseFileWizardParameters
    \brief Parameter class for passing parameters to instances of class Wizard
     containing name, icon and such.

    \sa Core::GeneratedFile, Core::BaseFileWizard, Core::StandardFileWizard
    \sa Core::Internal::WizardEventLoop
*/

BaseFileWizardParameters::BaseFileWizardParameters(IWizard::WizardKind kind) :
   m_d(new BaseFileWizardParameterData(kind))
{
}

BaseFileWizardParameters::BaseFileWizardParameters(const BaseFileWizardParameters &rhs) :
    m_d(rhs.m_d)
{
}

BaseFileWizardParameters &BaseFileWizardParameters::operator=(const BaseFileWizardParameters &rhs)
{
    if (this != &rhs)
        m_d.operator=(rhs.m_d);
    return *this;
}

BaseFileWizardParameters::~BaseFileWizardParameters()
{
}

void BaseFileWizardParameters::clear()
{
    m_d->clear();
}

CORE_EXPORT QDebug operator<<(QDebug d, const BaseFileWizardParameters &p)
{
    d.nospace() << "Kind: " << p.kind() << " Id: " << p.id()
                << " Category: " << p.category()
                << " DisplayName: " << p.displayName()
                << " Description: " << p.description()
                << " DisplayCategory: " << p.displayCategory()
                << " Required Features: " << p.requiredFeatures().toStringList();
    return d;
}

IWizard::WizardKind BaseFileWizardParameters::kind() const
{
    return m_d->kind;
}

void BaseFileWizardParameters::setKind(IWizard::WizardKind k)
{
    m_d->kind = k;
}

QIcon BaseFileWizardParameters::icon() const
{
    return m_d->icon;
}

void BaseFileWizardParameters::setIcon(const QIcon &icon)
{
    m_d->icon = icon;
}

QString BaseFileWizardParameters::description() const
{
    return m_d->description;
}

void BaseFileWizardParameters::setDescription(const QString &v)
{
    m_d->description = v;
}

QString BaseFileWizardParameters::displayName() const
{
    return m_d->displayName;
}

void BaseFileWizardParameters::setDisplayName(const QString &v)
{
    m_d->displayName = v;
}

QString BaseFileWizardParameters::id() const
{
    return m_d->id;
}

void BaseFileWizardParameters::setId(const QString &v)
{
    m_d->id = v;
}

QString BaseFileWizardParameters::category() const
{
    return m_d->category;
}

void BaseFileWizardParameters::setCategory(const QString &v)
{
    m_d->category = v;
}

QString BaseFileWizardParameters::displayCategory() const
{
    return m_d->displayCategory;
}

Core::FeatureSet BaseFileWizardParameters::requiredFeatures() const
{
    return m_d->requiredFeatures;
}

void BaseFileWizardParameters::setRequiredFeatures(Core::FeatureSet features)
{

    m_d->requiredFeatures = features;
}

void BaseFileWizardParameters::setDisplayCategory(const QString &v)
{
    m_d->displayCategory = v;
}

Core::IWizard::WizardFlags BaseFileWizardParameters::flags() const
{
    return m_d->flags;
}

void BaseFileWizardParameters::setFlags(Core::IWizard::WizardFlags flags)
{
    m_d->flags = flags;
}

QString BaseFileWizardParameters::descriptionImage() const
{
  return m_d->descriptionImage;
}

void BaseFileWizardParameters::setDescriptionImage(const QString &path)
{
    m_d->descriptionImage = path;
}

/*!
    \class Core::Internal::WizardEventLoop
    \brief Special event loop that runs a QWizard and terminates if the page changes.

    Use by Core::BaseFileWizard to intercept the change from the standard wizard pages
    to the extension pages (as the latter require the list of Core::GeneratedFile generated).

    Synopsis:
    \code
    Wizard wizard(parent);
    WizardEventLoop::WizardResult wr;
    do {
        wr = WizardEventLoop::execWizardPage(wizard);
    } while (wr == WizardEventLoop::PageChanged);
    \endcode

    \sa Core::GeneratedFile, Core::BaseFileWizardParameters, Core::BaseFileWizard, Core::StandardFileWizard
*/

class WizardEventLoop : public QEventLoop
{
    Q_OBJECT
    WizardEventLoop(QObject *parent);

public:
    enum WizardResult { Accepted, Rejected , PageChanged };

    static WizardResult execWizardPage(QWizard &w);

private slots:
    void pageChanged(int);
    void accepted();
    void rejected();

private:
    WizardResult execWizardPageI();

    WizardResult m_result;
};

WizardEventLoop::WizardEventLoop(QObject *parent) :
    QEventLoop(parent),
    m_result(Rejected)
{
}

WizardEventLoop::WizardResult WizardEventLoop::execWizardPage(QWizard &wizard)
{
    /* Install ourselves on the wizard. Main trick is here to connect
     * to the page changed signal and quit() on it. */
    WizardEventLoop *eventLoop = wizard.findChild<WizardEventLoop *>();
    if (!eventLoop) {
        eventLoop = new WizardEventLoop(&wizard);
        connect(&wizard, SIGNAL(currentIdChanged(int)), eventLoop, SLOT(pageChanged(int)));
        connect(&wizard, SIGNAL(accepted()), eventLoop, SLOT(accepted()));
        connect(&wizard, SIGNAL(rejected()), eventLoop, SLOT(rejected()));
        wizard.setAttribute(Qt::WA_ShowModal, true);
        wizard.show();
    }
    const WizardResult result = eventLoop->execWizardPageI();
    // Quitting?
    if (result != PageChanged)
        delete eventLoop;
    if (debugWizard)
        qDebug() << "WizardEventLoop::runWizard" << wizard.pageIds() << " returns " << result;

    return result;
}

WizardEventLoop::WizardResult WizardEventLoop::execWizardPageI()
{
    m_result = Rejected;
    exec(QEventLoop::DialogExec);
    return m_result;
}

void WizardEventLoop::pageChanged(int /*page*/)
{
    m_result = PageChanged;
    quit(); // !
}

void WizardEventLoop::accepted()
{
    m_result = Accepted;
    quit();
}

void WizardEventLoop::rejected()
{
    m_result = Rejected;
    quit();
}

/*!
    \class Core::BaseFileWizard
    \brief A generic wizard for creating files.

    The abstract methods:
    \list
    \o createWizardDialog(): Called to create the QWizard dialog to be shown
    \o generateFiles(): Generate file content
    \endlist

    must be implemented.
    The behaviour can be further customized by overwriting the virtual method \c postGenerateFiles(),
    which is called after generating the files.

    \sa Core::GeneratedFile, Core::BaseFileWizardParameters, Core::StandardFileWizard
    \sa Core::Internal::WizardEventLoop
*/

struct BaseFileWizardPrivate
{
    explicit BaseFileWizardPrivate(const Core::BaseFileWizardParameters &parameters)
      : m_parameters(parameters), m_wizardDialog(0)
    {}

    const Core::BaseFileWizardParameters m_parameters;
    QWizard *m_wizardDialog;
};

// ---------------- Wizard
BaseFileWizard::BaseFileWizard(const BaseFileWizardParameters &parameters,
                       QObject *parent) :
    IWizard(parent),
    d(new BaseFileWizardPrivate(parameters))
{
}

BaseFileWizardParameters BaseFileWizard::baseFileWizardParameters() const
{
    return d->m_parameters;
}

BaseFileWizard::~BaseFileWizard()
{
    delete d;
}

IWizard::WizardKind  BaseFileWizard::kind() const
{
    return d->m_parameters.kind();
}

QIcon BaseFileWizard::icon() const
{
    return d->m_parameters.icon();
}

QString BaseFileWizard::description() const
{
    return d->m_parameters.description();
}

QString BaseFileWizard::displayName() const
{
    return d->m_parameters.displayName();
}

QString BaseFileWizard::id() const
{
    return d->m_parameters.id();
}

QString BaseFileWizard::category() const
{
    return d->m_parameters.category();
}

QString BaseFileWizard::displayCategory() const
{
    return d->m_parameters.displayCategory();
}

QString BaseFileWizard::descriptionImage() const
{
       return d->m_parameters.descriptionImage();
}

void BaseFileWizard::runWizard(const QString &path, QWidget *parent, const QString &platform, const QVariantMap &extraValues)
{
    QTC_ASSERT(!path.isEmpty(), return);

    typedef  QList<IFileWizardExtension*> ExtensionList;

    QString errorMessage;
    // Compile extension pages, purge out unused ones
    ExtensionList extensions = ExtensionSystem::PluginManager::getObjects<IFileWizardExtension>();
    WizardPageList  allExtensionPages;
    for (ExtensionList::iterator it = extensions.begin(); it !=  extensions.end(); ) {
        const WizardPageList extensionPages = (*it)->extensionPages(this);
        if (extensionPages.empty()) {
            it = extensions.erase(it);
        } else {
            allExtensionPages += extensionPages;
            ++it;
        }
    }

    if (debugWizard)
        qDebug() << Q_FUNC_INFO <<  path << parent << "exs" <<  extensions.size() << allExtensionPages.size();

    QWizardPage *firstExtensionPage = 0;
    if (!allExtensionPages.empty())
        firstExtensionPage = allExtensionPages.front();

    // Create dialog and run it. Ensure that the dialog is deleted when
    // leaving the func, but not before the IFileWizardExtension::process
    // has been called

    WizardDialogParameters::DialogParameterFlags dialogParameterFlags;

    if (flags().testFlag(ForceCapitalLetterForFileName))
        dialogParameterFlags |= WizardDialogParameters::ForceCapitalLetterForFileName;

    const QScopedPointer<QWizard> wizard(createWizardDialog(parent, WizardDialogParameters(path,
                                                                                           allExtensionPages,
                                                                                           platform,
                                                                                           requiredFeatures(),
                                                                                           dialogParameterFlags,
                                                                                           extraValues)));
    QTC_ASSERT(!wizard.isNull(), return);

    GeneratedFiles files;
    // Run the wizard: Call generate files on switching to the first extension
    // page is OR after 'Accepted' if there are no extension pages
    while (true) {
        const WizardEventLoop::WizardResult wr = WizardEventLoop::execWizardPage(*wizard);
        if (wr == WizardEventLoop::Rejected) {
            files.clear();
            break;
        }
        const bool accepted = wr == WizardEventLoop::Accepted;
        const bool firstExtensionPageHit = wr == WizardEventLoop::PageChanged
                                           && wizard->page(wizard->currentId()) == firstExtensionPage;
        const bool needGenerateFiles = firstExtensionPageHit || (accepted && allExtensionPages.empty());
        if (needGenerateFiles) {
            QString errorMessage;
            files = generateFiles(wizard.data(), &errorMessage);
            if (files.empty()) {
                QMessageBox::critical(0, tr("File Generation Failure"), errorMessage);
                break;
            }
        }
        if (firstExtensionPageHit)
            foreach (IFileWizardExtension *ex, extensions)
                ex->firstExtensionPageShown(files, extraValues);
        if (accepted)
            break;
    }
    if (files.empty())
        return;
    // Compile result list and prompt for overwrite
    switch (promptOverwrite(&files, &errorMessage)) {
    case OverwriteCanceled:
        return;
    case OverwriteError:
        QMessageBox::critical(0, tr("Existing files"), errorMessage);
        return;
    case OverwriteOk:
        break;
    }

    foreach (IFileWizardExtension *ex, extensions) {
        for (int i = 0; i < files.count(); i++) {
            ex->applyCodeStyle(&files[i]);
        }
    }

    // Write
    if (!writeFiles(files, &errorMessage)) {
        QMessageBox::critical(parent, tr("File Generation Failure"), errorMessage);
        return;
    }

    bool removeOpenProjectAttribute = false;
    // Run the extensions
    foreach (IFileWizardExtension *ex, extensions) {
        bool remove;
        if (!ex->processFiles(files, &remove, &errorMessage)) {
            QMessageBox::critical(parent, tr("File Generation Failure"), errorMessage);
            return;
        }
        removeOpenProjectAttribute |= remove;
    }

    if (removeOpenProjectAttribute) {
        for (int i = 0; i < files.count(); i++) {
            if (files[i].attributes() & Core::GeneratedFile::OpenProjectAttribute)
                files[i].setAttributes(Core::GeneratedFile::OpenEditorAttribute);
        }
    }

    // Post generation handler
    if (!postGenerateFiles(wizard.data(), files, &errorMessage))
        if (!errorMessage.isEmpty())
            QMessageBox::critical(0, tr("File Generation Failure"), errorMessage);
}


Core::FeatureSet BaseFileWizard::requiredFeatures() const
{
    return d->m_parameters.requiredFeatures();
}

Core::IWizard::WizardFlags BaseFileWizard::flags() const
{
    return d->m_parameters.flags();
}

/*!
    \fn virtual QWizard *Core::BaseFileWizard::createWizardDialog(QWidget *parent,
                                                                  const QString &defaultPath,
                                                                  const WizardPageList &extensionPages) const = 0
    \brief Implement to create the wizard dialog on the parent, adding the extension pages.
*/

/*!
    \fn virtual Core::GeneratedFiles Core::BaseFileWizard::generateFiles(const QWizard *w,
                                                                         QString *errorMessage) const = 0
     \brief Overwrite to query the parameters from the dialog and generate the files.

     Note: This does not generate physical files, but merely the list of Core::GeneratedFile.
*/

/*!
    \brief Physically write files.

    Re-implement (calling the base implementation) to create files with CustomGeneratorAttribute set.
*/

bool BaseFileWizard::writeFiles(const GeneratedFiles &files, QString *errorMessage)
{
    const GeneratedFile::Attributes noWriteAttributes
        = GeneratedFile::CustomGeneratorAttribute|GeneratedFile::KeepExistingFileAttribute;
    foreach (const GeneratedFile &generatedFile, files)
        if (!(generatedFile.attributes() & noWriteAttributes ))
            if (!generatedFile.write(errorMessage))
                return false;
    return true;
}

/*!
    \brief Sets some standard options on a QWizard
*/

void BaseFileWizard::setupWizard(QWizard *w)
{
    w->setOption(QWizard::NoCancelButton, false);
    w->setOption(QWizard::NoDefaultButton, false);
    w->setOption(QWizard::NoBackButtonOnStartPage, true);
    w->setWindowFlags(w->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    if (Utils::HostOsInfo::isMacHost()) {
        w->setButtonLayout(QList<QWizard::WizardButton>()
                           << QWizard::CancelButton
                           << QWizard::Stretch
                           << QWizard::BackButton
                           << QWizard::NextButton
                           << QWizard::CommitButton
                           << QWizard::FinishButton);
    }
}

/*!
    \brief Read "shortTitle" dynamic property of the pageId and apply it as the title
    of corresponding progress item
*/

void BaseFileWizard::applyExtensionPageShortTitle(Utils::Wizard *wizard, int pageId)
{
    if (pageId < 0)
        return;
    QWizardPage *p = wizard->page(pageId);
    if (!p)
        return;
    Utils::WizardProgressItem *item = wizard->wizardProgress()->item(pageId);
    if (!item)
        return;
    const QString shortTitle = p->property("shortTitle").toString();
    if (!shortTitle.isEmpty())
      item->setTitle(shortTitle);
}

/*!
    \brief Overwrite to perform steps to be done after files are actually created.

    The default implementation opens editors with the newly generated files.
*/

bool BaseFileWizard::postGenerateFiles(const QWizard *, const GeneratedFiles &l, QString *errorMessage)
{
    return BaseFileWizard::postGenerateOpenEditors(l, errorMessage);
}

/*!
    \brief Utility to open the editors for the files whose attribute is set accordingly.
*/

bool BaseFileWizard::postGenerateOpenEditors(const GeneratedFiles &l, QString *errorMessage)
{
    foreach (const Core::GeneratedFile &file, l) {
        if (file.attributes() & Core::GeneratedFile::OpenEditorAttribute) {
            if (!Core::EditorManager::openEditor(file.path(), file.editorId(), Core::EditorManager::ModeSwitch )) {
                if (errorMessage)
                    *errorMessage = tr("Failed to open an editor for '%1'.").arg(QDir::toNativeSeparators(file.path()));
                return false;
            }
        }
    }
    return true;
}

/*!
    \brief Utility that performs an overwrite check on a set of files. It checks if
    the file exists, can be overwritten at all and prompts the user with a summary.
*/

BaseFileWizard::OverwriteResult BaseFileWizard::promptOverwrite(GeneratedFiles *files,
                                                                QString *errorMessage) const
{
    if (debugWizard)
        qDebug() << Q_FUNC_INFO << files;

    QStringList existingFiles;
    bool oddStuffFound = false;

    static const QString readOnlyMsg = tr(" [read only]");
    static const QString directoryMsg = tr(" [folder]");
    static const QString symLinkMsg = tr(" [symbolic link]");

    foreach (const GeneratedFile &file, *files) {
        const QFileInfo fi(file.path());
        if (fi.exists())
            existingFiles.append(file.path());
    }
    if (existingFiles.isEmpty())
        return OverwriteOk;
    // Before prompting to overwrite existing files, loop over files and check
    // if there is anything blocking overwriting them (like them being links or folders).
    // Format a file list message as ( "<file1> [readonly], <file2> [folder]").
    const QString commonExistingPath = Utils::commonPath(existingFiles);
    QString fileNamesMsgPart;
    foreach (const QString &fileName, existingFiles) {
        const QFileInfo fi(fileName);
        if (fi.exists()) {
            if (!fileNamesMsgPart.isEmpty())
                fileNamesMsgPart += QLatin1String(", ");
            fileNamesMsgPart += QDir::toNativeSeparators(fileName.mid(commonExistingPath.size() + 1));
            do {
                if (fi.isDir()) {
                    oddStuffFound = true;
                    fileNamesMsgPart += directoryMsg;
                    break;
                }
                if (fi.isSymLink()) {
                    oddStuffFound = true;
                    fileNamesMsgPart += symLinkMsg;
                    break;
            }
                if (!fi.isWritable()) {
                    oddStuffFound = true;
                    fileNamesMsgPart += readOnlyMsg;
                }
            } while (false);
        }
    }

    if (oddStuffFound) {
        *errorMessage = tr("The project directory %1 contains files which cannot be overwritten:\n%2.")
                .arg(QDir::toNativeSeparators(commonExistingPath)).arg(fileNamesMsgPart);
        return OverwriteError;
    }
    // Prompt to overwrite existing files.
    Internal::PromptOverwriteDialog overwriteDialog;
    // Scripts cannot handle overwrite
    overwriteDialog.setFiles(existingFiles);
    foreach (const GeneratedFile &file, *files)
        if (file.attributes() & GeneratedFile::CustomGeneratorAttribute)
            overwriteDialog.setFileEnabled(file.path(), false);
    if (overwriteDialog.exec() != QDialog::Accepted)
        return OverwriteCanceled;
    const QStringList existingFilesToKeep = overwriteDialog.uncheckedFiles();
    if (existingFilesToKeep.size() == files->size()) // All exist & all unchecked->Cancel.
        return OverwriteCanceled;
    // Set 'keep' attribute in files
    foreach (const QString &keepFile, existingFilesToKeep) {
        const int i = indexOfFile(*files, keepFile);
        QTC_ASSERT(i != -1, return OverwriteCanceled);
        GeneratedFile &file = (*files)[i];
        file.setAttributes(file.attributes() | GeneratedFile::KeepExistingFileAttribute);
    }
    return OverwriteOk;
}

/*!
    \brief Build a file name, adding the extension unless baseName already has one.
*/

QString BaseFileWizard::buildFileName(const QString &path,
                                      const QString &baseName,
                                      const QString &extension)
{
    QString rc = path;
    if (!rc.isEmpty() && !rc.endsWith(QDir::separator()))
        rc += QDir::separator();
    rc += baseName;
    // Add extension unless user specified something else
    const QChar dot = QLatin1Char('.');
    if (!extension.isEmpty() && !baseName.contains(dot)) {
        if (!extension.startsWith(dot))
            rc += dot;
        rc += extension;
    }
    if (debugWizard)
        qDebug() << Q_FUNC_INFO << rc;
    return rc;
}

/*!
    \brief Utility that returns the preferred suffix for a mime type
*/

QString BaseFileWizard::preferredSuffix(const QString &mimeType)
{
    const QString rc = Core::ICore::mimeDatabase()->preferredSuffixByType(mimeType);
    if (rc.isEmpty())
        qWarning("%s: WARNING: Unable to find a preferred suffix for %s.",
                 Q_FUNC_INFO, mimeType.toUtf8().constData());
    return rc;
}

/*!
    \class Core::StandardFileWizard
    \brief Convenience class for creating one file.

    It uses Utils::FileWizardDialog and introduces a new virtual to generate the
    files from path and name.

    \sa Core::GeneratedFile, Core::BaseFileWizardParameters, Core::BaseFileWizard
    \sa Core::Internal::WizardEventLoop
*/

/*!
    \fn Core::GeneratedFiles Core::StandardFileWizard::generateFilesFromPath(const QString &path,
                                                                             const QString &name,
                                                                             QString *errorMessage) const = 0
    \brief Newly introduced virtual that creates the files under the path.
*/

StandardFileWizard::StandardFileWizard(const BaseFileWizardParameters &parameters,
                                       QObject *parent) :
    BaseFileWizard(parameters, parent)
{
}

/*!
    \brief Implemented to create a Utils::FileWizardDialog.
*/

QWizard *StandardFileWizard::createWizardDialog(QWidget *parent,
                                                const WizardDialogParameters &wizardDialogParameters) const
{
    Utils::FileWizardDialog *standardWizardDialog = new Utils::FileWizardDialog(parent);
    if (wizardDialogParameters.flags().testFlag(WizardDialogParameters::ForceCapitalLetterForFileName))
        standardWizardDialog->setForceFirstCapitalLetterForFileName(true);
    standardWizardDialog->setWindowTitle(tr("New %1").arg(displayName()));
    setupWizard(standardWizardDialog);
    standardWizardDialog->setPath(wizardDialogParameters.defaultPath());
    foreach (QWizardPage *p, wizardDialogParameters.extensionPages())
        BaseFileWizard::applyExtensionPageShortTitle(standardWizardDialog, standardWizardDialog->addPage(p));
    return standardWizardDialog;
}

/*!
    \brief Implemented to retrieve path and name and call generateFilesFromPath()
*/

GeneratedFiles StandardFileWizard::generateFiles(const QWizard *w,
                                                 QString *errorMessage) const
{
    const Utils::FileWizardDialog *standardWizardDialog = qobject_cast<const Utils::FileWizardDialog *>(w);
    return generateFilesFromPath(standardWizardDialog->path(),
                                 standardWizardDialog->fileName(),
                                 errorMessage);
}

} // namespace Core

#include "basefilewizard.moc"
