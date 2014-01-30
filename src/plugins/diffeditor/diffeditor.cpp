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
#include "sidebysidediffeditorwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/minisplitter.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/displaysettings.h>

#include <QToolButton>
#include <QSpinBox>
#include <QStyle>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolBar>
#include <QComboBox>
#include <QFileInfo>

using namespace TextEditor;

namespace DiffEditor {

namespace Internal {

class DescriptionEditor : public BaseTextEditor
{
    Q_OBJECT
public:
    DescriptionEditor(BaseTextEditorWidget *editorWidget) : BaseTextEditor(editorWidget) {}

    Core::Id id() const { return "DescriptionEditor"; }
};

class DescriptionEditorWidget : public BaseTextEditorWidget
{
    Q_OBJECT
public:
    DescriptionEditorWidget(QWidget *parent = 0);
    virtual QSize sizeHint() const;

public slots:
    void setDisplaySettings(const DisplaySettings &ds);

protected:
    BaseTextEditor *createEditor() { return new DescriptionEditor(this); }

private:
};

DescriptionEditorWidget::DescriptionEditorWidget(QWidget *parent)
    : BaseTextEditorWidget(parent)
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

} // namespace Internal

///////////////////////////////// DiffEditor //////////////////////////////////

DiffEditor::DiffEditor(SideBySideDiffEditorWidget *editorWidget)
    : IEditor(0)
    , m_toolWidget(0)
    , m_document(new Internal::DiffEditorDocument(QLatin1String(Constants::DIFF_EDITOR_MIMETYPE), this))
    , m_diffWidget(editorWidget)
    , m_diffEditorController(0)
    , m_entriesComboBox(0)
    , m_toggleDescriptionAction(0)
{
    QSplitter *splitter = new Core::MiniSplitter(Qt::Vertical);
    m_descriptionWidget = new Internal::DescriptionEditorWidget(splitter);
    m_descriptionWidget->setReadOnly(true);
    splitter->addWidget(m_descriptionWidget);
    splitter->addWidget(editorWidget);
    setWidget(splitter);

    connect(TextEditorSettings::instance(), SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            m_descriptionWidget, SLOT(setDisplaySettings(TextEditor::DisplaySettings)));
    connect(TextEditorSettings::instance(), SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            m_descriptionWidget->baseTextDocument(), SLOT(setFontSettings(TextEditor::FontSettings)));
    m_descriptionWidget->setDisplaySettings(TextEditorSettings::displaySettings());
    m_descriptionWidget->setCodeStyle(TextEditorSettings::codeStyle());
    m_descriptionWidget->baseTextDocument()->setFontSettings(TextEditorSettings::fontSettings());

    m_diffEditorController = editorWidget ? editorWidget->diffEditorController() : 0;
    if (m_diffEditorController) {
        connect(m_diffEditorController, SIGNAL(currentDiffFileIndexChanged(int)),
                this, SLOT(activateEntry(int)));
        connect(m_diffEditorController, SIGNAL(descriptionChanged(QString)),
                this, SLOT(slotDescriptionChanged(QString)));
        connect(m_diffEditorController, SIGNAL(descriptionEnablementChanged(bool)),
                this, SLOT(slotDescriptionVisibilityChanged()));
        connect(m_diffEditorController, SIGNAL(descriptionVisibilityChanged(bool)),
                this, SLOT(slotDescriptionVisibilityChanged()));

        slotDescriptionChanged(m_diffEditorController->description());
        slotDescriptionVisibilityChanged();
    }
}

DiffEditor::~DiffEditor()
{
    delete m_toolWidget;
    if (m_widget)
        delete m_widget;
}

bool DiffEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName)
    Q_UNUSED(realFileName)
    return true;
}

Core::IDocument *DiffEditor::document()
{
    return m_document;
}

Core::Id DiffEditor::id() const
{
    return Constants::DIFF_EDITOR_ID;
}

