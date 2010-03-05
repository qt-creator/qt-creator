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

#include "bineditorplugin.h"
#include "bineditor.h"
#include "bineditorconstants.h"

#include "imageviewer.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QMainWindow>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolBar>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <find/ifindsupport.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>
#include <utils/linecolumnlabel.h>
#include <utils/reloadpromptutils.h>

using namespace BINEditor;
using namespace BINEditor::Internal;


class BinEditorFind : public Find::IFindSupport
{
    Q_OBJECT
public:
    BinEditorFind(BinEditor *editor)
    {
        m_editor = editor;
        m_incrementalStartPos = m_contPos = -1;
    }
    ~BinEditorFind() {}

    bool supportsReplace() const { return false; }
    IFindSupport::FindFlags supportedFindFlags() const
    {
        return IFindSupport::FindBackward | IFindSupport::FindCaseSensitively;
    }

    void resetIncrementalSearch()
    {
        m_incrementalStartPos = m_contPos = -1;
    }

    void clearResults() { m_editor->highlightSearchResults(QByteArray()); }
    QString currentFindString() const { return QString(); }
    QString completedFindString() const { return QString(); }


    int find(const QByteArray &pattern, int pos, Find::IFindSupport::FindFlags findFlags) {
        if (pattern.isEmpty()) {
            m_editor->setCursorPosition(pos);
            return pos;
        }

        return m_editor->find(pattern, pos, Find::IFindSupport::textDocumentFlagsForFindFlags(findFlags));
    }

    Result findIncremental(const QString &txt, Find::IFindSupport::FindFlags findFlags) {
        QByteArray pattern = txt.toLatin1();
        if (pattern != m_lastPattern)
            resetIncrementalSearch(); // Because we don't search for nibbles.
        m_lastPattern = pattern;
        if (m_incrementalStartPos < 0)
            m_incrementalStartPos = m_editor->selectionStart();
        if (m_contPos == -1)
            m_contPos = m_incrementalStartPos;
        int found = find(pattern, m_contPos, findFlags);
        Result result;
        if (found >= 0) {
            result = Found;
            m_editor->highlightSearchResults(pattern, Find::IFindSupport::textDocumentFlagsForFindFlags(findFlags));
            m_contPos = -1;
        } else {
            if (found == -2) {
                result = NotYetFound;
                m_contPos +=
                        findFlags & Find::IFindSupport::FindBackward
                        ? -BinEditor::SearchStride : BinEditor::SearchStride;
            } else {
                result = NotFound;
                m_contPos = -1;
                m_editor->highlightSearchResults(QByteArray(), 0);
            }
        }
        return result;
    }

    Result findStep(const QString &txt, Find::IFindSupport::FindFlags findFlags) {
        QByteArray pattern = txt.toLatin1();
        bool wasReset = (m_incrementalStartPos < 0);
        if (m_contPos == -1) {
            m_contPos = m_editor->cursorPosition();
            if (findFlags & Find::IFindSupport::FindBackward)
                m_contPos = m_editor->selectionStart()-1;
        }
        int found = find(pattern, m_contPos, findFlags);
        Result result;
        if (found >= 0) {
            result = Found;
            m_incrementalStartPos = found;
            m_contPos = -1;
            if (wasReset)
                m_editor->highlightSearchResults(pattern, Find::IFindSupport::textDocumentFlagsForFindFlags(findFlags));
        } else if (found == -2) {
            result = NotYetFound;
            m_contPos += findFlags & Find::IFindSupport::FindBackward
                         ? -BinEditor::SearchStride : BinEditor::SearchStride;
        } else {
            result = NotFound;
            m_contPos = -1;
        }

        return result;
    }

    bool replaceStep(const QString &, const QString &,
                     Find::IFindSupport::FindFlags) { return false;}
    int replaceAll(const QString &, const QString &,
                   Find::IFindSupport::FindFlags) { return 0; }

private:
    BinEditor *m_editor;
    int m_incrementalStartPos;
    int m_contPos; // Only valid if last result was NotYetFound.
    QByteArray m_lastPattern;
};


class BinEditorFile : public Core::IFile
{
    Q_OBJECT
public:
    BinEditorFile(BinEditor *parent) :
        Core::IFile(parent),
        m_mimeType(QLatin1String(BINEditor::Constants::C_BINEDITOR_MIMETYPE))
    {
        m_editor = parent;
        connect(m_editor, SIGNAL(lazyDataRequested(Core::IEditor *, quint64, bool)),
            this, SLOT(provideData(Core::IEditor *, quint64)));
    }
    ~BinEditorFile() {}

    virtual QString mimeType() const { return m_mimeType; }

