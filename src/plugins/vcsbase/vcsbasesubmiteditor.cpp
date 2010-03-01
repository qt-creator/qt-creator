/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "vcsbasesubmiteditor.h"
#include "vcsbaseoutputwindow.h"
#include "vcsbasesettings.h"
#include "vcsplugin.h"
#include "nicknamedialog.h"
#include "submiteditorfile.h"

#include <aggregation/aggregate.h>
#include <coreplugin/ifile.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <utils/submiteditorwidget.h>
#include <utils/checkablemessagebox.h>
#include <utils/synchronousprocess.h>
#include <utils/submitfieldwidget.h>
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
    QList<int> m_contexts;

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
    m_file(new VCSBase::Internal::SubmitEditorFile(QLatin1String(m_parameters->mimeType), q)),
    m_nickNameDialog(0)
{
    m_contexts << Core::UniqueIDManager::instance()->uniqueIdentifier(m_parameters->context);
}

VCSBaseSubmitEditor::VCSBaseSubmitEditor(const VCSBaseSubmitEditorParameters *parameters,
                                         Utils::SubmitEditorWidget *editorWidget) :
    m_d(new VCSBaseSubmitEditorPrivate(parameters, editorWidget, this))
{
    // Message font according to settings
    const TextEditor::FontSettings fs = TextEditor::TextEditorSettings::instance()->fontSettings();
    QFont font = editorWidget->descriptionEdit()->font();
    font.setFamily(fs.family());
    font.setPointSize(fs.fontSize());
    editorWidget->descriptionEdit()->setFont(font);

    m_d->m_file->setModified(false);
    // We are always clean to prevent the editor manager from asking to save.
    connect(m_d->m_file, SIGNAL(saveMe(QString)), this, SLOT(save(QString)));

    connect(m_d->m_widget, SIGNAL(diffSelected(QStringList)), this, SLOT(slotDiffSelectedVCSFiles(QStringList)));
    connect(m_d->m_widget->descriptionEdit(), SIGNAL(textChanged()), this, SLOT(slotDescriptionChanged()));

    const Internal::VCSBaseSettings settings = Internal::VCSPlugin::instance()->settings();
    // Add additional context menu settings
    if (!settings.submitMessageCheckScript.isEmpty() || !settings.nickNameMailMap.isEmpty()) {
        QAction *sep = new QAction(this);
        sep->setSeparator(true);
        m_d->m_widget->addDescriptionEditContextMenuAction(sep);
        // Run check action
        if (!settings.submitMessageCheckScript.isEmpty()) {
            QAction *checkAction = new QAction(tr("Check message"), this);
            connect(checkAction, SIGNAL(triggered()), this, SLOT(slotCheckSubmitMessage()));
            m_d->m_widget->addDescriptionEditContextMenuAction(checkAction);
        }
        // Insert nick
        if (!settings.nickNameMailMap.isEmpty()) {
            QAction *insertAction = new QAction(tr("Insert name..."), this);
            connect(insertAction, SIGNAL(triggered()), this, SLOT(slotInsertNickName()));
            m_d->m_widget->addDescriptionEditContextMenuAction(insertAction);
        }
    }
    // Do we have user fields?
    if (!settings.nickNameFieldListFile.isEmpty())
        createUserFields(settings.nickNameFieldListFile);

    // wrapping. etc
    slotUpdateEditorSettings(settings);
    connect(Internal::VCSPlugin::instance(),
            SIGNAL(settingsChanged(VCSBase::Internal::VCSBaseSettings)),
            this, SLOT(slotUpdateEditorSettings(VCSBase::Internal::VCSBaseSettings)));

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    aggregate->add(new Find::BaseTextFind(m_d->m_widget->descriptionEdit()));
    aggregate->add(this);
}

VCSBaseSubmitEditor::~VCSBaseSubmitEditor()
{
    delete m_d->m_toolWidget;
    delete m_d->m_widget;
    delete m_d;
}

void VCSBaseSubmitEditor::slotUpdateEditorSettings(const Internal::VCSBaseSettings &s)
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
    QFile fieldFile(fieldConfigFile);
    if (!fieldFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        qWarning("%s: Unable to open %s: %s", Q_FUNC_INFO, qPrintable(fieldConfigFile), qPrintable(fieldFile.errorString()));
        return;
    }
    // Parse into fields
    const QStringList fields = fieldTexts(QString::fromUtf8(fieldFile.readAll()));
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
    m_d->m_widget->addSubmitFieldWidget(fieldWidget);
}

void VCSBaseSubmitEditor::registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                                          QAction *submitAction, QAction *diffAction)\
{
    m_d->m_widget->registerActions(editorUndoAction, editorRedoAction, submitAction, diffAction);
    m_d->m_diffAction = diffAction;
    m_d->m_submitAction = submitAction;
}

void VCSBaseSubmitEditor::unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction, QAction *diffAction)
{
    m_d->m_widget->unregisterActions(editorUndoAction, editorRedoAction, submitAction, diffAction);
    m_d->m_diffAction = m_d->m_submitAction = 0;
}
int VCSBaseSubmitEditor::fileNameColumn() const
{
    return m_d->m_widget->fileNameColumn();
}

