// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codestyleselectorwidget.h"

#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"
#include "codestylepool.h"
#include "texteditortr.h"

#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

using namespace Utils;

namespace TextEditor {

CodeStyleSelectorWidget::CodeStyleSelectorWidget(ICodeStylePreferencesFactory *factory,
                                                 ProjectExplorer::Project *project,
                                                 QWidget *parent)
    : QWidget(parent)
    , m_factory(factory)
    , m_project(project)
{
    m_delegateComboBox = new QComboBox(this);
    m_delegateComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto copyButton = new QPushButton(Tr::tr("Copy..."));

    m_removeButton = new QPushButton(Tr::tr("Remove"));

    m_exportButton = new QPushButton(Tr::tr("Export..."));
    m_exportButton->setEnabled(false);

    m_importButton = new QPushButton(Tr::tr("Import..."));
    m_importButton->setEnabled(false);

    using namespace Layouting;

    Column {
        Grid {
            Tr::tr("Current settings:"),
            m_delegateComboBox,
            copyButton,
            m_removeButton,
            m_exportButton,
            m_importButton
        },
        noMargin,
    }.attachTo(this);

    connect(m_delegateComboBox, &QComboBox::activated,
            this, &CodeStyleSelectorWidget::slotComboBoxActivated);
    connect(copyButton, &QAbstractButton::clicked,
            this, &CodeStyleSelectorWidget::slotCopyClicked);
    connect(m_removeButton, &QAbstractButton::clicked,
            this, &CodeStyleSelectorWidget::slotRemoveClicked);
    connect(m_importButton, &QAbstractButton::clicked,
            this, &CodeStyleSelectorWidget::slotImportClicked);
    connect(m_exportButton, &QAbstractButton::clicked,
            this, &CodeStyleSelectorWidget::slotExportClicked);
}

CodeStyleSelectorWidget::~CodeStyleSelectorWidget() = default;

void CodeStyleSelectorWidget::setCodeStyle(ICodeStylePreferences *codeStyle)
{
    if (m_codeStyle == codeStyle)
        return; // nothing changes

    // cleanup old
    if (m_codeStyle) {
        CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
        if (codeStylePool) {
            disconnect(codeStylePool, &CodeStylePool::codeStyleAdded,
                    this, &CodeStyleSelectorWidget::slotCodeStyleAdded);
            disconnect(codeStylePool, &CodeStylePool::codeStyleRemoved,
                    this, &CodeStyleSelectorWidget::slotCodeStyleRemoved);
        }
        disconnect(m_codeStyle, &ICodeStylePreferences::currentDelegateChanged,
                this, &CodeStyleSelectorWidget::slotCurrentDelegateChanged);

        m_exportButton->setEnabled(false);
        m_importButton->setEnabled(false);
        m_delegateComboBox->clear();
    }
    m_codeStyle = codeStyle;
    // fillup new
    if (m_codeStyle) {
        QList<ICodeStylePreferences *> delegates;
        CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
        if (codeStylePool) {
            delegates = codeStylePool->codeStyles();

            connect(codeStylePool, &CodeStylePool::codeStyleAdded,
                    this, &CodeStyleSelectorWidget::slotCodeStyleAdded);
            connect(codeStylePool, &CodeStylePool::codeStyleRemoved,
                    this, &CodeStyleSelectorWidget::slotCodeStyleRemoved);
            m_exportButton->setEnabled(true);
            m_importButton->setEnabled(true);
        }

        for (int i = 0; i < delegates.count(); i++)
            slotCodeStyleAdded(delegates.at(i));

        slotCurrentDelegateChanged(m_codeStyle->currentDelegate());

        connect(m_codeStyle, &ICodeStylePreferences::currentDelegateChanged,
                this, &CodeStyleSelectorWidget::slotCurrentDelegateChanged);
    }
}

void CodeStyleSelectorWidget::slotComboBoxActivated(int index)
{
    if (m_ignoreChanges.isLocked())
        return;

    if (index < 0 || index >= m_delegateComboBox->count())
        return;
    auto delegate = m_delegateComboBox->itemData(index).value<ICodeStylePreferences *>();

    QSignalBlocker blocker(this);
    m_codeStyle->setCurrentDelegate(delegate);
}

void CodeStyleSelectorWidget::slotCurrentDelegateChanged(ICodeStylePreferences *delegate)
{
    {
        const GuardLocker locker(m_ignoreChanges);
        m_delegateComboBox->setCurrentIndex(m_delegateComboBox->findData(QVariant::fromValue(delegate)));
        m_delegateComboBox->setToolTip(m_delegateComboBox->currentText());
    }

    const bool removeEnabled = delegate && !delegate->isReadOnly() && !delegate->currentDelegate();
    m_removeButton->setEnabled(removeEnabled);
}

void CodeStyleSelectorWidget::slotCopyClicked()
{
    if (!m_codeStyle)
        return;

    CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
    ICodeStylePreferences *currentPreferences = m_codeStyle->currentPreferences();
    bool ok = false;
    const QString newName = QInputDialog::getText(this,
                                                  Tr::tr("Copy Code Style"),
                                                  Tr::tr("Code style name:"),
                                                  QLineEdit::Normal,
                                                  Tr::tr("%1 (Copy)").arg(currentPreferences->displayName()),
                                                  &ok);
    if (!ok || newName.trimmed().isEmpty())
        return;
    ICodeStylePreferences *copy = codeStylePool->cloneCodeStyle(currentPreferences);
    if (copy) {
        copy->setDisplayName(newName);
        m_codeStyle->setCurrentDelegate(copy);
    }
}

void CodeStyleSelectorWidget::slotRemoveClicked()
{
    if (!m_codeStyle)
        return;

    CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
    ICodeStylePreferences *currentPreferences = m_codeStyle->currentPreferences();

    QMessageBox messageBox(QMessageBox::Warning,
                           Tr::tr("Delete Code Style"),
                           Tr::tr("Are you sure you want to delete this code style permanently?"),
                           QMessageBox::Discard | QMessageBox::Cancel,
                           this);

    // Change the text and role of the discard button
    auto deleteButton = static_cast<QPushButton*>(messageBox.button(QMessageBox::Discard));
    deleteButton->setText(Tr::tr("Delete"));
    messageBox.addButton(deleteButton, QMessageBox::AcceptRole);
    messageBox.setDefaultButton(deleteButton);

    connect(deleteButton, &QAbstractButton::clicked, &messageBox, &QDialog::accept);
    if (messageBox.exec() == QDialog::Accepted)
        codeStylePool->removeCodeStyle(currentPreferences);
}

void CodeStyleSelectorWidget::slotImportClicked()
{
    const FilePath fileName =
            FileUtils::getOpenFilePath(this, Tr::tr("Import Code Style"), {},
                                       Tr::tr("Code styles (*.xml);;All files (*)"));
    if (!fileName.isEmpty()) {
        CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
        ICodeStylePreferences *importedStyle = codeStylePool->importCodeStyle(fileName);
        if (importedStyle)
            m_codeStyle->setCurrentDelegate(importedStyle);
        else
            QMessageBox::warning(this,
                                 Tr::tr("Import Code Style"),
                                 Tr::tr("Cannot import code style from \"%1\".")
                                     .arg(fileName.toUserOutput()));
    }
}

void CodeStyleSelectorWidget::slotExportClicked()
{
    ICodeStylePreferences *currentPreferences = m_codeStyle->currentPreferences();
    const FilePath filePath = FileUtils::getSaveFilePath(this, Tr::tr("Export Code Style"),
                             FilePath::fromString(QString::fromUtf8(currentPreferences->id() + ".xml")),
                             Tr::tr("Code styles (*.xml);;All files (*)"));
    if (!filePath.isEmpty()) {
        CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
        codeStylePool->exportCodeStyle(filePath, currentPreferences);
    }
}

void CodeStyleSelectorWidget::slotCodeStyleAdded(ICodeStylePreferences *codeStylePreferences)
{
    if (codeStylePreferences == m_codeStyle
            || codeStylePreferences->id() == m_codeStyle->id())
        return;

    const QVariant data = QVariant::fromValue(codeStylePreferences);
    const QString name = displayName(codeStylePreferences);
    m_delegateComboBox->addItem(name, data);
    m_delegateComboBox->setItemData(m_delegateComboBox->count() - 1, name, Qt::ToolTipRole);
    connect(codeStylePreferences, &ICodeStylePreferences::displayNameChanged,
            this, [this, codeStylePreferences] { slotUpdateName(codeStylePreferences); });
    if (codeStylePreferences->delegatingPool()) {
        connect(codeStylePreferences, &ICodeStylePreferences::currentPreferencesChanged,
                this, [this, codeStylePreferences] { slotUpdateName(codeStylePreferences); });
    }
}

void CodeStyleSelectorWidget::slotCodeStyleRemoved(ICodeStylePreferences *codeStylePreferences)
{
    const GuardLocker locker(m_ignoreChanges);
    m_delegateComboBox->removeItem(m_delegateComboBox->findData(
                                           QVariant::fromValue(codeStylePreferences)));
    disconnect(codeStylePreferences, &ICodeStylePreferences::displayNameChanged, this, nullptr);
    if (codeStylePreferences->delegatingPool()) {
        disconnect(codeStylePreferences, &ICodeStylePreferences::currentPreferencesChanged,
                   this, nullptr);
    }
}

void CodeStyleSelectorWidget::slotUpdateName(ICodeStylePreferences *codeStylePreferences)
{
    updateName(codeStylePreferences);

    QList<ICodeStylePreferences *> codeStyles = m_codeStyle->delegatingPool()->codeStyles();
    for (int i = 0; i < codeStyles.count(); i++) {
        ICodeStylePreferences *codeStyle = codeStyles.at(i);
        if (codeStyle->currentDelegate() == codeStylePreferences)
            updateName(codeStyle);
    }

    m_delegateComboBox->setToolTip(m_delegateComboBox->currentText());
}

void CodeStyleSelectorWidget::updateName(ICodeStylePreferences *codeStyle)
{
    const int idx = m_delegateComboBox->findData(QVariant::fromValue(codeStyle));
    if (idx < 0)
        return;

    const QString name = displayName(codeStyle);
    m_delegateComboBox->setItemText(idx, name);
    m_delegateComboBox->setItemData(idx, name, Qt::ToolTipRole);
}

QString CodeStyleSelectorWidget::displayName(ICodeStylePreferences *codeStyle) const
{
    QString name = codeStyle->displayName();
    if (codeStyle->currentDelegate())
        name = Tr::tr("%1 [proxy: %2]").arg(name).arg(codeStyle->currentDelegate()->displayName());
    if (codeStyle->isReadOnly())
        name = Tr::tr("%1 [built-in]").arg(name);
    return name;
}

} // TextEditor