    bool save(const QString &fileName = QString()) {
        if (m_editor->save(m_fileName, fileName)) {
            m_fileName = fileName;
            m_editor->editorInterface()->
                setDisplayName(QFileInfo(fileName).fileName());
            emit changed();
            return true;
        } else {
            return false;
        }
    }

    bool open(const QString &fileName) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            m_fileName = fileName;
            if (file.isSequential() && file.size() <= 64 * 1024 * 1024) {
                m_editor->setData(file.readAll());
            } else {
                m_editor->setLazyData(0, qMin(file.size(),
                                              static_cast<qint64>(INT_MAX-16)));
                m_editor->editorInterface()->
                        setDisplayName(QFileInfo(fileName).fileName());
            }
            file.close();
            return true;
        }
        return false;
    }

private slots:
    void provideData(Core::IEditor *, quint64 block) {
        QFile file(m_fileName);
        if (file.open(QIODevice::ReadOnly)) {
            int blockSize = m_editor->lazyDataBlockSize();
            file.seek(block * blockSize);
            QByteArray data = file.read(blockSize);
            if (data.size() != blockSize)
                data.resize(blockSize);
            m_editor->addLazyData(block, data);
            file.close();
        }
    }
public:

    void setFilename(const QString &filename) {
        m_fileName = filename;
    }

    QString fileName() const {
        return m_fileName;
    }

    QString defaultPath() const { return QString(); }
    QString suggestedFileName() const { return QString(); }
    QString fileFilter() const { return QString(); }
    QString fileExtension() const { return QString(); }

    bool isModified() const {
        return m_editor->isModified();
    }
    bool isReadOnly() const {
        const QFileInfo fi(m_fileName);
        return !fi.isWritable();
    }

    bool isSaveAsAllowed() const { return true; }

    void modified(ReloadBehavior *behavior) {
        const QString fileName = m_fileName;

        switch (*behavior) {
        case  Core::IFile::ReloadNone:
            return;
        case Core::IFile::ReloadUnmodified:
            if (!isModified()) {
                open(fileName);
                return;
            }
            break;
        case Core::IFile::ReloadAll:
            open(fileName);
            return;
        case Core::IFile::ReloadPermissions:
            emit changed();
            return;
        case Core::IFile::AskForReload:
            break;
        }

        switch (Utils::reloadPrompt(fileName, isModified(), Core::ICore::instance()->mainWindow())) {
        case Utils::ReloadCurrent:
            open(fileName);
            break;
        case Utils::ReloadAll:
            open(fileName);
            *behavior = Core::IFile::ReloadAll;
            break;
        case Utils::ReloadSkipCurrent:
            break;
        case Utils::ReloadNone:
            *behavior = Core::IFile::ReloadNone;
            break;
        }
    }

private:
    const QString m_mimeType;
    BinEditor *m_editor;
    QString m_fileName;
};

class BinEditorInterface : public Core::IEditor
{
    Q_OBJECT
public:
    BinEditorInterface(BinEditor *editor)
    {
        Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
        m_editor = editor;
        m_file = new BinEditorFile(m_editor);
        m_context << uidm->uniqueIdentifier(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
        m_context << uidm->uniqueIdentifier(Constants::C_BINEDITOR);
        m_cursorPositionLabel = new Utils::LineColumnLabel;

        QHBoxLayout *l = new QHBoxLayout;
        QWidget *w = new QWidget;
        l->setMargin(0);
        l->setContentsMargins(0, 0, 5, 0);
        l->addStretch(1);
        l->addWidget(m_cursorPositionLabel);
        w->setLayout(l);

        m_toolBar = new QToolBar;
        m_toolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        m_toolBar->addWidget(w);

        connect(m_editor, SIGNAL(cursorPositionChanged(int)), this, SLOT(updateCursorPosition(int)));
    }
    ~BinEditorInterface() {
        delete m_editor;
    }

    QWidget *widget() { return m_editor; }

    QList<int> context() const { return m_context; }

    bool createNew(const QString & /* contents */ = QString()) {
        m_editor->setData(QByteArray());
        m_file->setFilename(QString());
        return true;
    }
    bool open(const QString &fileName = QString()) {
        return m_file->open(fileName);
    }
    Core::IFile *file() { return m_file; }
    QString id() const { return QLatin1String(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID); }
    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString &title) { m_displayName = title; emit changed(); }

    bool duplicateSupported() const { return false; }
    IEditor *duplicate(QWidget * /* parent */) { return 0; }

    QByteArray saveState() const { return QByteArray(); } // TODO
    bool restoreState(const QByteArray & /* state */) { return false; } // TODO

    QWidget *toolBar() { return m_toolBar; }

