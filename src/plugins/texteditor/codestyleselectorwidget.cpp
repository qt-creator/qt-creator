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

#include "codestyleselectorwidget.h"
#include "ui_codestyleselectorwidget.h"
#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"
#include "codestylepool.h"
#include "tabsettings.h"

#include <utils/fileutils.h>

#include <QPushButton>
#include <QDialogButtonBox>
#include <QDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>

#include <QDebug>

using namespace TextEditor;

namespace TextEditor {
namespace Internal {

class CodeStyleDialog : public QDialog
{
    Q_OBJECT
public:
    CodeStyleDialog(ICodeStylePreferencesFactory *factory,
                    ICodeStylePreferences *codeStyle, QWidget *parent = 0);
    ~CodeStyleDialog();
    ICodeStylePreferences *codeStyle() const;
private:
    void slotCopyClicked();
    void slotDisplayNameChanged();

    ICodeStylePreferences *m_codeStyle;
    QLineEdit *m_lineEdit;
    QDialogButtonBox *m_buttons;
    QLabel *m_warningLabel;
    QPushButton *m_copyButton;
    QString m_originalDisplayName;
};

CodeStyleDialog::CodeStyleDialog(ICodeStylePreferencesFactory *factory,
                ICodeStylePreferences *codeStyle, QWidget *parent)
    : QDialog(parent),
      m_warningLabel(0),
      m_copyButton(0)
{
    setWindowTitle(tr("Edit Code Style"));
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("Code style name:"));
    m_lineEdit = new QLineEdit(codeStyle->displayName(), this);
    QHBoxLayout *nameLayout = new QHBoxLayout();
    nameLayout->addWidget(label);
    nameLayout->addWidget(m_lineEdit);
    layout->addLayout(nameLayout);

    if (codeStyle->isReadOnly()) {
        QHBoxLayout *warningLayout = new QHBoxLayout();
        m_warningLabel = new QLabel(
                    tr("You cannot save changes to a built-in code style. "
                       "Copy it first to create your own version."), this);
        QFont font = m_warningLabel->font();
        font.setItalic(true);
        m_warningLabel->setFont(font);
        m_warningLabel->setWordWrap(true);
        m_copyButton = new QPushButton(tr("Copy Built-in Code Style"), this);
        m_copyButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(m_copyButton, &QAbstractButton::clicked, this, &CodeStyleDialog::slotCopyClicked);
        warningLayout->addWidget(m_warningLabel);
        warningLayout->addWidget(m_copyButton);
        layout->addLayout(warningLayout);
    }

    m_originalDisplayName = codeStyle->displayName();
    m_codeStyle = factory->createCodeStyle();
    m_codeStyle->setTabSettings(codeStyle->tabSettings());
    m_codeStyle->setValue(codeStyle->value());
    m_codeStyle->setId(codeStyle->id());
    m_codeStyle->setDisplayName(m_originalDisplayName);
    QWidget *editor = factory->createEditor(m_codeStyle, this);

    m_buttons = new QDialogButtonBox(
                QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    if (codeStyle->isReadOnly()) {
        QPushButton *okButton = m_buttons->button(QDialogButtonBox::Ok);
        okButton->setEnabled(false);
    }

    if (editor)
        layout->addWidget(editor);
    layout->addWidget(m_buttons);
    resize(850, 600);

    connect(m_lineEdit, &QLineEdit::textChanged, this, &CodeStyleDialog::slotDisplayNameChanged);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ICodeStylePreferences *CodeStyleDialog::codeStyle() const
{
    return m_codeStyle;
}

void CodeStyleDialog::slotCopyClicked()
{
    if (m_warningLabel)
        m_warningLabel->hide();
    if (m_copyButton)
        m_copyButton->hide();
    QPushButton *okButton = m_buttons->button(QDialogButtonBox::Ok);
    okButton->setEnabled(true);
    if (m_lineEdit->text() == m_originalDisplayName)
        m_lineEdit->setText(tr("%1 (Copy)").arg(m_lineEdit->text()));
    m_lineEdit->selectAll();
}

void CodeStyleDialog::slotDisplayNameChanged()
{
    m_codeStyle->setDisplayName(m_lineEdit->text());
}

CodeStyleDialog::~CodeStyleDialog()
{
    delete m_codeStyle;
}

}
}

CodeStyleSelectorWidget::CodeStyleSelectorWidget(ICodeStylePreferencesFactory *factory, QWidget *parent) :
    QWidget(parent),
    m_factory(factory),
    m_codeStyle(0),
    m_ui(new Internal::Ui::CodeStyleSelectorWidget),
    m_ignoreGuiSignals(false)
{
    m_ui->setupUi(this);
    m_ui->importButton->setEnabled(false);
    m_ui->exportButton->setEnabled(false);

    connect(m_ui->delegateComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &CodeStyleSelectorWidget::slotComboBoxActivated);
    connect(m_ui->copyButton, &QAbstractButton::clicked,
            this, &CodeStyleSelectorWidget::slotCopyClicked);
    connect(m_ui->editButton, &QAbstractButton::clicked,
            this, &CodeStyleSelectorWidget::slotEditClicked);
    connect(m_ui->removeButton, &QAbstractButton::clicked,
            this, &CodeStyleSelectorWidget::slotRemoveClicked);
    connect(m_ui->importButton, &QAbstractButton::clicked,
            this, &CodeStyleSelectorWidget::slotImportClicked);
    connect(m_ui->exportButton, &QAbstractButton::clicked,
            this, &CodeStyleSelectorWidget::slotExportClicked);
}

CodeStyleSelectorWidget::~CodeStyleSelectorWidget()
{
    delete m_ui;
}

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

