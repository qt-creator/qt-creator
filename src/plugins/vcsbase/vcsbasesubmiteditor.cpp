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

#include "vcsbasesubmiteditor.h"

#include "commonvcssettings.h"
#include "nicknamedialog.h"
#include "submiteditorfile.h"
#include "submiteditorwidget.h"
#include "submitfieldwidget.h"
#include "submitfilemodel.h"
#include "vcsbaseoutputwindow.h"
#include "vcsplugin.h"

#include <aggregation/aggregate.h>
#include <cplusplus/Control.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Literals.h>
#include <cpptools/ModelManagerInterface.h>
#include <cplusplus/Symbol.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TranslationUnit.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/idocument.h>
#include <coreplugin/mainwindow.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/completingtextedit.h>
#include <utils/checkablemessagebox.h>
#include <utils/synchronousprocess.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <find/basetextfind.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>

#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QStringListModel>
#include <QTextStream>
#include <QStyle>
#include <QToolBar>
#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <QCompleter>
#include <QLineEdit>
#include <QTextEdit>
#include <cstring>

enum { debug = 0 };
enum { wantToolBar = 0 };

// Return true if word is meaningful and can be added to a completion model
static bool acceptsWordForCompletion(const char *word)
{
    if (word == 0)
        return false;
    static const std::size_t minWordLength = 7;
    return std::strlen(word) >= minWordLength;
}

// Return the class name which function belongs to
static const char *belongingClassName(const CPlusPlus::Function *function)
{
    if (function == 0)
        return 0;

    const CPlusPlus::Name *funcName = function->name();
    if (funcName != 0 && funcName->asQualifiedNameId() != 0) {
        const CPlusPlus::Name *funcBaseName = funcName->asQualifiedNameId()->base();
        if (funcBaseName != 0 && funcBaseName->identifier() != 0)
            return funcBaseName->identifier()->chars();
    }

    return 0;
}

/*!
    \struct VcsBase::VcsBaseSubmitEditorParameters

    \brief Utility struct to parametrize a VcsBaseSubmitEditor.
*/

/*!
    \class  VcsBase::VcsBaseSubmitEditor

    \brief Base class for a submit editor based on the SubmitEditorWidget.

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
    signal and then asking the IDocument interface of the editor to save the file
    within a DocumentManager::blockFileChange() section
    and to launch the submit process. In addition, the action registered
    for submit should be connected to a slot triggering the close of the
    current editor in the editor manager.
*/

namespace VcsBase {

using namespace Internal;
using namespace Utils;

static inline QString submitMessageCheckScript()
{
    return VcsPlugin::instance()->settings().submitMessageCheckScript;
}

struct VcsBaseSubmitEditorPrivate
{
    VcsBaseSubmitEditorPrivate(const VcsBaseSubmitEditorParameters *parameters,
                               SubmitEditorWidget *editorWidget,
                               QObject *q);

    SubmitEditorWidget *m_widget;
    QToolBar *m_toolWidget;
    const VcsBaseSubmitEditorParameters *m_parameters;
    QString m_displayName;
    QString m_checkScriptWorkingDirectory;
    SubmitEditorFile *m_file;

    QPointer<QAction> m_diffAction;
    QPointer<QAction> m_submitAction;

    NickNameDialog *m_nickNameDialog;
};

VcsBaseSubmitEditorPrivate::VcsBaseSubmitEditorPrivate(const VcsBaseSubmitEditorParameters *parameters,
                                                       SubmitEditorWidget *editorWidget,
                                                       QObject *q) :
    m_widget(editorWidget),
    m_toolWidget(0),
    m_parameters(parameters),
    m_file(new SubmitEditorFile(QLatin1String(parameters->mimeType), q)),
    m_nickNameDialog(0)
{
    QCompleter *completer = new QCompleter(q);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    m_widget->descriptionEdit()->setCompleter(completer);
    m_widget->descriptionEdit()->setCompletionLengthThreshold(4);
}

VcsBaseSubmitEditor::VcsBaseSubmitEditor(const VcsBaseSubmitEditorParameters *parameters,
                                         SubmitEditorWidget *editorWidget) :
    d(new VcsBaseSubmitEditorPrivate(parameters, editorWidget, this))
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
    connect(d->m_file, SIGNAL(saveMe(QString*,QString,bool)),
            this, SLOT(save(QString*,QString,bool)));

