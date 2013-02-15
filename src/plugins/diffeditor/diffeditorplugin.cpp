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

#include "diffeditorplugin.h"
#include "diffeditorwidget.h"
#include "diffeditorconstants.h"

#include <coreplugin/icore.h>
#include <QCoreApplication>
#include <QToolButton>
#include <QSpinBox>
#include <QStyle>
#include <QLabel>
#include <QFileDialog>
#include <QTextCodec>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>

using namespace DIFFEditor;
using namespace DIFFEditor::Internal;

///////////////////////////////// DiffEditor //////////////////////////////////

DiffEditor::DiffEditor(DiffEditorWidget *editorWidget)
    :
    IEditor(0),
    m_file(new DiffFile(QLatin1String(Constants::DIFF_EDITOR_MIMETYPE), this)),
    m_editorWidget(editorWidget),
    m_toolWidget(0)
{
    setWidget(editorWidget);
}

DiffEditor::~DiffEditor()
{
    delete m_toolWidget;
    if (m_widget)
        delete m_widget;
}

bool DiffEditor::createNew(const QString &contents)
{
    Q_UNUSED(contents)
//    setFileContents(contents);
    return true;
}

bool DiffEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName)
    Q_UNUSED(realFileName)
    const QString text = QLatin1String("Open");
    if (!createNew(text))
        return false;

    return true;
}

Core::IDocument *DiffEditor::document()
{
    return m_file;
}

QString DiffEditor::displayName() const
{
    if (m_displayName.isEmpty())
        m_displayName = QCoreApplication::translate("DiffEditor", Constants::DIFF_EDITOR_DISPLAY_NAME);
    return m_displayName;
}

void DiffEditor::setDisplayName(const QString &title)
{
    m_displayName = title;
    emit changed();
}

bool DiffEditor::duplicateSupported() const
{
    return false;
}

Core::IEditor *DiffEditor::duplicate(QWidget * /*parent*/)
{
    return 0;
}

Core::Id DiffEditor::id() const
{
    return Constants::DIFF_EDITOR_ID;
}

static QToolBar *createToolBar(const QWidget *someWidget)
{
    // Create
    QToolBar *toolBar = new QToolBar;
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    const int size = someWidget->style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolBar->setIconSize(QSize(size, size));
    toolBar->addSeparator();

    return toolBar;
}

QWidget *DiffEditor::toolBar()
{
    if (m_toolWidget)
        return m_toolWidget;

    // Create
    m_toolWidget = createToolBar(m_editorWidget);

    QToolButton *whitespaceButton = new QToolButton(m_toolWidget);
    whitespaceButton->setText(tr("Ignore Whitespaces"));
    whitespaceButton->setCheckable(true);
    whitespaceButton->setChecked(true);
    connect(whitespaceButton, SIGNAL(clicked(bool)),
            m_editorWidget, SLOT(setIgnoreWhitespaces(bool)));
    m_toolWidget->addWidget(whitespaceButton);

    QLabel *contextLabel = new QLabel(tr("Context Lines:"), m_toolWidget);
    m_toolWidget->addWidget(contextLabel);

    QSpinBox *contextSpinBox = new QSpinBox(m_toolWidget);
    contextSpinBox->setRange(-1, 100);
    contextSpinBox->setValue(1);
    connect(contextSpinBox, SIGNAL(valueChanged(int)),
            m_editorWidget, SLOT(setContextLinesNumber(int)));
    m_toolWidget->addWidget(contextSpinBox);

    return m_toolWidget;
}

QByteArray DiffEditor::saveState() const
{
    return QByteArray();
}

bool DiffEditor::restoreState(const QByteArray &/*state*/)
{
    return true;
}

///////////////////////////////// DiffFile //////////////////////////////////

DiffFile::DiffFile(const QString &mimeType, QObject *parent) :
    Core::IDocument(parent),
    m_mimeType(mimeType),
    m_modified(false)
{
}

void DiffFile::rename(const QString &newName)
{
    Q_UNUSED(newName);
    return;
}

