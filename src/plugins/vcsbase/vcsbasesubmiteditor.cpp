/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "vcsbasesubmiteditor.h"

#include "commonvcssettings.h"
#include "vcsbaseoutputwindow.h"
#include "vcsplugin.h"
#include "nicknamedialog.h"
#include "submiteditorfile.h"

#include <aggregation/aggregate.h>
#include <coreplugin/ifile.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/id.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <utils/submiteditorwidget.h>
#include <utils/checkablemessagebox.h>
#include <utils/synchronousprocess.h>
#include <utils/submitfieldwidget.h>
#include <utils/fileutils.h>
#include <find/basetextfind.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QProcess>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QTextStream>
#include <QtGui/QStyle>
#include <QtGui/QToolBar>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtGui/QCompleter>
#include <QtGui/QLineEdit>
#include <QtGui/QTextEdit>

enum { debug = 0 };
enum { wantToolBar = 0 };

/*!
    \struct VCSBase::VCSBaseSubmitEditorParameters

    \brief Utility struct to parametrize a VCSBaseSubmitEditor.
*/

/*!
    \class  VCSBase::VCSBaseSubmitEditor

    \brief Base class for a submit editor based on the Utils::SubmitEditorWidget.

    Presents the commit message in a text editor and an
    checkable list of modified files in a list window. The user can delete
    files from the list by pressing unchecking them or diff the selection
    by doubleclicking.

    The action matching the the ids (unless 0) of the parameter struct will be
    registered with the EditorWidget and submit/diff actions will be added to
    a toolbar.

    For the given context, there must be only one instance of the editor
    active.
    To start a submit, set the submit template on the editor and the output
    of the VCS status command listing the modified files as fileList and open
    it.

    The submit process is started by listening on the editor close
    signal and then asking the IFile interface of the editor to save the file
    within a IFileManager::blockFileChange() section
    and to launch the submit process. In addition, the action registered
    for submit should be connected to a slot triggering the close of the
    current editor in the editor manager.
*/

namespace VCSBase {

static inline QString submitMessageCheckScript()
{
    return Internal::VCSPlugin::instance()->settings().submitMessageCheckScript;
}

struct VCSBaseSubmitEditorPrivate
{
    VCSBaseSubmitEditorPrivate(const VCSBaseSubmitEditorParameters *parameters,
                               Utils::SubmitEditorWidget *editorWidget,
                               QObject *q);

    Utils::SubmitEditorWidget *m_widget;
    QToolBar *m_toolWidget;
    const VCSBaseSubmitEditorParameters *m_parameters;
    QString m_displayName;
    QString m_checkScriptWorkingDirectory;
    VCSBase::Internal::SubmitEditorFile *m_file;

    QPointer<QAction> m_diffAction;
    QPointer<QAction> m_submitAction;