    connect(d->m_widget, SIGNAL(diffSelected(QList<int>)), this, SLOT(slotDiffSelectedVcsFiles(QList<int>)));
    connect(d->m_widget->descriptionEdit(), SIGNAL(textChanged()), this, SLOT(slotDescriptionChanged()));

    const CommonVcsSettings settings = VcsPlugin::instance()->settings();
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
    connect(VcsPlugin::instance(),
            SIGNAL(settingsChanged(VcsBase::Internal::CommonVcsSettings)),
            this, SLOT(slotUpdateEditorSettings(VcsBase::Internal::CommonVcsSettings)));
    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(slotRefreshCommitData()));
    connect(Core::ICore::mainWindow(), SIGNAL(windowActivated()), this, SLOT(slotRefreshCommitData()));

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    aggregate->add(new Find::BaseTextFind(d->m_widget->descriptionEdit()));
    aggregate->add(this);
}

VcsBaseSubmitEditor::~VcsBaseSubmitEditor()
{
    delete d->m_toolWidget;
    delete d->m_widget;
    delete d;
}

void VcsBaseSubmitEditor::slotUpdateEditorSettings(const CommonVcsSettings &s)
{
    setLineWrapWidth(s.lineWrapWidth);
    setLineWrap(s.lineWrap);
}

void VcsBaseSubmitEditor::slotRefreshCommitData()
{
    if (Core::EditorManager::currentEditor() == this)
        updateFileModel();
}

// Return a trimmed list of non-empty field texts
static inline QStringList fieldTexts(const QString &fileContents)
{
    QStringList rc;
    const QStringList rawFields = fileContents.trimmed().split(QLatin1Char('\n'));
    foreach (const QString &field, rawFields) {
        const QString trimmedField = field.trimmed();
        if (!trimmedField.isEmpty())
            rc.push_back(trimmedField);
    }
    return rc;
}

void VcsBaseSubmitEditor::createUserFields(const QString &fieldConfigFile)
{
    Utils::FileReader reader;
    if (!reader.fetch(fieldConfigFile, QIODevice::Text, Core::ICore::mainWindow()))
        return;
    // Parse into fields
    const QStringList fields = fieldTexts(QString::fromUtf8(reader.data()));
    if (fields.empty())
        return;
    // Create a completer on user names
    const QStandardItemModel *nickNameModel = VcsPlugin::instance()->nickNameModel();
    QCompleter *completer = new QCompleter(NickNameDialog::nickNameList(nickNameModel), this);

    SubmitFieldWidget *fieldWidget = new SubmitFieldWidget;
    connect(fieldWidget, SIGNAL(browseButtonClicked(int,QString)),
            this, SLOT(slotSetFieldNickName(int)));
    fieldWidget->setCompleter(completer);
    fieldWidget->setAllowDuplicateFields(true);
    fieldWidget->setHasBrowseButton(true);
    fieldWidget->setFields(fields);
    d->m_widget->addSubmitFieldWidget(fieldWidget);
}

void VcsBaseSubmitEditor::registerActions(QAction *editorUndoAction, QAction *editorRedoAction,
                                          QAction *submitAction, QAction *diffAction)
{
    d->m_widget->registerActions(editorUndoAction, editorRedoAction, submitAction, diffAction);
    d->m_diffAction = diffAction;
    d->m_submitAction = submitAction;
}

void VcsBaseSubmitEditor::unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction, QAction *diffAction)
{
    d->m_widget->unregisterActions(editorUndoAction, editorRedoAction, submitAction, diffAction);
    d->m_diffAction = d->m_submitAction = 0;
}

QAbstractItemView::SelectionMode VcsBaseSubmitEditor::fileListSelectionMode() const
{
    return d->m_widget->fileListSelectionMode();
}

void VcsBaseSubmitEditor::setFileListSelectionMode(QAbstractItemView::SelectionMode sm)
{
    d->m_widget->setFileListSelectionMode(sm);
}

