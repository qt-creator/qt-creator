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

#include "diffeditor.h"
#include "diffeditorfile.h"
#include "diffeditorwidget.h"
#include "diffeditorconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>

#include <QCoreApplication>
#include <QToolButton>
#include <QSpinBox>
#include <QStyle>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolBar>
#include <QComboBox>
#include <QFileInfo>

namespace DiffEditor {

///////////////////////////////// DiffEditor //////////////////////////////////

DiffEditor::DiffEditor(DiffEditorWidget *editorWidget)
    : IEditor(0),
      m_toolWidget(0),
      m_file(new Internal::DiffEditorFile(QLatin1String(Constants::DIFF_EDITOR_MIMETYPE), this)),
      m_editorWidget(editorWidget),
      m_entriesComboBox(0)
{
    setWidget(editorWidget);
    connect(m_editorWidget, SIGNAL(navigatedToDiffFile(int)),
            this, SLOT(activateEntry(int)));
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
    return true;
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

    return toolBar;
}

QWidget *DiffEditor::toolBar()
{
    if (m_toolWidget)
        return m_toolWidget;

    // Create
    m_toolWidget = createToolBar(m_editorWidget);

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
    connect(whitespaceButton, SIGNAL(clicked(bool)),
            m_editorWidget, SLOT(setIgnoreWhitespaces(bool)));
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
    connect(contextSpinBox, SIGNAL(valueChanged(int)),
            m_editorWidget, SLOT(setContextLinesNumber(int)));
    m_toolWidget->addWidget(contextSpinBox);

    QToolButton *toggleSync = new QToolButton(m_toolWidget);
    toggleSync->setIcon(QIcon(QLatin1String(Core::Constants::ICON_LINK)));
    toggleSync->setCheckable(true);
    toggleSync->setChecked(true);
    toggleSync->setToolTip(tr("Synchronize Horizontal Scroll Bars"));
    connect(toggleSync, SIGNAL(clicked(bool)),
            m_editorWidget, SLOT(setHorizontalScrollBarSynchronization(bool)));
    m_toolWidget->addWidget(toggleSync);

    return m_toolWidget;
}

void DiffEditor::setDiff(const QList<DiffEditorWidget::DiffFilesContents> &diffFileList,
                                 const QString &workingDirectory)
{
    m_entriesComboBox->clear();
    const int count = diffFileList.count();
    for (int i = 0; i < count; i++) {
        const DiffEditorWidget::DiffFileInfo leftEntry = diffFileList.at(i).leftFileInfo;
        const DiffEditorWidget::DiffFileInfo rightEntry = diffFileList.at(i).rightFileInfo;
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
    m_editorWidget->setDiff(diffFileList, workingDirectory);
}

void DiffEditor::clear(const QString &message)
{
    m_entriesComboBox->clear();
    updateEntryToolTip();
    m_editorWidget->clear(message);
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
    m_editorWidget->navigateToDiffFile(index);
}

void DiffEditor::activateEntry(int index)
{
    m_entriesComboBox->blockSignals(true);
    m_entriesComboBox->setCurrentIndex(index);
    m_entriesComboBox->blockSignals(false);
    updateEntryToolTip();
}

} // namespace DiffEditor