    bool isTemporary() const { return false; }

public slots:
    void updateCursorPosition(int position) {
        m_cursorPositionLabel->setText(m_editor->addressString((uint)position),
                                       m_editor->addressString((uint)m_editor->data().size()));
    }

private:
    BinEditor *m_editor;
    QString m_displayName;
    BinEditorFile *m_file;
    QList<int> m_context;
    QToolBar *m_toolBar;
    Utils::LineColumnLabel *m_cursorPositionLabel;
};




///////////////////////////////// BinEditorFactory //////////////////////////////////

BinEditorFactory::BinEditorFactory(BinEditorPlugin *owner) :
    m_mimeTypes(QLatin1String(BINEditor::Constants::C_BINEDITOR_MIMETYPE)),
    m_owner(owner)
{
}

QString BinEditorFactory::id() const
{
    return QLatin1String(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
}

QString BinEditorFactory::displayName() const
{
    return tr(Constants::C_BINEDITOR_DISPLAY_NAME);
}

Core::IFile *BinEditorFactory::open(const QString &fileName)
{
    Core::EditorManager *em = Core::EditorManager::instance();
    Core::IEditor *iface = em->openEditor(fileName, id());
    return iface ? iface->file() : 0;
}

Core::IEditor *BinEditorFactory::createEditor(QWidget *parent)
{
    BinEditor *editor = new BinEditor(parent);
    m_owner->initializeEditor(editor);
    return editor->editorInterface();
}

QStringList BinEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

///////////////////////////////// BinEditorPlugin //////////////////////////////////

BinEditorPlugin::BinEditorPlugin()
{
    m_undoAction = m_redoAction = m_copyAction = m_selectAllAction = 0;
}

BinEditorPlugin::~BinEditorPlugin()
{
}

QAction *BinEditorPlugin::registerNewAction(const QString &id, const QString &title)
{
    QAction *result = new QAction(title, this);
    Core::ICore::instance()->actionManager()->registerAction(result, id, m_context);
    return result;
}

QAction *BinEditorPlugin::registerNewAction(const QString &id,
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

void BinEditorPlugin::initializeEditor(BinEditor *editor)
{
    BinEditorInterface *editorInterface = new BinEditorInterface(editor);
    QObject::connect(editor, SIGNAL(modificationChanged(bool)), editorInterface, SIGNAL(changed()));
    editor->setEditorInterface(editorInterface);

    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(Constants::C_BINEDITOR);
    if (!m_undoAction) {
        m_undoAction      = registerNewAction(QLatin1String(Core::Constants::UNDO),
                                              this, SLOT(undoAction()),
                                              tr("&Undo"));
        m_redoAction      = registerNewAction(QLatin1String(Core::Constants::REDO),
                                              this, SLOT(redoAction()),
                                              tr("&Redo"));
        m_copyAction      = registerNewAction(QLatin1String(Core::Constants::COPY),
                                              this, SLOT(copyAction()));
        m_selectAllAction = registerNewAction(QLatin1String(Core::Constants::SELECTALL),
                                              this, SLOT(selectAllAction()));
    }

    // Font settings
    TextEditor::TextEditorSettings *settings = TextEditor::TextEditorSettings::instance();
    editor->setFontSettings(settings->fontSettings());
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            editor, SLOT(setFontSettings(TextEditor::FontSettings)));

    QObject::connect(editor, SIGNAL(undoAvailable(bool)), this, SLOT(updateActions()));
    QObject::connect(editor, SIGNAL(redoAvailable(bool)), this, SLOT(updateActions()));
    QObject::connect(editor, SIGNAL(copyAvailable(bool)), this, SLOT(updateActions()));

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    BinEditorFind *binEditorFind = new BinEditorFind(editor);
    aggregate->add(binEditorFind);
    aggregate->add(editor);
}

bool BinEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/bineditor/ImageViewer.mimetypes.xml"), errorMessage))
        return false;

    connect(core, SIGNAL(contextAboutToChange(Core::IContext *)),
        this, SLOT(updateCurrentEditor(Core::IContext *)));

    addAutoReleasedObject(new BinEditorFactory(this));
    addAutoReleasedObject(new ImageViewerFactory);
    return true;
}

void BinEditorPlugin::extensionsInitialized()
{
}

void BinEditorPlugin::updateCurrentEditor(Core::IContext *object)
{
    do {
        if (!object) {
            if (!m_currentEditor)
                return;

            m_currentEditor = 0;
            break;
        }
        BinEditor *editor = qobject_cast<BinEditor *>(object->widget());
        if (!editor) {
            if (!m_currentEditor)
                return;

            m_currentEditor = 0;
            break;
        }

        if (editor == m_currentEditor)
            return;

        m_currentEditor = editor;

    } while (false);
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
    if (m_copyAction)
        m_copyAction->setEnabled(m_currentEditor && m_currentEditor->hasSelection());
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
