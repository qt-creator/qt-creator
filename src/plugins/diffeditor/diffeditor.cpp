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

#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditorguicontroller.h"
#include "sidebysidediffeditorwidget.h"
#include "unifieddiffeditorwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/minisplitter.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/displaysettings.h>

#include <QStackedWidget>
#include <QToolButton>
#include <QSpinBox>
#include <QStyle>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolBar>
#include <QComboBox>
#include <QFileInfo>
#include <QDir>
#include <QTextCodec>
#include <QTextBlock>

static const char settingsGroupC[] = "DiffEditor";
static const char diffEditorTypeKeyC[] = "DiffEditorType";
static const char sideBySideDiffEditorValueC[] = "SideBySide";
static const char unifiedDiffEditorValueC[] = "Unified";

static const char legacySettingsGroupC[] = "Git";
static const char useDiffEditorKeyC[] = "UseDiffEditor";

using namespace TextEditor;

namespace DiffEditor {

namespace Internal {

class DescriptionEditorWidget : public BaseTextEditorWidget
{
    Q_OBJECT
public:
    DescriptionEditorWidget(QWidget *parent = 0);
    virtual QSize sizeHint() const;

signals:
    void expandBranchesRequested();

public slots:
    void setDisplaySettings(const DisplaySettings &ds);

protected:
    BaseTextEditor *createEditor()
    {
        BaseTextEditor *editor = new BaseTextEditor(this);
        editor->document()->setId("DiffEditor.DescriptionEditor");
        return editor;
    }
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

    bool findContentsUnderCursor(const QTextCursor &cursor);
    void highlightCurrentContents();
    void handleCurrentContents();

private:
    QTextCursor m_currentCursor;
};

DescriptionEditorWidget::DescriptionEditorWidget(QWidget *parent)
    : BaseTextEditorWidget(new BaseTextDocument, parent)
{
    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = false;
    settings.m_highlightCurrentLine = false;
    settings.m_displayFoldingMarkers = false;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    BaseTextEditorWidget::setDisplaySettings(settings);

    setCodeFoldingSupported(true);
    setFrameStyle(QFrame::NoFrame);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

QSize DescriptionEditorWidget::sizeHint() const
{
    QSize size = BaseTextEditorWidget::sizeHint();
    size.setHeight(size.height() / 5);
    return size;
}

void DescriptionEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    DisplaySettings settings = displaySettings();
    settings.m_visualizeWhitespace = ds.m_visualizeWhitespace;
    BaseTextEditorWidget::setDisplaySettings(settings);
}

void DescriptionEditorWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons()) {
        TextEditor::BaseTextEditorWidget::mouseMoveEvent(e);
        return;
    }

    Qt::CursorShape cursorShape;

    const QTextCursor cursor = cursorForPosition(e->pos());
    if (findContentsUnderCursor(cursor)) {
        highlightCurrentContents();
        cursorShape = Qt::PointingHandCursor;
    } else {
        setExtraSelections(OtherSelection, QList<QTextEdit::ExtraSelection>());
        cursorShape = Qt::IBeamCursor;
    }

    TextEditor::BaseTextEditorWidget::mouseMoveEvent(e);
    viewport()->setCursor(cursorShape);
}

void DescriptionEditorWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier)) {
        const QTextCursor cursor = cursorForPosition(e->pos());
        if (findContentsUnderCursor(cursor)) {
            handleCurrentContents();
            e->accept();
            return;
        }
    }

    TextEditor::BaseTextEditorWidget::mouseReleaseEvent(e);
}

bool DescriptionEditorWidget::findContentsUnderCursor(const QTextCursor &cursor)
{
    m_currentCursor = cursor;
    return cursor.block().text() == QLatin1String(Constants::EXPAND_BRANCHES);
}

void DescriptionEditorWidget::highlightCurrentContents()
{
    QTextEdit::ExtraSelection sel;
    sel.cursor = m_currentCursor;
    sel.cursor.select(QTextCursor::LineUnderCursor);
    sel.format.setFontUnderline(true);
    setExtraSelections(BaseTextEditorWidget::OtherSelection,
                       QList<QTextEdit::ExtraSelection>() << sel);

}

void DescriptionEditorWidget::handleCurrentContents()
{
    m_currentCursor.select(QTextCursor::LineUnderCursor);
    m_currentCursor.removeSelectedText();
    m_currentCursor.insertText(QLatin1String("Branches: Expanding..."));
    emit expandBranchesRequested();
}

} // namespace Internal

