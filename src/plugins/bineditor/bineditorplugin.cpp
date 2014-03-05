/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "bineditorplugin.h"
#include "bineditor.h"
#include "bineditorconstants.h"

#include <coreplugin/icore.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QVariant>

#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QToolBar>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/ifindsupport.h>
#include <coreplugin/idocument.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/reloadpromptutils.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace BINEditor;
using namespace BINEditor::Internal;


class BinEditorFind : public Core::IFindSupport
{
    Q_OBJECT

public:
    BinEditorFind(BinEditorWidget *widget)
    {
        m_widget = widget;
        m_incrementalStartPos = m_contPos = -1;
        m_incrementalWrappedState = false;
    }

    bool supportsReplace() const { return false; }
    QString currentFindString() const { return QString(); }
    QString completedFindString() const { return QString(); }

    Core::FindFlags supportedFindFlags() const
    {
        return FindBackward | FindCaseSensitively;
    }

    void resetIncrementalSearch()
    {
        m_incrementalStartPos = m_contPos = -1;
        m_incrementalWrappedState = false;
    }

    virtual void highlightAll(const QString &txt, Core::FindFlags findFlags)
    {
        m_widget->highlightSearchResults(txt.toLatin1(), textDocumentFlagsForFindFlags(findFlags));
    }

    void clearResults()
    {
        m_widget->highlightSearchResults(QByteArray());
    }

    int find(const QByteArray &pattern, int pos, Core::FindFlags findFlags, bool *wrapped)
    {
        if (wrapped)
            *wrapped = false;
        if (pattern.isEmpty()) {
            m_widget->setCursorPosition(pos);
            return pos;
        }

        int res = m_widget->find(pattern, pos, textDocumentFlagsForFindFlags(findFlags));
        if (res < 0) {
            pos = (findFlags & FindBackward) ? -1 : 0;
            res = m_widget->find(pattern, pos, textDocumentFlagsForFindFlags(findFlags));
            if (res < 0)
                return res;
            if (wrapped)
                *wrapped = true;
        }
        return res;
    }

    Result findIncremental(const QString &txt, Core::FindFlags findFlags) {
        QByteArray pattern = txt.toLatin1();
        if (pattern != m_lastPattern)
            resetIncrementalSearch(); // Because we don't search for nibbles.
        m_lastPattern = pattern;
        if (m_incrementalStartPos < 0)
            m_incrementalStartPos = m_widget->selectionStart();
        if (m_contPos == -1)
            m_contPos = m_incrementalStartPos;
        bool wrapped;
        int found = find(pattern, m_contPos, findFlags, &wrapped);
        if (wrapped != m_incrementalWrappedState && (found >= 0)) {
            m_incrementalWrappedState = wrapped;
            showWrapIndicator(m_widget);
        }
        Result result;
        if (found >= 0) {
            result = Found;
            m_widget->highlightSearchResults(pattern, textDocumentFlagsForFindFlags(findFlags));
            m_contPos = -1;
        } else {
            if (found == -2) {
                result = NotYetFound;
                m_contPos +=
                        findFlags & FindBackward
                        ? -BinEditorWidget::SearchStride : BinEditorWidget::SearchStride;
            } else {
                result = NotFound;
                m_contPos = -1;
                m_widget->highlightSearchResults(QByteArray(), 0);
            }
        }
        return result;
    }