        m_ui->exportButton->setEnabled(false);
        m_ui->importButton->setEnabled(false);
        m_ui->delegateComboBox->clear();
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
            m_ui->exportButton->setEnabled(true);
            m_ui->importButton->setEnabled(true);
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
    if (m_ignoreGuiSignals)
        return;

    if (index < 0 || index >= m_ui->delegateComboBox->count())
        return;
    ICodeStylePreferences *delegate =
            m_ui->delegateComboBox->itemData(index).value<ICodeStylePreferences *>();

    const bool wasBlocked = blockSignals(true);
    m_codeStyle->setCurrentDelegate(delegate);
    blockSignals(wasBlocked);
}

void CodeStyleSelectorWidget::slotCurrentDelegateChanged(ICodeStylePreferences *delegate)
{
    m_ignoreGuiSignals = true;
    m_ui->delegateComboBox->setCurrentIndex(m_ui->delegateComboBox->findData(QVariant::fromValue(delegate)));
    m_ui->delegateComboBox->setToolTip(m_ui->delegateComboBox->currentText());
    m_ignoreGuiSignals = false;

    const bool removeEnabled = delegate && !delegate->isReadOnly() && !delegate->currentDelegate();
    m_ui->removeButton->setEnabled(removeEnabled);
}

void CodeStyleSelectorWidget::slotCopyClicked()
{
    if (!m_codeStyle)
        return;

    CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
    ICodeStylePreferences *currentPreferences = m_codeStyle->currentPreferences();
    bool ok = false;
    const QString newName = QInputDialog::getText(this,
                                                  tr("Copy Code Style"),
                                                  tr("Code style name:"),
                                                  QLineEdit::Normal,
                                                  tr("%1 (Copy)").arg(currentPreferences->displayName()),
                                                  &ok);
    if (!ok || newName.trimmed().isEmpty())
        return;
    ICodeStylePreferences *copy = codeStylePool->cloneCodeStyle(currentPreferences);
    if (copy) {
        copy->setDisplayName(newName);
        m_codeStyle->setCurrentDelegate(copy);
    }
}

void CodeStyleSelectorWidget::slotEditClicked()
{
    if (!m_codeStyle)
        return;

    ICodeStylePreferences *codeStyle = m_codeStyle->currentPreferences();
    // check if it's read-only

    Internal::CodeStyleDialog dialog(m_factory, codeStyle, this);
    if (dialog.exec() == QDialog::Accepted) {
        ICodeStylePreferences *dialogCodeStyle = dialog.codeStyle();
        if (codeStyle->isReadOnly()) {
            CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
            codeStyle = codeStylePool->cloneCodeStyle(dialogCodeStyle);
            if (codeStyle)
                m_codeStyle->setCurrentDelegate(codeStyle);
            return;
        }
        codeStyle->setTabSettings(dialogCodeStyle->tabSettings());
        codeStyle->setValue(dialogCodeStyle->value());
        codeStyle->setDisplayName(dialogCodeStyle->displayName());
    }
}

void CodeStyleSelectorWidget::slotRemoveClicked()
{
    if (!m_codeStyle)
        return;

    CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
    ICodeStylePreferences *currentPreferences = m_codeStyle->currentPreferences();

    QMessageBox messageBox(QMessageBox::Warning,
                           tr("Delete Code Style"),
                           tr("Are you sure you want to delete this code style permanently?"),
                           QMessageBox::Discard | QMessageBox::Cancel,
                           this);

    // Change the text and role of the discard button
    QPushButton *deleteButton = static_cast<QPushButton*>(messageBox.button(QMessageBox::Discard));
    deleteButton->setText(tr("Delete"));
    messageBox.addButton(deleteButton, QMessageBox::AcceptRole);
    messageBox.setDefaultButton(deleteButton);

    connect(deleteButton, &QAbstractButton::clicked, &messageBox, &QDialog::accept);
    if (messageBox.exec() == QDialog::Accepted)
        codeStylePool->removeCodeStyle(currentPreferences);
}

