/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditorguicontroller.h"
#include "diffview.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/minisplitter.h>

#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/displaysettings.h>
#include <texteditor/marginsettings.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QStackedWidget>
#include <QToolButton>
#include <QSpinBox>
#include <QStyle>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolBar>
#include <QComboBox>
#include <QAction>
#include <QDir>
#include <QTextCodec>
#include <QTextBlock>

static const char settingsGroupC[] = "DiffEditor";
static const char diffEditorTypeKeyC[] = "DiffEditorType";

static const char legacySettingsGroupC[] = "Git";
static const char useDiffEditorKeyC[] = "UseDiffEditor";

using namespace TextEditor;

namespace DiffEditor {
namespace Internal {

class DescriptionEditorWidget : public TextEditorWidget
{
    Q_OBJECT
public:
    DescriptionEditorWidget(QWidget *parent = 0);
    virtual QSize sizeHint() const;

signals:
    void requestBranchList();

protected:
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

    void setDisplaySettings(const DisplaySettings &ds);
    void setMarginSettings(const MarginSettings &ms);

    bool findContentsUnderCursor(const QTextCursor &cursor);
    void highlightCurrentContents();
    void handleCurrentContents();

private:
    QTextCursor m_currentCursor;
};

DescriptionEditorWidget::DescriptionEditorWidget(QWidget *parent)
    : TextEditorWidget(parent)
{
    setupFallBackEditor("DiffEditor.DescriptionEditor");

    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = false;
    settings.m_highlightCurrentLine = false;
    settings.m_displayFoldingMarkers = false;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    TextEditorWidget::setDisplaySettings(settings);

    setCodeFoldingSupported(true);
    setFrameStyle(QFrame::NoFrame);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

QSize DescriptionEditorWidget::sizeHint() const
{
    QSize size = TextEditorWidget::sizeHint();
    size.setHeight(size.height() / 5);
    return size;
}

void DescriptionEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    DisplaySettings settings = displaySettings();
    settings.m_visualizeWhitespace = ds.m_visualizeWhitespace;
    TextEditorWidget::setDisplaySettings(settings);
}

void DescriptionEditorWidget::setMarginSettings(const MarginSettings &ms)
{
    Q_UNUSED(ms);
    TextEditorWidget::setMarginSettings(MarginSettings());
}

void DescriptionEditorWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons()) {
        TextEditorWidget::mouseMoveEvent(e);
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

    TextEditorWidget::mouseMoveEvent(e);
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

    TextEditorWidget::mouseReleaseEvent(e);
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
    setExtraSelections(TextEditorWidget::OtherSelection,
                       QList<QTextEdit::ExtraSelection>() << sel);

}

void DescriptionEditorWidget::handleCurrentContents()
{
    m_currentCursor.select(QTextCursor::LineUnderCursor);
    m_currentCursor.removeSelectedText();
    m_currentCursor.insertText(QLatin1String("Branches: Expanding..."));
    emit requestBranchList();
}

///////////////////////////////// DiffEditor //////////////////////////////////

DiffEditor::DiffEditor(const QSharedPointer<DiffEditorDocument> &doc)
    : m_document(doc)
    , m_descriptionWidget(0)
    , m_stackedWidget(0)
    , m_currentViewIndex(-1)
    , m_guiController(0)
    , m_toolBar(0)
    , m_entriesComboBox(0)
    , m_toggleDescriptionAction(0)
    , m_reloadAction(0)
    , m_diffEditorSwitcher(0)
{
    QTC_ASSERT(m_document, return);

    QSplitter *splitter = new Core::MiniSplitter(Qt::Vertical);

    m_descriptionWidget = new DescriptionEditorWidget(splitter);
    m_descriptionWidget->setReadOnly(true);
    splitter->addWidget(m_descriptionWidget);

    m_stackedWidget = new QStackedWidget(splitter);
    splitter->addWidget(m_stackedWidget);

    addView(new SideBySideView);
    addView(new UnifiedView);

    setWidget(splitter);

    DiffEditorController *control = controller();
    m_guiController = new DiffEditorGuiController(control, this);

    connect(m_descriptionWidget, &DescriptionEditorWidget::requestBranchList,
            control, &DiffEditorController::expandBranchesRequested);
    connect(control, &DiffEditorController::cleared, this, &DiffEditor::slotCleared);
    connect(control, &DiffEditorController::diffFilesChanged,
            this, &DiffEditor::slotDiffFilesChanged);
    connect(control, &DiffEditorController::descriptionChanged,
            this, &DiffEditor::slotDescriptionChanged);
    connect(control, &DiffEditorController::descriptionEnablementChanged,
            this, &DiffEditor::slotDescriptionVisibilityChanged);
    connect(m_guiController, &DiffEditorGuiController::descriptionVisibilityChanged,
            this, &DiffEditor::slotDescriptionVisibilityChanged);
    connect(m_guiController, &DiffEditorGuiController::currentDiffFileIndexChanged,
            this, &DiffEditor::activateEntry);

    slotDescriptionChanged(control->description());
    slotDescriptionVisibilityChanged();

    showDiffView(readCurrentDiffEditorSetting());

    toolBar();
}

DiffEditor::~DiffEditor()
{
    delete m_toolBar;
    delete m_widget;
}

Core::IEditor *DiffEditor::duplicate()
{
    return new DiffEditor(m_document);
}

bool DiffEditor::open(QString *errorString,
                      const QString &fileName,
                      const QString &realFileName)
{
    Q_UNUSED(realFileName)
    return m_document->open(errorString, fileName);
}

Core::IDocument *DiffEditor::document()
{
    return m_document.data();
}

static QToolBar *createToolBar(IDiffView *someView)
{
    // Create
    QToolBar *toolBar = new QToolBar;
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    const int size = someView->widget()->style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolBar->setIconSize(QSize(size, size));

    return toolBar;
}

QWidget *DiffEditor::toolBar()
{
    QTC_ASSERT(!m_views.isEmpty(), return 0);

    if (m_toolBar)
        return m_toolBar;

    DiffEditorController *control = controller();

    // Create
    m_toolBar = createToolBar(m_views.at(0));

    m_entriesComboBox = new QComboBox;
    m_entriesComboBox->setMinimumContentsLength(20);
    // Make the combo box prefer to expand
    QSizePolicy policy = m_entriesComboBox->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_entriesComboBox->setSizePolicy(policy);
    connect(m_entriesComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &DiffEditor::entryActivated);
    m_toolBar->addWidget(m_entriesComboBox);

    QToolButton *whitespaceButton = new QToolButton(m_toolBar);
    whitespaceButton->setText(tr("Ignore Whitespace"));
    whitespaceButton->setCheckable(true);
    whitespaceButton->setChecked(control->isIgnoreWhitespace());
    m_whitespaceButtonAction = m_toolBar->addWidget(whitespaceButton);

    QLabel *contextLabel = new QLabel(m_toolBar);
    contextLabel->setText(tr("Context Lines:"));
    contextLabel->setContentsMargins(6, 0, 6, 0);
    m_contextLabelAction = m_toolBar->addWidget(contextLabel);

    QSpinBox *contextSpinBox = new QSpinBox(m_toolBar);
    contextSpinBox->setRange(1, 100);
    contextSpinBox->setValue(control->contextLinesNumber());
    contextSpinBox->setFrame(false);
    contextSpinBox->setSizePolicy(QSizePolicy::Minimum,
                                  QSizePolicy::Expanding); // Mac Qt5
    m_contextSpinBoxAction = m_toolBar->addWidget(contextSpinBox);

    QToolButton *toggleDescription = new QToolButton(m_toolBar);
    toggleDescription->setIcon(QIcon(QLatin1String(Constants::ICON_TOP_BAR)));
    toggleDescription->setCheckable(true);
    toggleDescription->setChecked(m_guiController->isDescriptionVisible());
    m_toggleDescriptionAction = m_toolBar->addWidget(toggleDescription);
    slotDescriptionVisibilityChanged();

    QToolButton *reloadButton = new QToolButton(m_toolBar);
    reloadButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_RELOAD_GRAY)));
    reloadButton->setToolTip(tr("Reload Editor"));
    m_reloadAction = m_toolBar->addWidget(reloadButton);
    slotReloaderChanged();

    QToolButton *toggleSync = new QToolButton(m_toolBar);
    toggleSync->setIcon(QIcon(QLatin1String(Core::Constants::ICON_LINK)));
    toggleSync->setCheckable(true);
    toggleSync->setChecked(m_guiController->horizontalScrollBarSynchronization());
    toggleSync->setToolTip(tr("Synchronize Horizontal Scroll Bars"));
    m_toolBar->addWidget(toggleSync);

    m_diffEditorSwitcher = new QToolButton(m_toolBar);
    m_toolBar->addWidget(m_diffEditorSwitcher);
    updateDiffEditorSwitcher();

    connect(whitespaceButton, &QToolButton::clicked,
            control, &DiffEditorController::setIgnoreWhitespace);
    connect(control, &DiffEditorController::ignoreWhitespaceChanged,
            whitespaceButton, &QToolButton::setChecked);
    connect(contextSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            control, &DiffEditorController::setContextLinesNumber);
    connect(control, &DiffEditorController::contextLinesNumberChanged,
            contextSpinBox, &QSpinBox::setValue);
    connect(toggleSync, &QAbstractButton::clicked,
            m_guiController, &DiffEditorGuiController::setHorizontalScrollBarSynchronization);
    connect(toggleDescription, &QAbstractButton::clicked,
            m_guiController, &DiffEditorGuiController::setDescriptionVisible);
    connect(m_diffEditorSwitcher, &QAbstractButton::clicked,
            this, [this]() { showDiffView(nextView()); });
    connect(reloadButton, &QAbstractButton::clicked,
            control, &DiffEditorController::requestReload);
    connect(control, &DiffEditorController::reloaderChanged,
            this, &DiffEditor::slotReloaderChanged);
    connect(control, &DiffEditorController::contextLinesNumberEnablementChanged,
            this, &DiffEditor::slotReloaderChanged);

    return m_toolBar;
}