    Result findStep(const QString &txt, Core::FindFlags findFlags) {
        QByteArray pattern = txt.toLatin1();
        bool wasReset = (m_incrementalStartPos < 0);
        if (m_contPos == -1) {
            m_contPos = m_widget->cursorPosition();
            if (findFlags & FindBackward)
                m_contPos = m_widget->selectionStart()-1;
        }
        bool wrapped;
        int found = find(pattern, m_contPos, findFlags, &wrapped);
        if (wrapped)
            showWrapIndicator(m_widget);
        Result result;
        if (found >= 0) {
            result = Found;
            m_incrementalStartPos = found;
            m_contPos = -1;
            if (wasReset)
                m_widget->highlightSearchResults(pattern, textDocumentFlagsForFindFlags(findFlags));
        } else if (found == -2) {
            result = NotYetFound;
            m_contPos += findFlags & FindBackward
                         ? -BinEditorWidget::SearchStride : BinEditorWidget::SearchStride;
        } else {
            result = NotFound;
            m_contPos = -1;
        }

        return result;
    }

private:
    BinEditorWidget *m_widget;
    int m_incrementalStartPos;
    int m_contPos; // Only valid if last result was NotYetFound.
    bool m_incrementalWrappedState;
    QByteArray m_lastPattern;
};


class BinEditorDocument : public Core::IDocument
{
    Q_OBJECT
public:
    BinEditorDocument(BinEditorWidget *parent) :
        Core::IDocument(parent)
    {
        setId(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
        m_widget = parent;
        connect(m_widget, SIGNAL(dataRequested(quint64)),
            this, SLOT(provideData(quint64)));
        connect(m_widget, SIGNAL(newRangeRequested(quint64)),
            this, SLOT(provideNewRange(quint64)));
    }
    ~BinEditorDocument() {}

    QString mimeType() const {
        return QLatin1String(BINEditor::Constants::C_BINEDITOR_MIMETYPE);
    }

    bool setContents(const QByteArray &contents)
    {
        if (!contents.isEmpty())
            return false;
        m_widget->clear();
        return true;
    }

    bool save(QString *errorString, const QString &fn, bool autoSave)
    {
        QTC_ASSERT(!autoSave, return true); // bineditor does not support autosave - it would be a bit expensive
        const QString fileNameToUse
                = fn.isEmpty() ? filePath() : fn;
        if (m_widget->save(errorString, filePath(), fileNameToUse)) {
            setFilePath(fileNameToUse);
            return true;
        } else {
            return false;
        }
    }

    bool open(QString *errorString, const QString &fileName, quint64 offset = 0) {
        QFile file(fileName);
        quint64 size = static_cast<quint64>(file.size());
        if (size == 0 && !fileName.isEmpty()) {
            QString msg = tr("The Binary Editor cannot open empty files.");
            if (errorString)
                *errorString = msg;
            else
                QMessageBox::critical(Core::ICore::mainWindow(), tr("File Error"), msg);
            return false;
        }
        if (offset >= size)
            return false;
        if (file.open(QIODevice::ReadOnly)) {
            file.close();
            setFilePath(fileName);
            m_widget->setSizes(offset, file.size());
            return true;
        }
        QString errStr = tr("Cannot open %1: %2").arg(
                QDir::toNativeSeparators(fileName), file.errorString());
        if (errorString)
            *errorString = errStr;
        else
            QMessageBox::critical(Core::ICore::mainWindow(), tr("File Error"), errStr);
        return false;
    }

private slots:
    void provideData(quint64 block)
    {
        const QString fn = filePath();
        if (fn.isEmpty())
            return;
        QFile file(fn);
        if (file.open(QIODevice::ReadOnly)) {
            int blockSize = m_widget->dataBlockSize();
            file.seek(block * blockSize);
            QByteArray data = file.read(blockSize);
            file.close();
            const int dataSize = data.size();
            if (dataSize != blockSize)
                data += QByteArray(blockSize - dataSize, 0);
            m_widget->addData(block, data);
        } else {
            QMessageBox::critical(Core::ICore::mainWindow(), tr("File Error"),
                                  tr("Cannot open %1: %2").arg(
                                        QDir::toNativeSeparators(fn), file.errorString()));
        }
    }

    void provideNewRange(quint64 offset)
    {
        open(0, filePath(), offset);
    }

public:

    QString defaultPath() const { return QString(); }

    QString suggestedFileName() const { return QString(); }

    bool isModified() const { return isTemporary()/*e.g. memory view*/ ? false
                                                                       : m_widget->isModified(); }

    bool isFileReadOnly() const {
        const QString fn = filePath();
        if (fn.isEmpty())
            return false;
        const QFileInfo fi(fn);
        return !fi.isWritable();
    }

    bool isSaveAsAllowed() const { return true; }

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) {
        if (flag == FlagIgnore)
            return true;
        if (type == TypePermissions) {
            emit changed();
        } else {
            emit aboutToReload();
            int cPos = m_widget->cursorPosition();
            m_widget->clear();
            const bool success = open(errorString, filePath());
            m_widget->setCursorPosition(cPos);
            emit reloadFinished(success);
            return success;
        }
        return true;
    }

private:
    BinEditorWidget *m_widget;
};