void CodeStyleSelectorWidget::slotImportClicked()
{
    const Utils::FileName fileName =
            Utils::FileName::fromString(QFileDialog::getOpenFileName(this, tr("Import Code Style"), QString(),
                                                                     tr("Code styles (*.xml);;All files (*)")));
    if (!fileName.isEmpty()) {
        CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
        ICodeStylePreferences *importedStyle = codeStylePool->importCodeStyle(fileName);
        if (importedStyle)
            m_codeStyle->setCurrentDelegate(importedStyle);
        else
            QMessageBox::warning(this, tr("Import Code Style"),
                                 tr("Cannot import code style from %1"), fileName.toUserOutput());
    }
}

void CodeStyleSelectorWidget::slotExportClicked()
{
    ICodeStylePreferences *currentPreferences = m_codeStyle->currentPreferences();
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Export Code Style"),
                             QString::fromUtf8(currentPreferences->id() + ".xml"),
                             tr("Code styles (*.xml);;All files (*)"));
    if (!fileName.isEmpty()) {
        CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
        codeStylePool->exportCodeStyle(Utils::FileName::fromString(fileName), currentPreferences);
    }
}

void CodeStyleSelectorWidget::slotCodeStyleAdded(ICodeStylePreferences *codeStylePreferences)
{
    if (codeStylePreferences == m_codeStyle
            || codeStylePreferences->id() == m_codeStyle->id())
        return;

    const QVariant data = QVariant::fromValue(codeStylePreferences);
    const QString name = displayName(codeStylePreferences);
    m_ui->delegateComboBox->addItem(name, data);
    m_ui->delegateComboBox->setItemData(m_ui->delegateComboBox->count() - 1, name, Qt::ToolTipRole);
    connect(codeStylePreferences, &ICodeStylePreferences::displayNameChanged,
            this, &CodeStyleSelectorWidget::slotUpdateName);
    if (codeStylePreferences->delegatingPool()) {
        connect(codeStylePreferences, &ICodeStylePreferences::currentPreferencesChanged,
                this, &CodeStyleSelectorWidget::slotUpdateName);
    }
}

void CodeStyleSelectorWidget::slotCodeStyleRemoved(ICodeStylePreferences *codeStylePreferences)
{
    m_ignoreGuiSignals = true;
    m_ui->delegateComboBox->removeItem(m_ui->delegateComboBox->findData(QVariant::fromValue(codeStylePreferences)));
    disconnect(codeStylePreferences, &ICodeStylePreferences::displayNameChanged,
               this, &CodeStyleSelectorWidget::slotUpdateName);
    if (codeStylePreferences->delegatingPool()) {
        disconnect(codeStylePreferences, &ICodeStylePreferences::currentPreferencesChanged,
                   this, &CodeStyleSelectorWidget::slotUpdateName);
    }
    m_ignoreGuiSignals = false;
}

void CodeStyleSelectorWidget::slotUpdateName()
{
    ICodeStylePreferences *changedCodeStyle = qobject_cast<ICodeStylePreferences *>(sender());
    if (!changedCodeStyle)
        return;

    updateName(changedCodeStyle);

    QList<ICodeStylePreferences *> codeStyles = m_codeStyle->delegatingPool()->codeStyles();
    for (int i = 0; i < codeStyles.count(); i++) {
        ICodeStylePreferences *codeStyle = codeStyles.at(i);
        if (codeStyle->currentDelegate() == changedCodeStyle)
            updateName(codeStyle);
    }

    m_ui->delegateComboBox->setToolTip(m_ui->delegateComboBox->currentText());
}

void CodeStyleSelectorWidget::updateName(ICodeStylePreferences *codeStyle)
{
    const int idx = m_ui->delegateComboBox->findData(QVariant::fromValue(codeStyle));
    if (idx < 0)
        return;

    const QString name = displayName(codeStyle);
    m_ui->delegateComboBox->setItemText(idx, name);
    m_ui->delegateComboBox->setItemData(idx, name, Qt::ToolTipRole);
}

QString CodeStyleSelectorWidget::displayName(ICodeStylePreferences *codeStyle) const
{
    QString name = codeStyle->displayName();
    if (codeStyle->currentDelegate())
        name = tr("%1 [proxy: %2]").arg(name).arg(codeStyle->currentDelegate()->displayName());
    if (codeStyle->isReadOnly())
        name = tr("%1 [built-in]").arg(name);
    return name;
}

#include "codestyleselectorwidget.moc"
