/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "vcsbasesubmiteditor.h"

#include "commonvcssettings.h"
#include "nicknamedialog.h"
#include "submiteditorfile.h"
#include "submiteditorwidget.h"
#include "submitfieldwidget.h"
#include "submitfilemodel.h"
#include "vcsoutputwindow.h"
#include "vcsplugin.h"
#include "vcsprojectcache.h"

#include <aggregation/aggregate.h>
#include <cpptools/cppmodelmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/checkablemessagebox.h>
#include <utils/completingtextedit.h>
#include <utils/synchronousprocess.h>
#include <utils/fileutils.h>
#include <utils/icon.h>
#include <utils/theme/theme.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>
#include <coreplugin/find/basetextfind.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QProcess>
#include <QSet>
#include <QStringListModel>
#include <QStyle>
#include <QToolBar>
#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <QCompleter>

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
    \class VcsBase::VcsBaseSubmitEditorParameters

    \brief The VcsBaseSubmitEditorParameters class is a utility class
    to parametrize a VcsBaseSubmitEditor.
*/

/*!
    \class  VcsBase::VcsBaseSubmitEditor

    \brief The VcsBaseSubmitEditor class is the base class for a submit editor
    based on the SubmitEditorWidget.

    Presents the commit message in a text editor and an
    checkable list of modified files in a list window. The user can delete
    files from the list by pressing unchecking them or diff the selection
    by doubleclicking.

    The action matching the ids (unless 0) of the parameter struct will be
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

class VcsBaseSubmitEditorPrivate
{
public:
    VcsBaseSubmitEditorPrivate(const VcsBaseSubmitEditorParameters *parameters,
                               SubmitEditorWidget *editorWidget,
                               VcsBaseSubmitEditor *q);

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
                                                       VcsBaseSubmitEditor *q) :
    m_widget(editorWidget),
    m_toolWidget(0),
    m_parameters(parameters),
    m_file(new SubmitEditorFile(parameters, q)),
    m_nickNameDialog(0)
{
    auto completer = new QCompleter(q);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    m_widget->descriptionEdit()->setCompleter(completer);
    m_widget->descriptionEdit()->setCompletionLengthThreshold(4);
}

VcsBaseSubmitEditor::VcsBaseSubmitEditor(const VcsBaseSubmitEditorParameters *parameters,
                                         SubmitEditorWidget *editorWidget) :
    d(new VcsBaseSubmitEditorPrivate(parameters, editorWidget, this))
{
    setWidget(d->m_widget);
    document()->setPreferredDisplayName(QCoreApplication::translate("VCS", d->m_parameters->displayName));

    // Message font according to settings
    CompletingTextEdit *descriptionEdit = editorWidget->descriptionEdit();
    const TextEditor::FontSettings fs = TextEditor::TextEditorSettings::fontSettings();
    const QTextCharFormat tf = fs.toTextCharFormat(TextEditor::C_TEXT);
    descriptionEdit->setFont(tf.font());
    const QTextCharFormat selectionFormat = fs.toTextCharFormat(TextEditor::C_SELECTION);
    QPalette pal;
    pal.setColor(QPalette::Base, tf.background().color());
    pal.setColor(QPalette::Text, tf.foreground().color());
    pal.setColor(QPalette::Foreground, tf.foreground().color());
    if (selectionFormat.background().style() != Qt::NoBrush)
        pal.setColor(QPalette::Highlight, selectionFormat.background().color());
    pal.setBrush(QPalette::HighlightedText, selectionFormat.foreground());
    descriptionEdit->setPalette(pal);

    d->m_file->setModified(false);
    // We are always clean to prevent the editor manager from asking to save.

    connect(d->m_widget, &SubmitEditorWidget::diffSelected,
            this, &VcsBaseSubmitEditor::slotDiffSelectedVcsFiles);
    connect(descriptionEdit, &QTextEdit::textChanged,
            this, &VcsBaseSubmitEditor::fileContentsChanged);

    const CommonVcsSettings settings = VcsPlugin::instance()->settings();
    // Add additional context menu settings
    if (!settings.submitMessageCheckScript.isEmpty() || !settings.nickNameMailMap.isEmpty()) {
        auto sep = new QAction(this);
        sep->setSeparator(true);
        d->m_widget->addDescriptionEditContextMenuAction(sep);
        // Run check action
        if (!settings.submitMessageCheckScript.isEmpty()) {
            auto checkAction = new QAction(tr("Check Message"), this);
            connect(checkAction, &QAction::triggered,
                    this, &VcsBaseSubmitEditor::slotCheckSubmitMessage);
            d->m_widget->addDescriptionEditContextMenuAction(checkAction);
        }
        // Insert nick
        if (!settings.nickNameMailMap.isEmpty()) {
            auto insertAction = new QAction(tr("Insert Name..."), this);
            connect(insertAction, &QAction::triggered, this, &VcsBaseSubmitEditor::slotInsertNickName);
            d->m_widget->addDescriptionEditContextMenuAction(insertAction);
        }
    }
    // Do we have user fields?
    if (!settings.nickNameFieldListFile.isEmpty())
        createUserFields(settings.nickNameFieldListFile);

    // wrapping. etc
    slotUpdateEditorSettings(settings);
    connect(VcsPlugin::instance(), &VcsPlugin::settingsChanged,
            this, &VcsBaseSubmitEditor::slotUpdateEditorSettings);
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, [this]() {
                if (Core::EditorManager::currentEditor() == this)
                    updateFileModel();
            });
    connect(qApp, &QApplication::applicationStateChanged,
            this, [this](Qt::ApplicationState state) {
                if (state == Qt::ApplicationActive)
                    updateFileModel();
            });

    auto aggregate = new Aggregation::Aggregate;
    aggregate->add(new Core::BaseTextFind(descriptionEdit));
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
    FileReader reader;
    if (!reader.fetch(fieldConfigFile, QIODevice::Text, Core::ICore::mainWindow()))
        return;
    // Parse into fields
    const QStringList fields = fieldTexts(QString::fromUtf8(reader.data()));
    if (fields.empty())
        return;
    // Create a completer on user names
    const QStandardItemModel *nickNameModel = VcsPlugin::instance()->nickNameModel();
    auto completer = new QCompleter(NickNameDialog::nickNameList(nickNameModel), this);

    auto fieldWidget = new SubmitFieldWidget;
    connect(fieldWidget, &SubmitFieldWidget::browseButtonClicked,
            this, &VcsBaseSubmitEditor::slotSetFieldNickName);
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

Core::IDocument *VcsBaseSubmitEditor::document()
{
    return d->m_file;
}

QString VcsBaseSubmitEditor::checkScriptWorkingDirectory() const
{
    return d->m_checkScriptWorkingDirectory;
}

void VcsBaseSubmitEditor::setCheckScriptWorkingDirectory(const QString &s)
{
    d->m_checkScriptWorkingDirectory = s;
}

static QToolBar *createToolBar(const QWidget *someWidget, QAction *submitAction, QAction *diffAction)
{
    // Create
    auto toolBar = new QToolBar;
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

QStringList VcsBaseSubmitEditor::checkedFiles() const
{
    return d->m_widget->checkedFiles();
}

void VcsBaseSubmitEditor::setFileModel(SubmitFileModel *model)
{
    QTC_ASSERT(model, return);
    SubmitFileModel *oldModel = d->m_widget->fileModel();
    QList<int> selected;
    if (oldModel) {
        model->updateSelections(oldModel);
        selected = d->m_widget->selectedRows();
    }
    d->m_widget->setFileModel(model);
    delete oldModel;
    if (!selected.isEmpty())
        d->m_widget->setSelectedRows(selected);

    QSet<QString> uniqueSymbols;
    const CPlusPlus::Snapshot cppSnapShot = CppTools::CppModelManager::instance()->snapshot();

    // Iterate over the files and get interesting symbols
    for (int row = 0; row < model->rowCount(); ++row) {
        const QFileInfo fileInfo(model->repositoryRoot(), model->file(row));

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
        emit diffSelectedRows(rawList);
    else
        emit diffSelectedFiles(rowsToFiles(rawList));
}

QByteArray VcsBaseSubmitEditor::fileContents() const
{
    return description().toLocal8Bit();
}

bool VcsBaseSubmitEditor::setFileContents(const QByteArray &contents)
{
    setDescription(QString::fromUtf8(contents));
    return true;
}

QString VcsBaseSubmitEditor::description() const
{
    return d->m_widget->descriptionText();
}

void VcsBaseSubmitEditor::setDescription(const QString &text)
{
    d->m_widget->setDescriptionText(text);
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
                                          bool canCommitOnFailure)
{
    SubmitEditorWidget *submitWidget = static_cast<SubmitEditorWidget *>(this->widget());

    Core::EditorManager::activateEditor(this, Core::EditorManager::IgnoreNavigationHistory);

    if (!submitWidget->isEnabled())
        return SubmitDiscarded;

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
                        CheckableMessageBox::question(parent, title, question,
                                                                   tr("Prompt to submit"), promptSetting,
                                                                   QDialogButtonBox::Yes|QDialogButtonBox::No|
                                                                   QDialogButtonBox::Cancel,
                                                                   QDialogButtonBox::Yes);
                answer = CheckableMessageBox::dialogButtonBoxToMessageBoxButton(danswer);
            } else {
                answer = QMessageBox::question(parent, title, question,
                                               QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
                                               QMessageBox::Yes);
            }
        }
    } else {
        // Check failed.
        QMessageBox::StandardButtons buttons;
        if (canCommitOnFailure)
            buttons = QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel;
        else
            buttons = QMessageBox::Yes|QMessageBox::No;
        QMessageBox msgBox(QMessageBox::Question, title, checkFailureQuestion,
                           buttons, parent);
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
    TempFileSaver saver(Utils::TemporaryDirectory::masterDirectoryPath() + "/msgXXXXXX.txt");
    saver.write(fileContents());
    if (!saver.finalize(errorMessage))
        return false;
    // Run check process
    VcsOutputWindow::appendShellCommandLine(msgCheckScript(d->m_checkScriptWorkingDirectory,
                                                           checkScript));
    QProcess checkProcess;
    if (!d->m_checkScriptWorkingDirectory.isEmpty())
        checkProcess.setWorkingDirectory(d->m_checkScriptWorkingDirectory);
    checkProcess.start(checkScript, QStringList(saver.fileName()));
    checkProcess.closeWriteChannel();
    if (!checkProcess.waitForStarted()) {
        *errorMessage = tr("The check script \"%1\" could not be started: %2").arg(checkScript, checkProcess.errorString());
        return false;
    }
    QByteArray stdOutData;
    QByteArray stdErrData;
    if (!SynchronousProcess::readDataFromProcess(checkProcess, 30, &stdOutData, &stdErrData, false)) {
        SynchronousProcess::stopProcess(checkProcess);
        *errorMessage = tr("The check script \"%1\" timed out.").
                        arg(QDir::toNativeSeparators(checkScript));
        return false;
    }
    if (checkProcess.exitStatus() != QProcess::NormalExit) {
        *errorMessage = tr("The check script \"%1\" crashed.").
                        arg(QDir::toNativeSeparators(checkScript));
        return false;
    }
    if (!stdOutData.isEmpty())
        VcsOutputWindow::appendSilently(QString::fromLocal8Bit(stdOutData));
    const QString stdErr = QString::fromLocal8Bit(stdErrData);
    if (!stdErr.isEmpty())
        VcsOutputWindow::appendSilently(stdErr);
    const int exitCode = checkProcess.exitCode();
    if (exitCode != 0) {
        const QString exMessage = tr("The check script returned exit code %1.").
                                  arg(exitCode);
        VcsOutputWindow::appendError(exMessage);
        *errorMessage = stdErr;
        if (errorMessage->isEmpty())
            *errorMessage = exMessage;
        return false;
    }
    return true;
}

QIcon VcsBaseSubmitEditor::diffIcon()
{
    using namespace Utils;
    return Icon({
        {":/vcsbase/images/diff_documents.png", Theme::PanelTextColorDark},
        {":/vcsbase/images/diff_arrows.png", Theme::IconsStopColor}
    }, Icon::Tint).icon();
}

QIcon VcsBaseSubmitEditor::submitIcon()
{
    using namespace Utils;
    return Icon({
        {":/vcsbase/images/submit_db.png", Theme::PanelTextColorDark},
        {":/vcsbase/images/submit_arrow.png", Theme::IconsRunColor}
    }, Icon::Tint | Icon::PunchEdges).icon();
}

// Reduce a list of untracked files reported by a VCS down to the files
// that are actually part of the current project(s).
void VcsBaseSubmitEditor::filterUntrackedFilesOfProject(const QString &repositoryDirectory,
                                                        QStringList *untrackedFiles)
{
    const QDir repoDir(repositoryDirectory);
    for (QStringList::iterator it = untrackedFiles->begin(); it != untrackedFiles->end(); ) {
        const QString path = repoDir.absoluteFilePath(*it);
        if (ProjectExplorer::SessionManager::projectForFile(FileName::fromString(path)))
            ++it;
        else
            it = untrackedFiles->erase(it);
    }
}

} // namespace VcsBase