bool VcsBaseSubmitEditor::isEmptyFileListEnabled() const
{
    return d->m_widget->isEmptyFileListEnabled();
}

void VcsBaseSubmitEditor::setEmptyFileListEnabled(bool e)
{
    d->m_widget->setEmptyFileListEnabled(e);
}

bool VcsBaseSubmitEditor::lineWrap() const
{
    return d->m_widget->lineWrap();
}

void VcsBaseSubmitEditor::setLineWrap(bool w)
{
    d->m_widget->setLineWrap(w);
}

int VcsBaseSubmitEditor::lineWrapWidth() const
{
    return d->m_widget->lineWrapWidth();
}

void VcsBaseSubmitEditor::setLineWrapWidth(int w)
{
    d->m_widget->setLineWrapWidth(w);
}

void VcsBaseSubmitEditor::slotDescriptionChanged()
{
}

bool VcsBaseSubmitEditor::createNew(const QString &contents)
{
    setFileContents(contents);
    return true;
}

bool VcsBaseSubmitEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
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

Core::IDocument *VcsBaseSubmitEditor::document()
{
    return d->m_file;
}

QString VcsBaseSubmitEditor::displayName() const
{
    if (d->m_displayName.isEmpty())
        d->m_displayName = QCoreApplication::translate("VCS", d->m_parameters->displayName);
    return d->m_displayName;
}

void VcsBaseSubmitEditor::setDisplayName(const QString &title)
{
    d->m_displayName = title;
    emit changed();
}

QString VcsBaseSubmitEditor::checkScriptWorkingDirectory() const
{
    return d->m_checkScriptWorkingDirectory;
}

void VcsBaseSubmitEditor::setCheckScriptWorkingDirectory(const QString &s)
{
    d->m_checkScriptWorkingDirectory = s;
}

bool VcsBaseSubmitEditor::duplicateSupported() const
{
    return false;
}

Core::IEditor *VcsBaseSubmitEditor::duplicate(QWidget * /*parent*/)
{
    return 0;
}