void DiffFile::setFileName(const QString &name)
{
    if (m_fileName == name)
        return;
    m_fileName = name;
    emit changed();
}

void DiffFile::setModified(bool modified)
{
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit changed();
}

bool DiffFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    emit saveMe(errorString, fileName, autoSave);
    if (!errorString->isEmpty())
        return false;
    emit changed();
    return true;
}

QString DiffFile::mimeType() const
{
    return m_mimeType;
}

Core::IDocument::ReloadBehavior DiffFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool DiffFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}

///////////////////////////////// DiffEditorFactory //////////////////////////////////

DiffEditorFactory::DiffEditorFactory(DiffEditorPlugin *owner) :
    m_mimeTypes(QLatin1String(Constants::DIFF_EDITOR_MIMETYPE)),
    m_owner(owner)
{
}

Core::Id DiffEditorFactory::id() const
{
    return Constants::DIFF_EDITOR_ID;
}

QString DiffEditorFactory::displayName() const
{
    return qApp->translate("DiffEditorFactory", Constants::DIFF_EDITOR_DISPLAY_NAME);
}

Core::IEditor *DiffEditorFactory::createEditor(QWidget *parent)
{
    DiffEditorWidget *editorWidget = new DiffEditorWidget(parent);
    DiffEditor *editor = new DiffEditor(editorWidget);
    return editor;
}

QStringList DiffEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

///////////////////////////////// DiffEditorPlugin //////////////////////////////////

DiffEditorPlugin::DiffEditorPlugin()
{
}

DiffEditorPlugin::~DiffEditorPlugin()
{
}

void DiffEditorPlugin::initializeEditor(DiffEditorWidget *editor)
{
    Q_UNUSED(editor)
}

bool DiffEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    //register actions
    Core::ActionContainer *toolsContainer =
        Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsContainer->insertGroup(Core::Constants::G_TOOLS_OPTIONS, Constants::G_TOOLS_DIFF);

    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    QAction *diffAction = new QAction(tr("Diff..."), this);
    Core::Command *diffCommand = Core::ActionManager::registerAction(diffAction,
                             "DiffEditor.Diff", globalcontext);
    connect(diffAction, SIGNAL(triggered()), this, SLOT(diff()));
    toolsContainer->addAction(diffCommand, Constants::G_TOOLS_DIFF);

    addAutoReleasedObject(new DiffEditorFactory(this));

    return true;
}

void DiffEditorPlugin::extensionsInitialized()
{
}

void DiffEditorPlugin::diff()
{
    QString fileName1 = QFileDialog::getOpenFileName(0,
                                                     tr("Select the first file for diff"),
                                                     QString());
    if (fileName1.isNull())
        return;

    QString fileName2 = QFileDialog::getOpenFileName(0,
                                                     tr("Select the second file for diff"),
                                                     QString());
    if (fileName2.isNull())
        return;


    const Core::Id editorId = Constants::DIFF_EDITOR_ID;
    QString title = tr("Diff \"%1\", \"%2\"").arg(fileName1).arg(fileName2);
    Core::IEditor *outputEditor = Core::EditorManager::openEditorWithContents(editorId, &title, QString());
    Core::EditorManager::activateEditor(outputEditor, Core::EditorManager::ModeSwitch);

    DiffEditorWidget *editorWidget = getDiffEditorWidget(outputEditor);
    if (editorWidget) {
        const QString text1 = getFileContents(fileName1, editorWidget->codec());
        const QString text2 = getFileContents(fileName2, editorWidget->codec());

        editorWidget->setDiff(text1, text2);
    }
}

DiffEditorWidget *DiffEditorPlugin::getDiffEditorWidget(const Core::IEditor *editor) const
{
    if (const DiffEditor *de = qobject_cast<const DiffEditor *>(editor))
        return de->editorWidget();
    return 0;
}

QString DiffEditorPlugin::getFileContents(const QString &fileName, QTextCodec *codec) const
{
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return codec->toUnicode(file.readAll());
    }
    return QString();
}


Q_EXPORT_PLUGIN(DiffEditorPlugin)