QTextCodec *DiffEditor::codec() const
{
    return m_diffWidget->codec();
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
    if (m_toolWidget)
        return m_toolWidget;

    // Create
    m_toolWidget = createToolBar(m_diffWidget);

    m_entriesComboBox = new QComboBox;
    m_entriesComboBox->setMinimumContentsLength(20);
    // Make the combo box prefer to expand
    QSizePolicy policy = m_entriesComboBox->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_entriesComboBox->setSizePolicy(policy);
    connect(m_entriesComboBox, SIGNAL(activated(int)),
            this, SLOT(entryActivated(int)));
    m_toolWidget->addWidget(m_entriesComboBox);

    QToolButton *whitespaceButton = new QToolButton(m_toolWidget);
    whitespaceButton->setText(tr("Ignore Whitespace"));
    whitespaceButton->setCheckable(true);
    whitespaceButton->setChecked(true);
    m_toolWidget->addWidget(whitespaceButton);

    QLabel *contextLabel = new QLabel(m_toolWidget);
    contextLabel->setText(tr("Context Lines:"));
    contextLabel->setContentsMargins(6, 0, 6, 0);
    m_toolWidget->addWidget(contextLabel);

    QSpinBox *contextSpinBox = new QSpinBox(m_toolWidget);
    contextSpinBox->setRange(-1, 100);
    contextSpinBox->setValue(3);
    contextSpinBox->setFrame(false);
    contextSpinBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding); // Mac Qt5
    m_toolWidget->addWidget(contextSpinBox);

    QToolButton *toggleSync = new QToolButton(m_toolWidget);
    toggleSync->setIcon(QIcon(QLatin1String(Core::Constants::ICON_LINK)));
    toggleSync->setCheckable(true);
    toggleSync->setChecked(true);
    toggleSync->setToolTip(tr("Synchronize Horizontal Scroll Bars"));
    m_toolWidget->addWidget(toggleSync);

    QToolButton *toggleDescription = new QToolButton(m_toolWidget);
    toggleDescription->setIcon(QIcon(QLatin1String(Core::Constants::ICON_TOGGLE_TOPBAR)));
    toggleDescription->setCheckable(true);
    toggleDescription->setChecked(true);
    m_toggleDescriptionAction = m_toolWidget->addWidget(toggleDescription);
    slotDescriptionVisibilityChanged();

    if (m_diffEditorController) {
        connect(whitespaceButton, SIGNAL(clicked(bool)),
                m_diffEditorController, SLOT(setIgnoreWhitespaces(bool)));
        connect(contextSpinBox, SIGNAL(valueChanged(int)),
                m_diffEditorController, SLOT(setContextLinesNumber(int)));
        connect(toggleSync, SIGNAL(clicked(bool)),
                m_diffEditorController, SLOT(setHorizontalScrollBarSynchronization(bool)));
        connect(toggleDescription, SIGNAL(clicked(bool)),
                m_diffEditorController, SLOT(setDescriptionVisible(bool)));
        // TODO: synchronize in opposite direction too
    }

    return m_toolWidget;
}

void DiffEditor::setDiff(const QList<DiffEditorController::DiffFilesContents> &diffFileList,
                                 const QString &workingDirectory)
{
    m_entriesComboBox->clear();
    const int count = diffFileList.count();
    for (int i = 0; i < count; i++) {
        const DiffEditorController::DiffFileInfo leftEntry = diffFileList.at(i).leftFileInfo;
        const DiffEditorController::DiffFileInfo rightEntry = diffFileList.at(i).rightFileInfo;
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
                        .arg(leftEntry.typeInfo, rightEntry.typeInfo, leftEntry.fileName);
            }
        } else {
            if (leftShortFileName == rightShortFileName) {
                itemText = leftShortFileName;
            } else {
                itemText = tr("%1 vs. %2")
                        .arg(leftShortFileName, rightShortFileName);
            }

            if (leftEntry.typeInfo.isEmpty() && rightEntry.typeInfo.isEmpty()) {
                itemToolTip = tr("%1 vs. %2")
                        .arg(leftEntry.fileName, rightEntry.fileName);
            } else {
                itemToolTip = tr("[%1] %2 vs. [%3] %4")
                        .arg(leftEntry.typeInfo, leftEntry.fileName, rightEntry.typeInfo, rightEntry.fileName);
            }
        }
        m_entriesComboBox->addItem(itemText);
        m_entriesComboBox->setItemData(m_entriesComboBox->count() - 1, itemToolTip, Qt::ToolTipRole);
    }
    updateEntryToolTip();
    if (m_diffEditorController)
        m_diffEditorController->setDiffContents(diffFileList, workingDirectory);
}

void DiffEditor::setDescription(const QString &description)
{
    if (m_diffEditorController)
        m_diffEditorController->setDescription(description);
}

void DiffEditor::setDescriptionEnabled(bool on)
{
    if (m_diffEditorController)
        m_diffEditorController->setDescriptionEnabled(on);
}

void DiffEditor::clear(const QString &message)
{
    m_entriesComboBox->clear();
    updateEntryToolTip();
    if (m_diffEditorController)
        m_diffEditorController->clear(message);
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
    if (m_diffEditorController)
        m_diffEditorController->setCurrentDiffFileIndex(index);
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
    if (!m_diffEditorController)
        return;

    const bool visible = m_diffEditorController->isDescriptionVisible();
    const bool enabled = m_diffEditorController->isDescriptionEnabled();

    m_descriptionWidget->setVisible(visible && enabled);

    if (!m_toggleDescriptionAction)
        return;

    QWidget *toggle = m_toolWidget->widgetForAction(m_toggleDescriptionAction);
    if (visible)
        toggle->setToolTip(tr("Hide Change Description"));
    else
        toggle->setToolTip(tr("Show Change Description"));

    m_toggleDescriptionAction->setVisible(enabled);

}

} // namespace DiffEditor

#include "diffeditor.moc"