    Internal::NickNameDialog *m_nickNameDialog;
};

VCSBaseSubmitEditorPrivate::VCSBaseSubmitEditorPrivate(const VCSBaseSubmitEditorParameters *parameters,
                                                       Utils::SubmitEditorWidget *editorWidget,
                                                       QObject *q) :
    m_widget(editorWidget),
    m_toolWidget(0),
    m_parameters(parameters),
    m_file(new VCSBase::Internal::SubmitEditorFile(QLatin1String(parameters->mimeType), q)),
    m_nickNameDialog(0)
{
}

VCSBaseSubmitEditor::VCSBaseSubmitEditor(const VCSBaseSubmitEditorParameters *parameters,
                                         Utils::SubmitEditorWidget *editorWidget) :
    d(new VCSBaseSubmitEditorPrivate(parameters, editorWidget, this))
{
    setContext(Core::Context(parameters->context));
    setWidget(d->m_widget);

    // Message font according to settings
    const TextEditor::FontSettings fs = TextEditor::TextEditorSettings::instance()->fontSettings();
    QFont font = editorWidget->descriptionEdit()->font();
    font.setFamily(fs.family());
    font.setPointSize(fs.fontSize());
    editorWidget->descriptionEdit()->setFont(font);

    d->m_file->setModified(false);
    // We are always clean to prevent the editor manager from asking to save.
    connect(d->m_file, SIGNAL(saveMe(QString*, QString, bool)),
            this, SLOT(save(QString*, QString, bool)));

    connect(d->m_widget, SIGNAL(diffSelected(QStringList)), this, SLOT(slotDiffSelectedVCSFiles(QStringList)));
    connect(d->m_widget->descriptionEdit(), SIGNAL(textChanged()), this, SLOT(slotDescriptionChanged()));

    const Internal::CommonVcsSettings settings = Internal::VCSPlugin::instance()->settings();
    // Add additional context menu settings
    if (!settings.submitMessageCheckScript.isEmpty() || !settings.nickNameMailMap.isEmpty()) {
        QAction *sep = new QAction(this);
        sep->setSeparator(true);
        d->m_widget->addDescriptionEditContextMenuAction(sep);
        // Run check action
        if (!settings.submitMessageCheckScript.isEmpty()) {
            QAction *checkAction = new QAction(tr("Check Message"), this);
            connect(checkAction, SIGNAL(triggered()), this, SLOT(slotCheckSubmitMessage()));
            d->m_widget->addDescriptionEditContextMenuAction(checkAction);
        }
        // Insert nick
        if (!settings.nickNameMailMap.isEmpty()) {
            QAction *insertAction = new QAction(tr("Insert Name..."), this);
            connect(insertAction, SIGNAL(triggered()), this, SLOT(slotInsertNickName()));
            d->m_widget->addDescriptionEditContextMenuAction(insertAction);
        }
    }
    // Do we have user fields?
    if (!settings.nickNameFieldListFile.isEmpty())
        createUserFields(settings.nickNameFieldListFile);

    // wrapping. etc
    slotUpdateEditorSettings(settings);
    connect(Internal::VCSPlugin::instance(),
            SIGNAL(settingsChanged(VCSBase::Internal::CommonVcsSettings)),
            this, SLOT(slotUpdateEditorSettings(VCSBase::Internal::CommonVcsSettings)));

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    aggregate->add(new Find::BaseTextFind(d->m_widget->descriptionEdit()));
    aggregate->add(this);
}

VCSBaseSubmitEditor::~VCSBaseSubmitEditor()
{
    delete d->m_toolWidget;
    delete d->m_widget;
    delete d;
}

void VCSBaseSubmitEditor::slotUpdateEditorSettings(const Internal::CommonVcsSettings &s)
{
    setLineWrapWidth(s.lineWrapWidth);
    setLineWrap(s.lineWrap);
}

// Return a trimmed list of non-empty field texts
static inline QStringList fieldTexts(const QString &fileContents)
{
    QStringList rc;
    const QStringList rawFields = fileContents.trimmed().split(QLatin1Char('\n'));
    foreach(const QString &field, rawFields) {
        const QString trimmedField = field.trimmed();
        if (!trimmedField.isEmpty())
            rc.push_back(trimmedField);
    }
    return rc;
}

void VCSBaseSubmitEditor::createUserFields(const QString &fieldConfigFile)
{
    Utils::FileReader reader;
    if (!reader.fetch(fieldConfigFile, QIODevice::Text, Core::ICore::instance()->mainWindow()))
        return;
    // Parse into fields
    const QStringList fields = fieldTexts(QString::fromUtf8(reader.data()));
    if (fields.empty())
        return;
    // Create a completer on user names
    const QStandardItemModel *nickNameModel = Internal::VCSPlugin::instance()->nickNameModel();
    QCompleter *completer = new QCompleter(Internal::NickNameDialog::nickNameList(nickNameModel), this);

    Utils::SubmitFieldWidget *fieldWidget = new Utils::SubmitFieldWidget;
    connect(fieldWidget, SIGNAL(browseButtonClicked(int,QString)),
            this, SLOT(slotSetFieldNickName(int)));
    fieldWidget->setCompleter(completer);
    fieldWidget->setAllowDuplicateFields(true);
    fieldWidget->setHasBrowseButton(true);
    fieldWidget->setFields(fields);
    d->m_widget->addSubmitFieldWidget(fieldWidget);
}

void VCSBaseSubmitEditor::registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                                          QAction *submitAction, QAction *diffAction)\
{
    d->m_widget->registerActions(editorUndoAction, editorRedoAction, submitAction, diffAction);
    d->m_diffAction = diffAction;
    d->m_submitAction = submitAction;
}