void VCSBaseSubmitEditor::setFileNameColumn(int c)
{
    m_d->m_widget->setFileNameColumn(c);
}

QAbstractItemView::SelectionMode VCSBaseSubmitEditor::fileListSelectionMode() const
{
    return m_d->m_widget->fileListSelectionMode();
}

void VCSBaseSubmitEditor::setFileListSelectionMode(QAbstractItemView::SelectionMode sm)
{
    m_d->m_widget->setFileListSelectionMode(sm);
}

bool VCSBaseSubmitEditor::lineWrap() const
{
    return m_d->m_widget->lineWrap();
}

void VCSBaseSubmitEditor::setLineWrap(bool w)
{
    m_d->m_widget->setLineWrap(w);
}

int VCSBaseSubmitEditor::lineWrapWidth() const
{
    return m_d->m_widget->lineWrapWidth();
}

void VCSBaseSubmitEditor::setLineWrapWidth(int w)
{
    m_d->m_widget->setLineWrapWidth(w);
}

void VCSBaseSubmitEditor::slotDescriptionChanged()
{
}

bool VCSBaseSubmitEditor::createNew(const QString &contents)
{
    setFileContents(contents);
    return true;
}

bool VCSBaseSubmitEditor::open(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;

    const QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable())
        return false;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        qWarning("Unable to open %s: %s", qPrintable(fileName), qPrintable(file.errorString()));
        return false;
    }

    const QString text = QString::fromLocal8Bit(file.readAll());
    if (!createNew(text))
        return false;

    m_d->m_file->setFileName(fi.absoluteFilePath());
    return true;
}

Core::IFile *VCSBaseSubmitEditor::file()
{
    return m_d->m_file;
}

QString VCSBaseSubmitEditor::displayName() const
{
    if (m_d->m_displayName.isEmpty())
        m_d->m_displayName = QCoreApplication::translate("VCS", m_d->m_parameters->displayName);
    return m_d->m_displayName;
}

void VCSBaseSubmitEditor::setDisplayName(const QString &title)
{
    m_d->m_displayName = title;
}

QString VCSBaseSubmitEditor::checkScriptWorkingDirectory() const
{
    return m_d->m_checkScriptWorkingDirectory;
}

void VCSBaseSubmitEditor::setCheckScriptWorkingDirectory(const QString &s)
{
    m_d->m_checkScriptWorkingDirectory = s;
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
    return m_d->m_parameters->id;
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

    if (m_d->m_toolWidget)
        return m_d->m_toolWidget;

    if (!m_d->m_diffAction && !m_d->m_submitAction)
        return 0;

    // Create
    m_d->m_toolWidget = createToolBar(m_d->m_widget, m_d->m_submitAction, m_d->m_diffAction);
    return m_d->m_toolWidget;
}

QList<int> VCSBaseSubmitEditor::context() const
{
    return m_d->m_contexts;
}

QWidget *VCSBaseSubmitEditor::widget()
{
    return m_d->m_widget;
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
    return m_d->m_widget->checkedFiles();
}

void VCSBaseSubmitEditor::setFileModel(QAbstractItemModel *m)
{
    m_d->m_widget->setFileModel(m);
}

QAbstractItemModel *VCSBaseSubmitEditor::fileModel() const
{
    return m_d->m_widget->fileModel();
}

void VCSBaseSubmitEditor::slotDiffSelectedVCSFiles(const QStringList &rawList)
{
     emit diffSelectedFiles(rawList);
}

bool VCSBaseSubmitEditor::save(const QString &fileName)
{
    const QString fName = fileName.isEmpty() ? m_d->m_file->fileName() : fileName;
    QFile file(fName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning("Unable to open %s: %s", qPrintable(fName), qPrintable(file.errorString()));
        return false;
    }
    file.write(fileContents().toLocal8Bit());
    if (!file.flush())
        return false;
    file.close();
    const QFileInfo fi(fName);
    m_d->m_file->setFileName(fi.absoluteFilePath());
    m_d->m_file->setModified(false);
    return true;
}

QString VCSBaseSubmitEditor::fileContents() const
{
    return m_d->m_widget->descriptionText();
}

bool VCSBaseSubmitEditor::setFileContents(const QString &contents)
{
    m_d->m_widget->setDescriptionText(contents);
    return true;
}

enum { checkDialogMinimumWidth = 500 };