DiffEditorController *DiffEditor::controller() const
{
    return m_document->controller();
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
        const QString leftShortFileName = Utils::FileName::fromString(leftEntry.fileName).fileName();
        const QString rightShortFileName = Utils::FileName::fromString(rightEntry.fileName).fileName();
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
    const bool enabled = controller()->isDescriptionEnabled();
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

void DiffEditor::slotReloaderChanged()
{
    DiffEditorController *control = controller();
    const DiffEditorReloader *reloader = control->reloader();
    const bool contextVisible = control->isContextLinesNumberEnabled();

    m_whitespaceButtonAction->setVisible(reloader);
    m_contextLabelAction->setVisible(reloader && contextVisible);
    m_contextSpinBoxAction->setVisible(reloader && contextVisible);
    m_reloadAction->setVisible(reloader);
}

void DiffEditor::updateDiffEditorSwitcher()
{
    if (!m_diffEditorSwitcher)
        return;

    m_diffEditorSwitcher->setIcon(currentView()->icon());
    m_diffEditorSwitcher->setToolTip(currentView()->toolTip());
}

void DiffEditor::addView(IDiffView *view)
{
    QTC_ASSERT(!m_views.contains(view), return);
    m_views.append(view);
    m_stackedWidget->addWidget(view->widget());
}

IDiffView *DiffEditor::currentView() const
{
    if (m_currentViewIndex < 0)
        return 0;
    return m_views.at(m_currentViewIndex);
}

void DiffEditor::setCurrentView(IDiffView *view)
{
    const int pos = Utils::indexOf(m_views, [view](IDiffView *v) { return v == view; });
    QTC_ASSERT(pos >= 0 && pos < m_views.count(), return);
    m_currentViewIndex = pos;
}

IDiffView *DiffEditor::nextView()
{
    int pos = m_currentViewIndex + 1;
    if (pos >= m_views.count())
        pos = 0;

    return m_views.at(pos);
}

void DiffEditor::showDiffView(IDiffView *newView)
{
    QTC_ASSERT(newView, return);

    if (currentView() == newView)
        return;

    if (currentView()) // during initialization
        currentView()->setDiffEditorGuiController(0);
    setCurrentView(newView);
    currentView()->setDiffEditorGuiController(m_guiController);

    m_stackedWidget->setCurrentWidget(currentView()->widget());

    writeCurrentDiffEditorSetting(currentView());
    updateDiffEditorSwitcher();
    widget()->setFocusProxy(currentView()->widget());
}

// TODO: Remove in 3.6:
IDiffView *DiffEditor::readLegacyCurrentDiffEditorSetting()
{
    QTC_ASSERT(!m_views.isEmpty(), return 0);
    QTC_ASSERT(m_views.count() == 2, return m_views.at(0));

    QSettings *s = Core::ICore::settings();

    s->beginGroup(QLatin1String(legacySettingsGroupC));
    const bool legacyExists = s->contains(QLatin1String(useDiffEditorKeyC));
    const bool legacyEditor = s->value(
                QLatin1String(useDiffEditorKeyC), true).toBool();
    if (legacyExists)
        s->remove(QLatin1String(useDiffEditorKeyC));
    s->endGroup();

    IDiffView *currentEditor = m_views.at(0);
    if (!legacyEditor)
        currentEditor = m_views.at(1);

    if (legacyExists)
        writeCurrentDiffEditorSetting(currentEditor);

    return currentEditor;
}

IDiffView *DiffEditor::readCurrentDiffEditorSetting()
{
    // replace it with m_sideBySideEditor when dropping legacy stuff
    IDiffView *view = readLegacyCurrentDiffEditorSetting();

    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    const Core::Id id = Core::Id::fromSetting(s->value(QLatin1String(diffEditorTypeKeyC)));
    s->endGroup();

    return Utils::findOr(m_views, view, [id](IDiffView *v) { return v->id() == id; });
}

void DiffEditor::writeCurrentDiffEditorSetting(IDiffView *currentEditor)
{
    QTC_ASSERT(currentEditor, return);
    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(diffEditorTypeKeyC), currentEditor->id().toSetting());
    s->endGroup();
}

} // namespace Internal
} // namespace DiffEditor

#include "diffeditor.moc"
