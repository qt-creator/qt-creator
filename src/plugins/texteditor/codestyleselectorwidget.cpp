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

#include "codestyleselectorwidget.h"
#include "ui_codestyleselectorwidget.h"
#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"
#include "codestylepool.h"
#include "tabsettings.h"

#include <QComboBox>
#include <QBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QMenu>
#include <QDialogButtonBox>
#include <QDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QSignalMapper>

#include <QDebug>

using namespace TextEditor;

Q_DECLARE_METATYPE(TextEditor::ICodeStylePreferences *)

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
private slots:
    void slotCopyClicked();
    void slotDisplayNameChanged();
private:
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
        connect(m_copyButton, SIGNAL(clicked()),
                this, SLOT(slotCopyClicked()));
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

    connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotDisplayNameChanged()));
    connect(m_buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
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

    connect(m_ui->delegateComboBox, SIGNAL(activated(int)),
            this, SLOT(slotComboBoxActivated(int)));
    connect(m_ui->copyButton, SIGNAL(clicked()),
            this, SLOT(slotCopyClicked()));
    connect(m_ui->editButton, SIGNAL(clicked()),
            this, SLOT(slotEditClicked()));
    connect(m_ui->removeButton, SIGNAL(clicked()),
            this, SLOT(slotRemoveClicked()));
    connect(m_ui->importButton, SIGNAL(clicked()),
            this, SLOT(slotImportClicked()));
    connect(m_ui->exportButton, SIGNAL(clicked()),
            this, SLOT(slotExportClicked()));
}

CodeStyleSelectorWidget::~CodeStyleSelectorWidget()
{
    delete m_ui;
}

void CodeStyleSelectorWidget::setCodeStyle(TextEditor::ICodeStylePreferences *codeStyle)
{
    if (m_codeStyle == codeStyle)
        return; // nothing changes

    // cleanup old
    if (m_codeStyle) {
        CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
        if (codeStylePool) {
            disconnect(codeStylePool, SIGNAL(codeStyleAdded(ICodeStylePreferences*)),
                    this, SLOT(slotCodeStyleAdded(ICodeStylePreferences*)));
            disconnect(codeStylePool, SIGNAL(codeStyleRemoved(ICodeStylePreferences*)),
                    this, SLOT(slotCodeStyleRemoved(ICodeStylePreferences*)));
        }
        disconnect(m_codeStyle, SIGNAL(currentDelegateChanged(ICodeStylePreferences*)),
                this, SLOT(slotCurrentDelegateChanged(ICodeStylePreferences*)));

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

            connect(codeStylePool, SIGNAL(codeStyleAdded(ICodeStylePreferences*)),
                    this, SLOT(slotCodeStyleAdded(ICodeStylePreferences*)));
            connect(codeStylePool, SIGNAL(codeStyleRemoved(ICodeStylePreferences*)),
                    this, SLOT(slotCodeStyleRemoved(ICodeStylePreferences*)));
            m_ui->exportButton->setEnabled(true);
            m_ui->importButton->setEnabled(true);
        }

        for (int i = 0; i < delegates.count(); i++)
            slotCodeStyleAdded(delegates.at(i));

        slotCurrentDelegateChanged(m_codeStyle->currentDelegate());

        connect(m_codeStyle, SIGNAL(currentDelegateChanged(TextEditor::ICodeStylePreferences*)),
                this, SLOT(slotCurrentDelegateChanged(TextEditor::ICodeStylePreferences*)));
    }
}

void CodeStyleSelectorWidget::slotComboBoxActivated(int index)
{
    if (m_ignoreGuiSignals)
        return;

    if (index < 0 || index >= m_ui->delegateComboBox->count())
        return;
    TextEditor::ICodeStylePreferences *delegate =
            m_ui->delegateComboBox->itemData(index).value<TextEditor::ICodeStylePreferences *>();

    const bool wasBlocked = blockSignals(true);
    m_codeStyle->setCurrentDelegate(delegate);
    blockSignals(wasBlocked);
}

void CodeStyleSelectorWidget::slotCurrentDelegateChanged(TextEditor::ICodeStylePreferences *delegate)
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
    if (!ok)
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

    connect(deleteButton, SIGNAL(clicked()), &messageBox, SLOT(accept()));
    if (messageBox.exec() == QDialog::Accepted)
        codeStylePool->removeCodeStyle(currentPreferences);
}

void CodeStyleSelectorWidget::slotImportClicked()
{
    const Utils::FileName fileName =
            Utils::FileName::fromString(QFileDialog::getOpenFileName(this, tr("Import Code Style"), QString::null,
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
                             currentPreferences->id() + QLatin1String(".xml"),
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
    connect(codeStylePreferences, SIGNAL(displayNameChanged(QString)),
            this, SLOT(slotUpdateName()));
    if (codeStylePreferences->delegatingPool()) {
        connect(codeStylePreferences, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)),
                this, SLOT(slotUpdateName()));
    }
}

void CodeStyleSelectorWidget::slotCodeStyleRemoved(ICodeStylePreferences *codeStylePreferences)
{
    m_ignoreGuiSignals = true;
    m_ui->delegateComboBox->removeItem(m_ui->delegateComboBox->findData(QVariant::fromValue(codeStylePreferences)));
    disconnect(codeStylePreferences, SIGNAL(displayNameChanged(QString)),
            this, SLOT(slotUpdateName()));
    if (codeStylePreferences->delegatingPool()) {
        disconnect(codeStylePreferences, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)),
                this, SLOT(slotUpdateName()));
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

QString CodeStyleSelectorWidget::searchKeywords() const
{
    // no useful keywords here
    return QString();
}

#include "codestyleselectorwidget.moc"