///////////////////////////////// DiffEditor //////////////////////////////////

DiffEditor::DiffEditor()
    : IEditor(0)
    , m_document(new DiffEditorDocument())
    , m_descriptionWidget(0)
    , m_stackedWidget(0)
    , m_sideBySideEditor(0)
    , m_unifiedEditor(0)
    , m_currentEditor(0)
    , m_controller(0)
    , m_guiController(0)
    , m_toolBar(0)
    , m_entriesComboBox(0)
    , m_toggleDescriptionAction(0)
    , m_reloadAction(0)
    , m_diffEditorSwitcher(0)
{
    ctor();
}

DiffEditor::DiffEditor(DiffEditor *other)
    : IEditor(0)
    , m_document(other->m_document)
    , m_descriptionWidget(0)
    , m_stackedWidget(0)
    , m_sideBySideEditor(0)
    , m_unifiedEditor(0)
    , m_currentEditor(0)
    , m_controller(0)
    , m_guiController(0)
    , m_toolBar(0)
    , m_entriesComboBox(0)
    , m_toggleDescriptionAction(0)
    , m_reloadAction(0)
    , m_diffEditorSwitcher(0)
{
    ctor();
}

void DiffEditor::ctor()
{
    setDuplicateSupported(true);

    QSplitter *splitter = new Core::MiniSplitter(Qt::Vertical);

    m_descriptionWidget = new Internal::DescriptionEditorWidget(splitter);
    m_descriptionWidget->setReadOnly(true);
    splitter->addWidget(m_descriptionWidget);

    m_stackedWidget = new QStackedWidget(splitter);
    splitter->addWidget(m_stackedWidget);

    m_sideBySideEditor = new SideBySideDiffEditorWidget(m_stackedWidget);
    m_stackedWidget->addWidget(m_sideBySideEditor);

    m_unifiedEditor = new UnifiedDiffEditorWidget(m_stackedWidget);
    m_stackedWidget->addWidget(m_unifiedEditor);

    setWidget(splitter);

    connect(m_descriptionWidget, SIGNAL(expandBranchesRequested()),
            m_document->controller(), SLOT(expandBranchesRequested()));
    connect(TextEditorSettings::instance(),
            SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            m_descriptionWidget,
            SLOT(setDisplaySettings(TextEditor::DisplaySettings)));
    connect(TextEditorSettings::instance(),
            SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            m_descriptionWidget->textDocument(),
            SLOT(setFontSettings(TextEditor::FontSettings)));

    m_descriptionWidget->setDisplaySettings(
                TextEditorSettings::displaySettings());
    m_descriptionWidget->setCodeStyle(
                TextEditorSettings::codeStyle());
    m_descriptionWidget->textDocument()->setFontSettings(
                TextEditorSettings::fontSettings());

    m_controller = m_document->controller();
    m_guiController = new DiffEditorGuiController(m_controller, this);

    connect(m_controller, SIGNAL(cleared(QString)),
            this, SLOT(slotCleared(QString)));
    connect(m_controller, SIGNAL(diffFilesChanged(QList<FileData>,QString)),
            this, SLOT(slotDiffFilesChanged(QList<FileData>,QString)));
    connect(m_controller, SIGNAL(descriptionChanged(QString)),
            this, SLOT(slotDescriptionChanged(QString)));
    connect(m_controller, SIGNAL(descriptionEnablementChanged(bool)),
            this, SLOT(slotDescriptionVisibilityChanged()));
    connect(m_guiController, SIGNAL(descriptionVisibilityChanged(bool)),
            this, SLOT(slotDescriptionVisibilityChanged()));
    connect(m_guiController, SIGNAL(currentDiffFileIndexChanged(int)),
            this, SLOT(activateEntry(int)));

    slotDescriptionChanged(m_controller->description());
    slotDescriptionVisibilityChanged();

    showDiffEditor(readCurrentDiffEditorSetting());

    toolBar();
}

DiffEditor::~DiffEditor()
{
    delete m_toolBar;
    if (m_widget)
        delete m_widget;
}

Core::IEditor *DiffEditor::duplicate()
{
    return new DiffEditor(this);
}