Core::Id VcsBaseSubmitEditor::id() const
{
    return Core::Id(QByteArray(d->m_parameters->id));
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

QWidget *VcsBaseSubmitEditor::toolBar()
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

QByteArray VcsBaseSubmitEditor::saveState() const
{
    return QByteArray();
}

bool VcsBaseSubmitEditor::restoreState(const QByteArray &/*state*/)
{
    return true;
}

QStringList VcsBaseSubmitEditor::checkedFiles() const
{
    return d->m_widget->checkedFiles();
}

void VcsBaseSubmitEditor::setFileModel(SubmitFileModel *model, const QString &repositoryDirectory)
{
    QTC_ASSERT(model, return);
    if (SubmitFileModel *oldModel = d->m_widget->fileModel()) {
        model->updateSelections(oldModel);
        delete oldModel;
    }
    d->m_widget->setFileModel(model);

    QSet<QString> uniqueSymbols;
    const CPlusPlus::Snapshot cppSnapShot = CPlusPlus::CppModelManagerInterface::instance()->snapshot();

    // Iterate over the files and get interesting symbols
    for (int row = 0; row < model->rowCount(); ++row) {
        const QFileInfo fileInfo(repositoryDirectory, model->file(row));

        // Add file name
        uniqueSymbols.insert(fileInfo.fileName());

        const QString filePath = fileInfo.absoluteFilePath();
        // Add symbols from the C++ code model
        const CPlusPlus::Document::Ptr doc = cppSnapShot.document(filePath);
        if (!doc.isNull() && doc->control() != 0) {
            const CPlusPlus::Control *ctrl = doc->control();
            CPlusPlus::Symbol **symPtr = ctrl->firstSymbol(); // Read-only
            while (symPtr != ctrl->lastSymbol()) {
                const CPlusPlus::Symbol *sym = *symPtr;

                const CPlusPlus::Identifier *symId = sym->identifier();
                // Add any class, function or namespace identifiers
                if ((sym->isClass() || sym->isFunction() || sym->isNamespace())
                        && (symId != 0 && acceptsWordForCompletion(symId->chars())))
                {
                    uniqueSymbols.insert(QString::fromUtf8(symId->chars()));
                }

                // Handle specific case : get "Foo" in "void Foo::function() {}"
                if (sym->isFunction() && !sym->asFunction()->isDeclaration()) {
                    const char *className = belongingClassName(sym->asFunction());
                    if (acceptsWordForCompletion(className))
                        uniqueSymbols.insert(QString::fromUtf8(className));
                }

                ++symPtr;
            }
        }
    }

    // Populate completer with symbols
    if (!uniqueSymbols.isEmpty()) {
        QCompleter *completer = d->m_widget->descriptionEdit()->completer();
        QStringList symbolsList = uniqueSymbols.toList();
        symbolsList.sort();
        completer->setModel(new QStringListModel(symbolsList, completer));
    }
}

SubmitFileModel *VcsBaseSubmitEditor::fileModel() const
{
    return d->m_widget->fileModel();
}

QStringList VcsBaseSubmitEditor::rowsToFiles(const QList<int> &rows) const
{
    if (rows.empty())
        return QStringList();

    QStringList rc;
    const SubmitFileModel *model = fileModel();
    const int count = rows.size();
    for (int i = 0; i < count; i++)
        rc.push_back(model->file(rows.at(i)));
    return rc;
}

void VcsBaseSubmitEditor::slotDiffSelectedVcsFiles(const QList<int> &rawList)
{
    if (d->m_parameters->diffType == VcsBaseSubmitEditorParameters::DiffRows)
        emit diffSelectedFiles(rawList);
    else
        emit diffSelectedFiles(rowsToFiles(rawList));
}

bool VcsBaseSubmitEditor::save(QString *errorString, const QString &fileName, bool autoSave)
{
    const QString fName = fileName.isEmpty() ? d->m_file->fileName() : fileName;
    Utils::FileSaver saver(fName, QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    saver.write(fileContents());
    if (!saver.finalize(errorString))
        return false;
    if (autoSave)
        return true;
    const QFileInfo fi(fName);
    d->m_file->setFileName(fi.absoluteFilePath());
    d->m_file->setModified(false);
    return true;
}

QByteArray VcsBaseSubmitEditor::fileContents() const
{
    return d->m_widget->descriptionText().toLocal8Bit();
}

bool VcsBaseSubmitEditor::setFileContents(const QString &contents)
{
    d->m_widget->setDescriptionText(contents);
    return true;
}

bool VcsBaseSubmitEditor::isDescriptionMandatory() const
{
    return d->m_widget->isDescriptionMandatory();
}

void VcsBaseSubmitEditor::setDescriptionMandatory(bool v)
{
    d->m_widget->setDescriptionMandatory(v);
}

enum { checkDialogMinimumWidth = 500 };

VcsBaseSubmitEditor::PromptSubmitResult
        VcsBaseSubmitEditor::promptSubmit(const QString &title,
                                          const QString &question,
                                          const QString &checkFailureQuestion,
                                          bool *promptSetting,
                                          bool forcePrompt,
                                          bool canCommitOnFailure) const
{
    SubmitEditorWidget *submitWidget =
            static_cast<SubmitEditorWidget *>(const_cast<VcsBaseSubmitEditor *>(this)->widget());

    raiseSubmitEditor();

    QString errorMessage;
    QMessageBox::StandardButton answer = QMessageBox::Yes;

    const bool prompt = forcePrompt || *promptSetting;

    QWidget *parent = Core::ICore::mainWindow();
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

QString VcsBaseSubmitEditor::promptForNickName()
{
    if (!d->m_nickNameDialog)
        d->m_nickNameDialog = new NickNameDialog(VcsPlugin::instance()->nickNameModel(), d->m_widget);
    if (d->m_nickNameDialog->exec() == QDialog::Accepted)
       return d->m_nickNameDialog->nickName();
    return QString();
}

void VcsBaseSubmitEditor::slotInsertNickName()
{
    const QString nick = promptForNickName();
    if (!nick.isEmpty())
        d->m_widget->descriptionEdit()->textCursor().insertText(nick);
}

void VcsBaseSubmitEditor::slotSetFieldNickName(int i)
{
    if (SubmitFieldWidget *sfw = d->m_widget->submitFieldWidgets().front()) {
        const QString nick = promptForNickName();
        if (!nick.isEmpty())
            sfw->setFieldValue(i, nick);
    }
}

void VcsBaseSubmitEditor::slotCheckSubmitMessage()
{
    QString errorMessage;
    if (!checkSubmitMessage(&errorMessage)) {
        QMessageBox msgBox(QMessageBox::Warning, tr("Submit Message Check Failed"),
                           errorMessage, QMessageBox::Ok, d->m_widget);
        msgBox.setMinimumWidth(checkDialogMinimumWidth);
        msgBox.exec();
    }
}

bool VcsBaseSubmitEditor::checkSubmitMessage(QString *errorMessage) const
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
           VcsBaseSubmitEditor::tr("Executing %1").arg(nativeCmd) :
           VcsBaseSubmitEditor::tr("Executing [%1] %2").
           arg(QDir::toNativeSeparators(workingDir), nativeCmd);
}

bool VcsBaseSubmitEditor::runSubmitMessageCheckScript(const QString &checkScript, QString *errorMessage) const
{
    // Write out message
    QString tempFilePattern = QDir::tempPath();
    if (!tempFilePattern.endsWith(QDir::separator()))
        tempFilePattern += QDir::separator();
    tempFilePattern += QLatin1String("msgXXXXXX.txt");
    TempFileSaver saver(tempFilePattern);
    saver.write(fileContents());
    if (!saver.finalize(errorMessage))
        return false;
    // Run check process
    VcsBaseOutputWindow *outputWindow = VcsBaseOutputWindow::instance();
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
    if (!SynchronousProcess::readDataFromProcess(checkProcess, 30000, &stdOutData, &stdErrData, false)) {
        SynchronousProcess::stopProcess(checkProcess);
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

QIcon VcsBaseSubmitEditor::diffIcon()
{
    return QIcon(QLatin1String(":/vcsbase/images/diff.png"));
}

QIcon VcsBaseSubmitEditor::submitIcon()
{
    return QIcon(QLatin1String(":/vcsbase/images/submit.png"));
}

// Compile a list if files in the current projects. TODO: Recurse down qrc files?
QStringList VcsBaseSubmitEditor::currentProjectFiles(bool nativeSeparators, QString *name)
{
    if (name)
        name->clear();

    if (const ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectExplorerPlugin::currentProject()) {
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
    return QStringList();
}

// Reduce a list of untracked files reported by a VCS down to the files
// that are actually part of the current project(s).
void VcsBaseSubmitEditor::filterUntrackedFilesOfProject(const QString &repositoryDirectory, QStringList *untrackedFiles)
{
    if (untrackedFiles->empty())
        return;
    const QStringList nativeProjectFiles = VcsBaseSubmitEditor::currentProjectFiles(true);
    if (nativeProjectFiles.empty())
        return;
    const QDir repoDir(repositoryDirectory);
    for (QStringList::iterator it = untrackedFiles->begin(); it != untrackedFiles->end(); ) {
        const QString path = QDir::toNativeSeparators(repoDir.absoluteFilePath(*it));
        if (nativeProjectFiles.contains(path))
            ++it;
        else
            it = untrackedFiles->erase(it);
    }
}

// Helper to raise an already open submit editor to prevent opening twice.
bool VcsBaseSubmitEditor::raiseSubmitEditor()
{
    // Nothing to do?
    if (Core::IEditor *ce = Core::EditorManager::currentEditor())
        if (qobject_cast<VcsBaseSubmitEditor*>(ce))
            return true;
    // Try to activate a hidden one
    Core::EditorManager *em = Core::EditorManager::instance();
    foreach (Core::IEditor *e, em->openedEditors()) {
        if (qobject_cast<VcsBaseSubmitEditor*>(e)) {
            Core::EditorManager::activateEditor(e,
                Core::EditorManager::IgnoreNavigationHistory | Core::EditorManager::ModeSwitch);
            return true;
        }
    }
    return false;
}

} // namespace VcsBase