class BinEditor : public Core::IEditor
{
    Q_OBJECT
public:
    BinEditor(BinEditorWidget *widget)
    {
        setWidget(widget);
        m_widget = widget;
        m_file = new BinEditorDocument(m_widget);
        m_context.add(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
        m_context.add(BINEditor::Constants::C_BINEDITOR);
        m_addressEdit = new QLineEdit;
        QRegExpValidator * const addressValidator
            = new QRegExpValidator(QRegExp(QLatin1String("[0-9a-fA-F]{1,16}")),
                m_addressEdit);
        m_addressEdit->setValidator(addressValidator);

        QHBoxLayout *l = new QHBoxLayout;
        QWidget *w = new QWidget;
        l->setMargin(0);
        l->setContentsMargins(0, 0, 5, 0);
        l->addStretch(1);
        l->addWidget(m_addressEdit);
        w->setLayout(l);

        m_toolBar = new QToolBar;
        m_toolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        m_toolBar->addWidget(w);

        widget->setEditor(this);

        connect(m_widget, SIGNAL(cursorPositionChanged(int)), SLOT(updateCursorPosition(int)));
        connect(m_addressEdit, SIGNAL(editingFinished()), SLOT(jumpToAddress()));
        connect(m_widget, SIGNAL(modificationChanged(bool)), m_file, SIGNAL(changed()));
        updateCursorPosition(m_widget->cursorPosition());
    }

    ~BinEditor() {
        delete m_widget;
    }

    bool open(QString *errorString, const QString &fileName, const QString &realFileName) {
        QTC_ASSERT(fileName == realFileName, return false); // The bineditor can do no autosaving
        return m_file->open(errorString, fileName);
    }
    Core::IDocument *document() { return m_file; }

    QWidget *toolBar() { return m_toolBar; }

private slots:
    void updateCursorPosition(int position) {
        m_addressEdit->setText(QString::number(m_widget->baseAddress() + position, 16));
    }

    void jumpToAddress() {
        m_widget->jumpToAddress(m_addressEdit->text().toULongLong(0, 16));
        updateCursorPosition(m_widget->cursorPosition());
    }

private:
    BinEditorWidget *m_widget;
    BinEditorDocument *m_file;
    QToolBar *m_toolBar;
    QLineEdit *m_addressEdit;
};



///////////////////////////////// BinEditorFactory //////////////////////////////////

BinEditorFactory::BinEditorFactory(BinEditorPlugin *owner) :
    m_owner(owner)
{
    setId(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
    setDisplayName(qApp->translate("OpenWith::Editors", Constants::C_BINEDITOR_DISPLAY_NAME));
    addMimeType(Constants::C_BINEDITOR_MIMETYPE);
}

Core::IEditor *BinEditorFactory::createEditor()
{
    BinEditorWidget *widget = new BinEditorWidget();
    BinEditor *editor = new BinEditor(widget);

    m_owner->initializeEditor(widget);
    return editor;
}


/*!
   \class BINEditor::BinEditorWidgetFactory
   \brief The BinEditorWidgetFactory class offers a service registered with
   PluginManager to create bin editor widgets for plugins
   without direct linkage.

   \sa ExtensionSystem::PluginManager::getObjectByClassName, ExtensionSystem::invoke
*/

BinEditorWidgetFactory::BinEditorWidgetFactory(QObject *parent) :
    QObject(parent)
{
}

QWidget *BinEditorWidgetFactory::createWidget(QWidget *parent)
{
    return new BinEditorWidget(parent);
}

///////////////////////////////// BinEditorPlugin //////////////////////////////////

BinEditorPlugin::BinEditorPlugin()
{
    m_undoAction = m_redoAction = m_copyAction = m_selectAllAction = 0;
}

BinEditorPlugin::~BinEditorPlugin()
{
}

QAction *BinEditorPlugin::registerNewAction(Core::Id id, const QString &title)
{
    QAction *result = new QAction(title, this);
    Core::ActionManager::registerAction(result, id, m_context);
    return result;
}

QAction *BinEditorPlugin::registerNewAction(Core::Id id,
                                            QObject *receiver,
                                            const char *slot,
                                            const QString &title)
{
    QAction *rc = registerNewAction(id, title);
    if (!rc)
        return 0;

    connect(rc, SIGNAL(triggered()), receiver, slot);
    return rc;
}

void BinEditorPlugin::initializeEditor(BinEditorWidget *widget)
{
    m_context.add(Constants::C_BINEDITOR);
    if (!m_undoAction) {
        m_undoAction      = registerNewAction(Core::Constants::UNDO, this, SLOT(undoAction()), tr("&Undo"));
        m_redoAction      = registerNewAction(Core::Constants::REDO, this, SLOT(redoAction()), tr("&Redo"));
        m_copyAction      = registerNewAction(Core::Constants::COPY, this, SLOT(copyAction()));
        m_selectAllAction = registerNewAction(Core::Constants::SELECTALL, this, SLOT(selectAllAction()));
    }

    QObject::connect(widget, SIGNAL(undoAvailable(bool)), this, SLOT(updateActions()));
    QObject::connect(widget, SIGNAL(redoAvailable(bool)), this, SLOT(updateActions()));

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    BinEditorFind *binEditorFind = new BinEditorFind(widget);
    aggregate->add(binEditorFind);
    aggregate->add(widget);
}

bool BinEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
        this, SLOT(updateCurrentEditor(Core::IEditor*)));