bool DiffEditor::open(QString *errorString,
                      const QString &fileName,
                      const QString &realFileName)
{
    Q_UNUSED(realFileName)

    if (!m_controller)
        return false;

    QString patch;
    if (m_document->read(fileName, &patch, errorString) != Utils::TextFileFormat::ReadSuccess)
        return false;

    bool ok = false;
    QList<FileData> fileDataList
            = DiffUtils::readPatch(patch,
                                   m_controller->isIgnoreWhitespace(),
                                   &ok);
    if (!ok) {
        *errorString = tr("Could not parse patch file \"%1\". "
                          "The contents is not of unified diff format.")
                .arg(fileName);
        return false;
    }

    const QFileInfo fi(fileName);
    m_document->setFilePath(QDir::cleanPath(fi.absoluteFilePath()));
    m_controller->setDiffFiles(fileDataList, fi.absolutePath());
    return true;
}

Core::IDocument *DiffEditor::document()
{
    return m_document.data();
}

static QToolBar *createToolBar(const QWidget *someWidget)
{
    // Create
    QToolBar *toolBar = new QToolBar;
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    const int size = someWidget->style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolBar->setIconSize(QSize(size, size));

    return toolBar;
}

QWidget *DiffEditor::toolBar()
{
    if (m_toolBar)
        return m_toolBar;

    // Create
    m_toolBar = createToolBar(m_sideBySideEditor);

    m_entriesComboBox = new QComboBox;
    m_entriesComboBox->setMinimumContentsLength(20);
    // Make the combo box prefer to expand
    QSizePolicy policy = m_entriesComboBox->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_entriesComboBox->setSizePolicy(policy);
    connect(m_entriesComboBox, SIGNAL(activated(int)),
            this, SLOT(entryActivated(int)));
    m_toolBar->addWidget(m_entriesComboBox);

    QToolButton *whitespaceButton = new QToolButton(m_toolBar);
    whitespaceButton->setText(tr("Ignore Whitespace"));
    whitespaceButton->setCheckable(true);
    whitespaceButton->setChecked(m_controller->isIgnoreWhitespace());
    m_toolBar->addWidget(whitespaceButton);

    QLabel *contextLabel = new QLabel(m_toolBar);
    contextLabel->setText(tr("Context Lines:"));
    contextLabel->setContentsMargins(6, 0, 6, 0);
    m_toolBar->addWidget(contextLabel);

    QSpinBox *contextSpinBox = new QSpinBox(m_toolBar);
    contextSpinBox->setRange(1, 100);
    contextSpinBox->setValue(m_controller->contextLinesNumber());
    contextSpinBox->setFrame(false);
    contextSpinBox->setSizePolicy(QSizePolicy::Minimum,
                                  QSizePolicy::Expanding); // Mac Qt5
    m_toolBar->addWidget(contextSpinBox);

    QToolButton *toggleDescription = new QToolButton(m_toolBar);
    toggleDescription->setIcon(
                QIcon(QLatin1String(Constants::ICON_TOP_BAR)));
    toggleDescription->setCheckable(true);
    toggleDescription->setChecked(m_guiController->isDescriptionVisible());
    m_toggleDescriptionAction = m_toolBar->addWidget(toggleDescription);
    slotDescriptionVisibilityChanged();

    QToolButton *reloadButton = new QToolButton(m_toolBar);
    reloadButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_RELOAD_GRAY)));
    reloadButton->setToolTip(tr("Reload Editor"));
    m_reloadAction = m_toolBar->addWidget(reloadButton);
    slotReloaderChanged(m_controller->reloader());

    QToolButton *toggleSync = new QToolButton(m_toolBar);
    toggleSync->setIcon(QIcon(QLatin1String(Core::Constants::ICON_LINK)));
    toggleSync->setCheckable(true);
    toggleSync->setChecked(m_guiController->horizontalScrollBarSynchronization());
    toggleSync->setToolTip(tr("Synchronize Horizontal Scroll Bars"));
    m_toolBar->addWidget(toggleSync);

    m_diffEditorSwitcher = new QToolButton(m_toolBar);
    m_toolBar->addWidget(m_diffEditorSwitcher);
    updateDiffEditorSwitcher();

    connect(whitespaceButton, SIGNAL(clicked(bool)),
            m_controller, SLOT(setIgnoreWhitespace(bool)));
    connect(m_controller, SIGNAL(ignoreWhitespaceChanged(bool)),
            whitespaceButton, SLOT(setChecked(bool)));
    connect(contextSpinBox, SIGNAL(valueChanged(int)),
            m_controller, SLOT(setContextLinesNumber(int)));
    connect(m_controller, SIGNAL(contextLinesNumberChanged(int)),
            contextSpinBox, SLOT(setValue(int)));
    connect(toggleSync, SIGNAL(clicked(bool)),
            m_guiController, SLOT(setHorizontalScrollBarSynchronization(bool)));
    connect(toggleDescription, SIGNAL(clicked(bool)),
            m_guiController, SLOT(setDescriptionVisible(bool)));
    connect(m_diffEditorSwitcher, SIGNAL(clicked()),
            this, SLOT(slotDiffEditorSwitched()));
    connect(reloadButton, SIGNAL(clicked()),
            m_controller, SLOT(requestReload()));
    connect(m_controller, SIGNAL(reloaderChanged(DiffEditorReloader*)),
            this, SLOT(slotReloaderChanged(DiffEditorReloader*)));

    return m_toolBar;
}