VCSBaseSubmitEditor::PromptSubmitResult
        VCSBaseSubmitEditor::promptSubmit(const QString &title,
                                          const QString &question,
                                          const QString &checkFailureQuestion,
                                          bool *promptSetting,
                                          bool forcePrompt) const
{
    QString errorMessage;
    QMessageBox::StandardButton answer = QMessageBox::Yes;

    const bool prompt = forcePrompt || *promptSetting;

    QWidget *parent = Core::ICore::instance()->mainWindow();
    // Pop up a message depending on whether the check succeeded and the
    // user wants to be prompted
    if (checkSubmitMessage(&errorMessage)) {
        // Check ok, do prompt?
        if (prompt) {
            // Provide check box to turn off prompt ONLY if it was not forced
            if (*promptSetting && !forcePrompt) {
                const QDialogButtonBox::StandardButton danswer =
                        Utils::CheckableMessageBox::question(parent, title, question,
                                                                   tr("Prompt to submit"), promptSetting,
                                                                   QDialogButtonBox::Yes|QDialogButtonBox::No|QDialogButtonBox::Cancel,
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
    switch (answer) {
    case QMessageBox::Cancel:
        return SubmitCanceled;
    case QMessageBox::No:
        return SubmitDiscarded;
    default:
        break;
    }
    return SubmitConfirmed;
}

QString VCSBaseSubmitEditor::promptForNickName()
{
    if (!m_d->m_nickNameDialog)
        m_d->m_nickNameDialog = new Internal::NickNameDialog(Internal::VCSPlugin::instance()->nickNameModel(), m_d->m_widget);
    if (m_d->m_nickNameDialog->exec() == QDialog::Accepted)
       return m_d->m_nickNameDialog->nickName();
    return QString();
}

void VCSBaseSubmitEditor::slotInsertNickName()
{
    const QString nick = promptForNickName();
    if (!nick.isEmpty())
        m_d->m_widget->descriptionEdit()->textCursor().insertText(nick);
}

void VCSBaseSubmitEditor::slotSetFieldNickName(int i)
{
    if (Utils::SubmitFieldWidget *sfw  =m_d->m_widget->submitFieldWidgets().front()) {
        const QString nick = promptForNickName();
        if (!nick.isEmpty())
            sfw->setFieldValue(i, nick);
    }
}

void VCSBaseSubmitEditor::slotCheckSubmitMessage()
{
    QString errorMessage;
    if (!checkSubmitMessage(&errorMessage)) {
        QMessageBox msgBox(QMessageBox::Warning, tr("Submit Message Check failed"),
                           errorMessage, QMessageBox::Ok, m_d->m_widget);
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
    return workingDir.isEmpty() ?
           VCSBaseSubmitEditor::tr("Executing %1").arg(cmd) :
           VCSBaseSubmitEditor::tr("Executing [%1] %2").arg(workingDir, cmd);
}

bool VCSBaseSubmitEditor::runSubmitMessageCheckScript(const QString &checkScript, QString *errorMessage) const
{
    // Write out message
    QString tempFilePattern = QDir::tempPath();
    if (!tempFilePattern.endsWith(QDir::separator()))
        tempFilePattern += QDir::separator();
    tempFilePattern += QLatin1String("msgXXXXXX.txt");
    QTemporaryFile messageFile(tempFilePattern);
    messageFile.setAutoRemove(true);
    if (!messageFile.open()) {
        *errorMessage = tr("Unable to open '%1': %2").arg(messageFile.fileName(), messageFile.errorString());
        return false;
    }
    const QString messageFileName = messageFile.fileName();
    messageFile.write(fileContents().toUtf8());
    messageFile.close();
    // Run check process
    VCSBaseOutputWindow *outputWindow = VCSBaseOutputWindow::instance();
    outputWindow->appendCommand(msgCheckScript(m_d->m_checkScriptWorkingDirectory, checkScript));
    QProcess checkProcess;
    if (!m_d->m_checkScriptWorkingDirectory.isEmpty())
        checkProcess.setWorkingDirectory(m_d->m_checkScriptWorkingDirectory);
    checkProcess.start(checkScript, QStringList(messageFileName));
    checkProcess.closeWriteChannel();
    if (!checkProcess.waitForStarted()) {
        *errorMessage = tr("The check script '%1' could not be started: %2").arg(checkScript, checkProcess.errorString());
        return false;
    }
    QByteArray stdOutData;
    QByteArray stdErrData;
    if (!Utils::SynchronousProcess::readDataFromProcess(checkProcess, 30000, &stdOutData, &stdErrData)) {
        Utils::SynchronousProcess::stopProcess(checkProcess);
        *errorMessage = tr("The check script '%1' timed out.").arg(checkScript);
        return false;
    }
    if (checkProcess.exitStatus() != QProcess::NormalExit) {
        *errorMessage = tr("The check script '%1' crashed").arg(checkScript);
        return false;
    }
    if (!stdOutData.isEmpty())
        outputWindow->appendSilently(QString::fromLocal8Bit(stdOutData));
    const QString stdErr = QString::fromLocal8Bit(stdErrData);
    if (!stdErr.isEmpty())
        outputWindow->appendSilently(stdErr);
    const int exitCode = checkProcess.exitCode();
    if (exitCode != 0) {
        const QString exMessage = tr("The check script returned exit code %1.").arg(exitCode);
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
            em->activateEditor(e, Core::EditorManager::IgnoreNavigationHistory);
            return true;
        }
    }
    return false;
}

} // namespace VCSBase
