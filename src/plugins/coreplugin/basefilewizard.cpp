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

#include "basefilewizard.h"

#include "coreconstants.h"
#include "icore.h"
#include "ifilewizardextension.h"
#include "mimedatabase.h"
#include "editormanager/editormanager.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/filewizarddialog.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QVector>
#include <QtCore/QDebug>
#include <QtCore/QSharedData>
#include <QtCore/QEventLoop>
#include <QtCore/QSharedPointer>

#include <QtGui/QMessageBox>
#include <QtGui/QWizard>
#include <QtGui/QMainWindow>

enum { debugWizard = 0 };

namespace Core {

class GeneratedFilePrivate : public QSharedData
{
public:
    GeneratedFilePrivate() : binary(false) {}
    explicit GeneratedFilePrivate(const QString &p);
    QString path;
    QByteArray contents;
    QString editorId;
    bool binary;
};

GeneratedFilePrivate::GeneratedFilePrivate(const QString &p) :
    path(p),
    binary(false)
{
}

GeneratedFile::GeneratedFile() :
    m_d(new GeneratedFilePrivate)
{
}

GeneratedFile::GeneratedFile(const QString &p) :
    m_d(new GeneratedFilePrivate(p))
{
}

GeneratedFile::GeneratedFile(const GeneratedFile &rhs) :
    m_d(rhs.m_d)
{
}

GeneratedFile &GeneratedFile::operator=(const GeneratedFile &rhs)
{
    if (this != &rhs)
        m_d.operator=(rhs.m_d);
    return *this;
}

GeneratedFile::~GeneratedFile()
{
}

QString GeneratedFile::path() const
{
    return m_d->path;
}

void GeneratedFile::setPath(const QString &p)
{
    m_d->path = p;
}

QString GeneratedFile::contents() const
{
    return QString::fromUtf8(m_d->contents);
}

void GeneratedFile::setContents(const QString &c)
{
    m_d->contents = c.toUtf8();
}

QByteArray GeneratedFile::binaryContents() const
{
    return m_d->contents;
}

void GeneratedFile::setBinaryContents(const QByteArray &c)
{
    m_d->contents = c;
}

bool GeneratedFile::isBinary() const
{
    return m_d->binary;
}

void GeneratedFile::setBinary(bool b)
{
    m_d->binary = b;
}

QString GeneratedFile::editorId() const
{
    return m_d->editorId;
}

void GeneratedFile::setEditorId(const QString &k)
{
    m_d->editorId = k;
}

bool GeneratedFile::write(QString *errorMessage) const
{
    // Ensure the directory
    const QFileInfo info(m_d->path);
    const QDir dir = info.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            *errorMessage = BaseFileWizard::tr("Unable to create the directory %1.").arg(dir.absolutePath());
            return false;
        }
    }
    // Write out
    QFile file(m_d->path);

    QIODevice::OpenMode flags = QIODevice::WriteOnly|QIODevice::Truncate;
    if (!isBinary())
        flags |= QIODevice::Text;

    if (!file.open(flags)) {
        *errorMessage = BaseFileWizard::tr("Unable to open %1 for writing: %2").arg(m_d->path, file.errorString());
        return false;
    }
    if (file.write(m_d->contents) == -1) {
        *errorMessage =  BaseFileWizard::tr("Error while writing to %1: %2").arg(m_d->path, file.errorString());
        return false;
    }
    file.close();
    return true;
}


// ------------ BaseFileWizardParameterData
class BaseFileWizardParameterData : public QSharedData
{
public:
    explicit BaseFileWizardParameterData(IWizard::WizardKind kind = IWizard::FileWizard);

    IWizard::WizardKind kind;
    QIcon icon;
    QString description;
    QString displayName;
    QString id;
    QString category;
    QString displayCategory;
};

BaseFileWizardParameterData::BaseFileWizardParameterData(IWizard::WizardKind k) :
    kind(k)
{
}

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

void BaseFileWizardParameters::setDisplayCategory(const QString &v)
{
    m_d->displayCategory = v;
}

/* WizardEventLoop: Special event loop that runs a QWizard and terminates if the page changes.
 * Synopsis:
 * \code
    Wizard wizard(parent);
    WizardEventLoop::WizardResult wr;
    do {
        wr = WizardEventLoop::execWizardPage(wizard);
    } while (wr == WizardEventLoop::PageChanged);
 * \endcode */

class WizardEventLoop : public QEventLoop
{
    Q_OBJECT
    Q_DISABLE_COPY(WizardEventLoop)
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

// ---------------- BaseFileWizardPrivate
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
    m_d(new BaseFileWizardPrivate(parameters))
{
}

BaseFileWizard::~BaseFileWizard()
{
    delete m_d;
}

IWizard::WizardKind  BaseFileWizard::kind() const
{
    return m_d->m_parameters.kind();
}

QIcon BaseFileWizard::icon() const
{
    return m_d->m_parameters.icon();
}

QString BaseFileWizard::description() const
{
    return m_d->m_parameters.description();
}

QString BaseFileWizard::displayName() const
{
    return m_d->m_parameters.displayName();
}

QString BaseFileWizard::id() const
{
    return m_d->m_parameters.id();
}

QString BaseFileWizard::category() const
{
    return m_d->m_parameters.category();
}

QString BaseFileWizard::displayCategory() const
{
    return m_d->m_parameters.displayCategory();
}

QStringList BaseFileWizard::runWizard(const QString &path, QWidget *parent)
{
    QTC_ASSERT(!path.isEmpty(), return QStringList())

    typedef  QList<IFileWizardExtension*> ExtensionList;

    QString errorMessage;
    // Compile extension pages, purge out unused ones
    ExtensionList extensions = ExtensionSystem::PluginManager::instance()->getObjects<IFileWizardExtension>();
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
    const QSharedPointer<QWizard> wizard(createWizardDialog(parent, path, allExtensionPages));
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
                ex->firstExtensionPageShown(files);
        if (accepted)
            break;
    }
    if (files.empty())
        return QStringList();
    // Compile result list and prompt for overwrite
    QStringList result;
    foreach (const GeneratedFile &generatedFile, files)
        result.push_back(generatedFile.path());