    addAutoReleasedObject(new BinEditorFactory(this));
    addAutoReleasedObject(new BinEditorWidgetFactory);
    return true;
}

void BinEditorPlugin::extensionsInitialized()
{
}

void BinEditorPlugin::updateCurrentEditor(Core::IEditor *editor)
{
    BinEditorWidget *binEditor = 0;
    if (editor)
        binEditor = qobject_cast<BinEditorWidget *>(editor->widget());
    if (m_currentEditor == binEditor)
        return;
    m_currentEditor = binEditor;
    updateActions();
}

void BinEditorPlugin::updateActions()
{
    bool hasEditor = (m_currentEditor != 0);
    if (m_selectAllAction)
        m_selectAllAction->setEnabled(hasEditor);
    if (m_undoAction)
        m_undoAction->setEnabled(m_currentEditor && m_currentEditor->isUndoAvailable());
    if (m_redoAction)
        m_redoAction->setEnabled(m_currentEditor && m_currentEditor->isRedoAvailable());
}

void BinEditorPlugin::undoAction()
{
    if (m_currentEditor)
        m_currentEditor->undo();
}

void BinEditorPlugin::redoAction()
{
    if (m_currentEditor)
        m_currentEditor->redo();
}

void BinEditorPlugin::copyAction()
{
    if (m_currentEditor)
        m_currentEditor->copy();
}

void BinEditorPlugin::selectAllAction()
{
    if (m_currentEditor)
        m_currentEditor->selectAll();
}


Q_EXPORT_PLUGIN(BinEditorPlugin)

#include "bineditorplugin.moc"