DiffEditorController *DiffEditor::controller() const
{
    return m_controller;
}

void DiffEditor::updateEntryToolTip()
{
    const QString &toolTip = m_entriesComboBox->itemData(
                m_entriesComboBox->currentIndex(), Qt::ToolTipRole).toString();
    m_entriesComboBox->setToolTip(toolTip);
}

void DiffEditor::entryActivated(int index)
{
    updateEntryToolTip();
    m_guiController->setCurrentDiffFileIndex(index);
}

void DiffEditor::slotCleared(const QString &message)
{
    Q_UNUSED(message)

    m_entriesComboBox->clear();
    updateEntryToolTip();
}

void DiffEditor::slotDiffFilesChanged(const QList<FileData> &diffFileList,
                                      const QString &workingDirectory)
{
    Q_UNUSED(workingDirectory)

    m_entriesComboBox->clear();
    const int count = diffFileList.count();
    for (int i = 0; i < count; i++) {
        const DiffFileInfo leftEntry = diffFileList.at(i).leftFileInfo;
        const DiffFileInfo rightEntry = diffFileList.at(i).rightFileInfo;
        const QString leftShortFileName = QFileInfo(leftEntry.fileName).fileName();
        const QString rightShortFileName = QFileInfo(rightEntry.fileName).fileName();
        QString itemText;
        QString itemToolTip;
        if (leftEntry.fileName == rightEntry.fileName) {
            itemText = leftShortFileName;

            if (leftEntry.typeInfo.isEmpty() && rightEntry.typeInfo.isEmpty()) {
                itemToolTip = leftEntry.fileName;
            } else {
                itemToolTip = tr("[%1] vs. [%2] %3")
                        .arg(leftEntry.typeInfo,
                             rightEntry.typeInfo,
                             leftEntry.fileName);
            }
        } else {
            if (leftShortFileName == rightShortFileName) {
                itemText = leftShortFileName;
            } else {
                itemText = tr("%1 vs. %2")
                        .arg(leftShortFileName,
                             rightShortFileName);
            }

            if (leftEntry.typeInfo.isEmpty() && rightEntry.typeInfo.isEmpty()) {
                itemToolTip = tr("%1 vs. %2")
                        .arg(leftEntry.fileName,
                             rightEntry.fileName);
            } else {
                itemToolTip = tr("[%1] %2 vs. [%3] %4")
                        .arg(leftEntry.typeInfo,
                             leftEntry.fileName,
                             rightEntry.typeInfo,
                             rightEntry.fileName);
            }
        }
        m_entriesComboBox->addItem(itemText);
        m_entriesComboBox->setItemData(m_entriesComboBox->count() - 1,
                                       itemToolTip, Qt::ToolTipRole);
    }
    updateEntryToolTip();
}

void DiffEditor::activateEntry(int index)
{
    m_entriesComboBox->blockSignals(true);
    m_entriesComboBox->setCurrentIndex(index);
    m_entriesComboBox->blockSignals(false);
    updateEntryToolTip();
}

void DiffEditor::slotDescriptionChanged(const QString &description)
{
    m_descriptionWidget->setPlainText(description);
}

void DiffEditor::slotDescriptionVisibilityChanged()
{
    const bool enabled = m_controller->isDescriptionEnabled();
    const bool visible = m_guiController->isDescriptionVisible();

    m_descriptionWidget->setVisible(visible && enabled);

    if (!m_toggleDescriptionAction)
        return;

    QWidget *toggle = m_toolBar->widgetForAction(m_toggleDescriptionAction);
    if (visible)
        toggle->setToolTip(tr("Hide Change Description"));
    else
        toggle->setToolTip(tr("Show Change Description"));

    m_toggleDescriptionAction->setVisible(enabled);
}