void VCSBaseSubmitEditor::unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction, QAction *diffAction)
{
    d->m_widget->unregisterActions(editorUndoAction, editorRedoAction, submitAction, diffAction);
    d->m_diffAction = d->m_submitAction = 0;
}

int VCSBaseSubmitEditor::fileNameColumn() const
{
    return d->m_widget->fileNameColumn();
}

void VCSBaseSubmitEditor::setFileNameColumn(int c)
{
    d->m_widget->setFileNameColumn(c);
}

QAbstractItemView::SelectionMode VCSBaseSubmitEditor::fileListSelectionMode() const
{
    return d->m_widget->fileListSelectionMode();
}

void VCSBaseSubmitEditor::setFileListSelectionMode(QAbstractItemView::SelectionMode sm)
{
    d->m_widget->setFileListSelectionMode(sm);
}

bool VCSBaseSubmitEditor::isEmptyFileListEnabled() const
{
    return d->m_widget->isEmptyFileListEnabled();
}

void VCSBaseSubmitEditor::setEmptyFileListEnabled(bool e)
{
    d->m_widget->setEmptyFileListEnabled(e);
}

bool VCSBaseSubmitEditor::lineWrap() const
{
    return d->m_widget->lineWrap();
}

void VCSBaseSubmitEditor::setLineWrap(bool w)
{
    d->m_widget->setLineWrap(w);
}

int VCSBaseSubmitEditor::lineWrapWidth() const
{
    return d->m_widget->lineWrapWidth();
}

void VCSBaseSubmitEditor::setLineWrapWidth(int w)
{
    d->m_widget->setLineWrapWidth(w);
}

void VCSBaseSubmitEditor::slotDescriptionChanged()
{
}

bool VCSBaseSubmitEditor::createNew(const QString &contents)
{
    setFileContents(contents);
    return true;
}

bool VCSBaseSubmitEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (fileName.isEmpty())
        return false;

    Utils::FileReader reader;
    if (!reader.fetch(realFileName, QIODevice::Text, errorString))
        return false;

    const QString text = QString::fromLocal8Bit(reader.data());
    if (!createNew(text))
        return false;

    d->m_file->setFileName(QFileInfo(fileName).absoluteFilePath());
    d->m_file->setModified(fileName != realFileName);
    return true;
}

Core::IFile *VCSBaseSubmitEditor::file()
{
    return d->m_file;
}

QString VCSBaseSubmitEditor::displayName() const
{
    if (d->m_displayName.isEmpty())
        d->m_displayName = QCoreApplication::translate("VCS", d->m_parameters->displayName);
    return d->m_displayName;
}

void VCSBaseSubmitEditor::setDisplayName(const QString &title)
{
    d->m_displayName = title;
    emit changed();
}

QString VCSBaseSubmitEditor::checkScriptWorkingDirectory() const
{
    return d->m_checkScriptWorkingDirectory;
}

void VCSBaseSubmitEditor::setCheckScriptWorkingDirectory(const QString &s)
{
    d->m_checkScriptWorkingDirectory = s;
}

bool VCSBaseSubmitEditor::duplicateSupported() const
{
    return false;
}

Core::IEditor *VCSBaseSubmitEditor::duplicate(QWidget * /*parent*/)
{
    return 0;
}

QString VCSBaseSubmitEditor::id() const
{
    return d->m_parameters->id;
}

static QToolBar *createToolBar(const QWidget *someWidget, QAction *submitAction, QAction *diffAction)
{
    // Create
    QToolBar *toolBar = new QToolBar;
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    const int size = someWidget->style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolBar->setIconSize(QSize(size, size));
    toolBar->addSeparator();

    if (submitAction)
        toolBar->addAction(submitAction);
    if (diffAction)
        toolBar->addAction(diffAction);
    return toolBar;
}