    switch (promptOverwrite(path, result, &errorMessage)) {
    case OverwriteCanceled:
        return QStringList();
    case OverwriteError:
        QMessageBox::critical(0, tr("Existing files"), errorMessage);
        return QStringList();
    case OverwriteOk:
        break;
    }

    // Write
    foreach (const GeneratedFile &generatedFile, files) {
        if (!generatedFile.write(&errorMessage)) {
            QMessageBox::critical(parent, tr("File Generation Failure"), errorMessage);
            return QStringList();
        }
    }
    // Run the extensions
    foreach (IFileWizardExtension *ex, extensions)
        if (!ex->process(files, &errorMessage)) {
            QMessageBox::critical(parent, tr("File Generation Failure"), errorMessage);
            return QStringList();
        }

    // Post generation handler
    if (!postGenerateFiles(wizard.data(), files, &errorMessage)) {
        QMessageBox::critical(0, tr("File Generation Failure"), errorMessage);
        return QStringList();
    }

    return result;
}

void BaseFileWizard::setupWizard(QWizard *w)
{
    w->setOption(QWizard::NoCancelButton, false);
    w->setOption(QWizard::NoDefaultButton, false);
    w->setOption(QWizard::NoBackButtonOnStartPage, true);
}

bool BaseFileWizard::postGenerateFiles(const QWizard *w, const GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(w);
    // File mode: open the editors in file mode and ensure editor pane
    const Core::GeneratedFiles::const_iterator cend = l.constEnd();
    Core::EditorManager *em = Core::EditorManager::instance();
    for (Core::GeneratedFiles::const_iterator it = l.constBegin(); it != cend; ++it) {
        if (!em->openEditor(it->path(), it->editorId())) {
            *errorMessage = tr("Failed to open an editor for '%1'.").arg(it->path());
            return false;
        }
    }
    em->ensureEditorManagerVisible();
    return true;
}

BaseFileWizard::OverwriteResult BaseFileWizard::promptOverwrite(const QString &location,
                                                                const QStringList &files,
                                                                QString *errorMessage) const
{
    if (debugWizard)
        qDebug() << Q_FUNC_INFO  << location << files;

    bool existingFilesFound = false;
    bool oddStuffFound = false;

    static const QString readOnlyMsg = tr(" [read only]");
    static const QString directoryMsg = tr(" [directory]");
    static const QString symLinkMsg = tr(" [symbolic link]");

    // Format a file list message as ( "<file1> [readonly], <file2> [directory]").
    QString fileNamesMsgPart;
    foreach (const QString &fileName, files) {
        const QFileInfo fi(fileName);
        if (fi.exists()) {
            existingFilesFound = true;
            if (!fileNamesMsgPart.isEmpty())
                fileNamesMsgPart += QLatin1String(", ");
            fileNamesMsgPart += fi.fileName();
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

    if (!existingFilesFound)
        return OverwriteOk;

    if (oddStuffFound) {
        *errorMessage = tr("The project directory %1 contains files which cannot be overwritten:\n%2.").arg(location).arg(fileNamesMsgPart);
        return OverwriteError;
    }

    const QString messageFormat = tr("The following files already exist in the directory %1:\n"
                                     "%2.\nWould you like to overwrite them?");
    const QString message = messageFormat.arg(location).arg(fileNamesMsgPart);
    const bool yes = (QMessageBox::question(Core::ICore::instance()->mainWindow(),
                                            tr("Existing files"), message,
                                            QMessageBox::Yes | QMessageBox::No,
                                            QMessageBox::No)
                      == QMessageBox::Yes);
    return yes ? OverwriteOk :  OverwriteCanceled;
}

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

QString BaseFileWizard::preferredSuffix(const QString &mimeType)
{
    const QString rc = Core::ICore::instance()->mimeDatabase()->preferredSuffixByType(mimeType);
    if (rc.isEmpty())
        qWarning("%s: WARNING: Unable to find a preferred suffix for %s.",
                 Q_FUNC_INFO, mimeType.toUtf8().constData());
    return rc;
}

// ------------- StandardFileWizard

StandardFileWizard::StandardFileWizard(const BaseFileWizardParameters &parameters,
                                       QObject *parent) :
    BaseFileWizard(parameters, parent)
{
}

QWizard *StandardFileWizard::createWizardDialog(QWidget *parent,
                                                const QString &defaultPath,
                                                const WizardPageList &extensionPages) const
{
    Utils::FileWizardDialog *standardWizardDialog = new Utils::FileWizardDialog(parent);
    standardWizardDialog->setWindowTitle(tr("New %1").arg(displayName()));
    setupWizard(standardWizardDialog);
    standardWizardDialog->setPath(defaultPath);
    foreach (QWizardPage *p, extensionPages)
        standardWizardDialog->addPage(p);
    return standardWizardDialog;
}

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