void DiffEditor::slotReloaderChanged(DiffEditorReloader *reloader)
{
    m_reloadAction->setVisible(reloader);
}

void DiffEditor::slotDiffEditorSwitched()
{
    QWidget *oldEditor = m_currentEditor;
    QWidget *newEditor = 0;
    if (oldEditor == m_sideBySideEditor)
        newEditor = m_unifiedEditor;
    else if (oldEditor == m_unifiedEditor)
        newEditor = m_sideBySideEditor;
    else
        newEditor = readCurrentDiffEditorSetting();

    showDiffEditor(newEditor);
}

void DiffEditor::updateDiffEditorSwitcher()
{
    if (!m_diffEditorSwitcher)
        return;

    QIcon actionIcon;
    QString actionToolTip;
    if (m_currentEditor == m_unifiedEditor) {
        actionIcon = QIcon(QLatin1String(Constants::ICON_SIDE_BY_SIDE_DIFF));
        actionToolTip = tr("Switch to Side By Side Diff Editor");
    } else if (m_currentEditor == m_sideBySideEditor) {
        actionIcon = QIcon(QLatin1String(Constants::ICON_UNIFIED_DIFF));
        actionToolTip = tr("Switch to Unified Diff Editor");
    }

    m_diffEditorSwitcher->setIcon(actionIcon);
    m_diffEditorSwitcher->setToolTip(actionToolTip);
}

void DiffEditor::showDiffEditor(QWidget *newEditor)
{
    if (m_currentEditor == newEditor)
        return;

    if (m_currentEditor == m_sideBySideEditor)
        m_sideBySideEditor->setDiffEditorGuiController(0);
    else if (m_currentEditor == m_unifiedEditor)
        m_unifiedEditor->setDiffEditorGuiController(0);

    m_currentEditor = newEditor;

    if (m_currentEditor == m_unifiedEditor)
        m_unifiedEditor->setDiffEditorGuiController(m_guiController);
    else if (m_currentEditor == m_sideBySideEditor)
        m_sideBySideEditor->setDiffEditorGuiController(m_guiController);

    m_stackedWidget->setCurrentWidget(m_currentEditor);

    writeCurrentDiffEditorSetting(m_currentEditor);
    updateDiffEditorSwitcher();
}

QWidget *DiffEditor::readLegacyCurrentDiffEditorSetting()
{
    QSettings *s = Core::ICore::settings();

    s->beginGroup(QLatin1String(legacySettingsGroupC));
    const bool legacyExists = s->contains(QLatin1String(useDiffEditorKeyC));
    const bool legacyEditor = s->value(
                QLatin1String(useDiffEditorKeyC), true).toBool();
    if (legacyExists)
        s->remove(QLatin1String(useDiffEditorKeyC));
    s->endGroup();

    QWidget *currentEditor = m_sideBySideEditor;
    if (!legacyEditor)
        currentEditor = m_unifiedEditor;

    if (legacyExists && currentEditor == m_unifiedEditor)
        writeCurrentDiffEditorSetting(currentEditor);

    return currentEditor;
}

QWidget *DiffEditor::readCurrentDiffEditorSetting()
{
    // replace it with m_sideBySideEditor when dropping legacy stuff
    QWidget *defaultEditor = readLegacyCurrentDiffEditorSetting();

    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    const QString editorString = s->value(
                QLatin1String(diffEditorTypeKeyC)).toString();
    s->endGroup();
    if (editorString == QLatin1String(unifiedDiffEditorValueC))
        return m_unifiedEditor;

    if (editorString == QLatin1String(sideBySideDiffEditorValueC))
        return m_sideBySideEditor;

    return defaultEditor;
}

void DiffEditor::writeCurrentDiffEditorSetting(QWidget *currentEditor)
{
    const QString editorString = currentEditor == m_unifiedEditor
            ? QLatin1String(unifiedDiffEditorValueC)
            : QLatin1String(sideBySideDiffEditorValueC);
    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(diffEditorTypeKeyC), editorString);
    s->endGroup();
}

} // namespace DiffEditor

#include "diffeditor.moc"