QWidget *VCSBaseSubmitEditor::toolBar()
{
    if (!wantToolBar)
        return 0;

    if (d->m_toolWidget)
        return d->m_toolWidget;

    if (!d->m_diffAction && !d->m_submitAction)
        return 0;

    // Create
    d->m_toolWidget = createToolBar(d->m_widget, d->m_submitAction, d->m_diffAction);
    return d->m_toolWidget;
}

QByteArray VCSBaseSubmitEditor::saveState() const
{
    return QByteArray();
}

bool VCSBaseSubmitEditor::restoreState(const QByteArray &/*state*/)
{
    return true;
}

QStringList VCSBaseSubmitEditor::checkedFiles() const
{
    return d->m_widget->checkedFiles();
}

void VCSBaseSubmitEditor::setFileModel(QAbstractItemModel *m)
{
    d->m_widget->setFileModel(m);
}

QAbstractItemModel *VCSBaseSubmitEditor::fileModel() const
{
    return d->m_widget->fileModel();
}

void VCSBaseSubmitEditor::slotDiffSelectedVCSFiles(const QStringList &rawList)
{
     emit diffSelectedFiles(rawList);
}

bool VCSBaseSubmitEditor::save(QString *errorString, const QString &fileName, bool autoSave)
{
    const QString fName = fileName.isEmpty() ? d->m_file->fileName() : fileName;
    Utils::FileSaver saver(fName, QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    saver.write(fileContents().toLocal8Bit());
    if (!saver.finalize(errorString))
        return false;
    if (autoSave)
        return true;
    const QFileInfo fi(fName);
    d->m_file->setFileName(fi.absoluteFilePath());
    d->m_file->setModified(false);
    return true;
}

QString VCSBaseSubmitEditor::fileContents() const
{
    return d->m_widget->descriptionText();
}

bool VCSBaseSubmitEditor::setFileContents(const QString &contents)
{
    d->m_widget->setDescriptionText(contents);
    return true;
}

enum { checkDialogMinimumWidth = 500 };

VCSBaseSubmitEditor::PromptSubmitResult
        VCSBaseSubmitEditor::promptSubmit(const QString &title,
                                          const QString &question,
                                          const QString &checkFailureQuestion,
                                          bool *promptSetting,
                                          bool forcePrompt,
                                          bool canCommitOnFailure) const
{
    Utils::SubmitEditorWidget *submitWidget =
            static_cast<Utils::SubmitEditorWidget *>(const_cast<VCSBaseSubmitEditor *>(this)->widget());

    raiseSubmitEditor();

    QString errorMessage;
    QMessageBox::StandardButton answer = QMessageBox::Yes;

    const bool prompt = forcePrompt || *promptSetting;

    QWidget *parent = Core::ICore::instance()->mainWindow();
    // Pop up a message depending on whether the check succeeded and the
    // user wants to be prompted
    bool canCommit = checkSubmitMessage(&errorMessage) && submitWidget->canSubmit();
    if (canCommit) {
        // Check ok, do prompt?
        if (prompt) {
            // Provide check box to turn off prompt ONLY if it was not forced
            if (*promptSetting && !forcePrompt) {
                const QDialogButtonBox::StandardButton danswer =
                        Utils::CheckableMessageBox::question(parent, title, question,
                                                                   tr("Prompt to submit"), promptSetting,
                                                                   QDialogButtonBox::Yes|QDialogButtonBox::No|
                                                                   QDialogButtonBox::Cancel,
                                                                   QDialogButtonBox::Yes);
                answer = Utils::CheckableMessageBox::dialogButtonBoxToMessageBoxButton(danswer);
            } else {
                answer = QMessageBox::question(parent, title, question,
                                               QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
                                               QMessageBox::Yes);
            }
        }
    } else {
        // Check failed.
        QMessageBox msgBox(QMessageBox::Question, title, checkFailureQuestion,
                           QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, parent);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setInformativeText(errorMessage);
        msgBox.setMinimumWidth(checkDialogMinimumWidth);
        answer = static_cast<QMessageBox::StandardButton>(msgBox.exec());
    }
    if (!canCommit && !canCommitOnFailure) {
        switch (answer) {
        case QMessageBox::No:
                return SubmitDiscarded;
        case QMessageBox::Yes:
                return SubmitCanceled;
        default:
            break;
        }
    } else {
        switch (answer) {
        case QMessageBox::No:
            return SubmitDiscarded;
        case QMessageBox::Yes:
            return SubmitConfirmed;
        default:
            break;
        }
    }

    return SubmitCanceled;
}

QString VCSBaseSubmitEditor::promptForNickName()
{
    if (!d->m_nickNameDialog)
        d->m_nickNameDialog = new Internal::NickNameDialog(Internal::VCSPlugin::instance()->nickNameModel(), d->m_widget);
    if (d->m_nickNameDialog->exec() == QDialog::Accepted)
       return d->m_nickNameDialog->nickName();
    return QString();
}

void VCSBaseSubmitEditor::slotInsertNickName()
{
    const QString nick = promptForNickName();
    if (!nick.isEmpty())
        d->m_widget->descriptionEdit()->textCursor().insertText(nick);
}

void VCSBaseSubmitEditor::slotSetFieldNickName(int i)
{
    if (Utils::SubmitFieldWidget *sfw  =d->m_widget->submitFieldWidgets().front()) {
        const QString nick = promptForNickName();
        if (!nick.isEmpty())
            sfw->setFieldValue(i, nick);
    }
}

void VCSBaseSubmitEditor::slotCheckSubmitMessage()
{
    QString errorMessage;
    if (!checkSubmitMessage(&errorMessage)) {
        QMessageBox msgBox(QMessageBox::Warning, tr("Submit Message Check Failed"),
                           errorMessage, QMessageBox::Ok, d->m_widget);
        msgBox.setMinimumWidth(checkDialogMinimumWidth);
        msgBox.exec();
    }
}

bool VCSBaseSubmitEditor::checkSubmitMessage(QString *errorMessage) const
{
    const QString checkScript = submitMessageCheckScript();
    if (checkScript.isEmpty())
        return true;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool rc = runSubmitMessageCheckScript(checkScript, errorMessage);
    QApplication::restoreOverrideCursor();
    return rc;
}

static inline QString msgCheckScript(const QString &workingDir, const QString &cmd)
{
    const QString nativeCmd = QDir::toNativeSeparators(cmd);
    return workingDir.isEmpty() ?
           VCSBaseSubmitEditor::tr("Executing %1").arg(nativeCmd) :
           VCSBaseSubmitEditor::tr("Executing [%1] %2").
           arg(QDir::toNativeSeparators(workingDir), nativeCmd);
}

bool VCSBaseSubmitEditor::runSubmitMessageCheckScript(const QString &checkScript, QString *errorMessage) const
{
    // Write out message
    QString tempFilePattern = QDir::tempPath();
    if (!tempFilePattern.endsWith(QDir::separator()))
        tempFilePattern += QDir::separator();
    tempFilePattern += QLatin1String("msgXXXXXX.txt");
    Utils::TempFileSaver saver(tempFilePattern);
    saver.write(fileContents().toUtf8());
    if (!saver.finalize(errorMessage))
        return false;
    // Run check process
    VCSBaseOutputWindow *outputWindow = VCSBaseOutputWindow::instance();
    outputWindow->appendCommand(msgCheckScript(d->m_checkScriptWorkingDirectory, checkScript));
    QProcess checkProcess;
    if (!d->m_checkScriptWorkingDirectory.isEmpty())
        checkProcess.setWorkingDirectory(d->m_checkScriptWorkingDirectory);
    checkProcess.start(checkScript, QStringList(saver.fileName()));
    checkProcess.closeWriteChannel();
    if (!checkProcess.waitForStarted()) {
        *errorMessage = tr("The check script '%1' could not be started: %2").arg(checkScript, checkProcess.errorString());
        return false;
    }
    QByteArray stdOutData;
    QByteArray stdErrData;
    if (!Utils::SynchronousProcess::readDataFromProcess(checkProcess, 30000, &stdOutData, &stdErrData, false)) {
        Utils::SynchronousProcess::stopProcess(checkProcess);
        *errorMessage = tr("The check script '%1' timed out.").
                        arg(QDir::toNativeSeparators(checkScript));
        return false;
    }
    if (checkProcess.exitStatus() != QProcess::NormalExit) {
        *errorMessage = tr("The check script '%1' crashed.").
                        arg(QDir::toNativeSeparators(checkScript));
        return false;
    }
    if (!stdOutData.isEmpty())
        outputWindow->appendSilently(QString::fromLocal8Bit(stdOutData));
    const QString stdErr = QString::fromLocal8Bit(stdErrData);
    if (!stdErr.isEmpty())
        outputWindow->appendSilently(stdErr);
    const int exitCode = checkProcess.exitCode();
    if (exitCode != 0) {
        const QString exMessage = tr("The check script returned exit code %1.").
                                  arg(exitCode);
        outputWindow->appendError(exMessage);
        *errorMessage = stdErr;
        if (errorMessage->isEmpty())
            *errorMessage = exMessage;
        return false;
    }
    return true;
}

QIcon VCSBaseSubmitEditor::diffIcon()
{
    return QIcon(QLatin1String(":/vcsbase/images/diff.png"));
}

QIcon VCSBaseSubmitEditor::submitIcon()
{
    return QIcon(QLatin1String(":/vcsbase/images/submit.png"));
}

// Compile a list if files in the current projects. TODO: Recurse down qrc files?
QStringList VCSBaseSubmitEditor::currentProjectFiles(bool nativeSeparators, QString *name)
{
    if (name)
        name->clear();

    if (ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance()) {
        if (const ProjectExplorer::Project *currentProject = pe->currentProject()) {
            QStringList files = currentProject->files(ProjectExplorer::Project::ExcludeGeneratedFiles);
            if (name)
                *name = currentProject->displayName();
            if (nativeSeparators && !files.empty()) {
                const QStringList::iterator end = files.end();
                for (QStringList::iterator it = files.begin(); it != end; ++it)
                    *it = QDir::toNativeSeparators(*it);
            }
            return files;
        }
    }
    return QStringList();
}

// Reduce a list of untracked files reported by a VCS down to the files
// that are actually part of the current project(s).
void VCSBaseSubmitEditor::filterUntrackedFilesOfProject(const QString &repositoryDirectory, QStringList *untrackedFiles)
{
    if (untrackedFiles->empty())
        return;
    const QStringList nativeProjectFiles = VCSBase::VCSBaseSubmitEditor::currentProjectFiles(true);
    if (nativeProjectFiles.empty())
        return;
    const QDir repoDir(repositoryDirectory);
    for (QStringList::iterator it = untrackedFiles->begin(); it != untrackedFiles->end(); ) {
        const QString path = QDir::toNativeSeparators(repoDir.absoluteFilePath(*it));
        if (nativeProjectFiles.contains(path)) {
            ++it;
        } else {
            it = untrackedFiles->erase(it);
        }
    }
}

// Helper to raise an already open submit editor to prevent opening twice.
bool VCSBaseSubmitEditor::raiseSubmitEditor()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    // Nothing to do?
    if (Core::IEditor *ce = em->currentEditor())
        if (qobject_cast<VCSBaseSubmitEditor*>(ce))
            return true;
    // Try to activate a hidden one
    foreach (Core::IEditor *e, em->openedEditors()) {
        if (qobject_cast<VCSBaseSubmitEditor*>(e)) {
            em->activateEditor(e, Core::EditorManager::IgnoreNavigationHistory | Core::EditorManager::ModeSwitch);
            return true;
        }
    }
    return false;
}

} // namespace VCSBase
