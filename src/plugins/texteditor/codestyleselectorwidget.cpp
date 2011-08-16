/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "codestyleselectorwidget.h"
#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"
#include "codestylepool.h"
#include "tabsettings.h"

#include <QtGui/QComboBox>
#include <QtGui/QBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QMenu>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QDialog>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtCore/QTextStream>
#include <QtCore/QSignalMapper>

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
    QString displayName() const;
private:
    ICodeStylePreferences *m_codeStyle;
    QLineEdit *m_lineEdit;
};

CodeStyleDialog::CodeStyleDialog(ICodeStylePreferencesFactory *factory,
                ICodeStylePreferences *codeStyle, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit Code Style"));
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("Code style name:"));
    m_lineEdit = new QLineEdit(codeStyle->displayName(), this);
    QHBoxLayout *nameLayout = new QHBoxLayout();
    nameLayout->addWidget(label);
    nameLayout->addWidget(m_lineEdit);
    layout->addLayout(nameLayout);
    m_codeStyle = factory->createCodeStyle();
    m_codeStyle->setTabSettings(codeStyle->tabSettings());
    m_codeStyle->setValue(codeStyle->value());
    QWidget *editor = factory->createEditor(m_codeStyle, this);
    QDialogButtonBox *buttons = new QDialogButtonBox(
                QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    if (editor)
        layout->addWidget(editor);
    layout->addWidget(buttons);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

ICodeStylePreferences *CodeStyleDialog::codeStyle() const
{
    return m_codeStyle;
}

QString CodeStyleDialog::displayName() const
{
    return m_lineEdit->text();
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
    m_layout(0),
    m_comboBox(0),
    m_comboBoxLabel(0),
    m_ignoreGuiSignals(false)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(QMargins());
    m_copyButton = new QPushButton(tr("Copy..."), this);
    m_editButton = new QPushButton(tr("Edit..."), this);
    m_removeButton = new QPushButton(tr("Remove"), this);
    m_importButton = new QPushButton(tr("Import..."), this);
    m_exportButton = new QPushButton(tr("Export..."), this);
    m_importButton->setEnabled(false);
    m_exportButton->setEnabled(false);

    m_comboBoxLabel = new QLabel(tr("Current settings:"), this);
    m_comboBoxLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_layout->addWidget(m_comboBoxLabel);
    m_comboBox = new QComboBox(this);
    m_comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_layout->addWidget(m_comboBox);
    connect(m_comboBox, SIGNAL(activated(int)),
            this, SLOT(slotComboBoxActivated(int)));

    m_layout->addWidget(m_copyButton);
    m_layout->addWidget(m_editButton);
    m_layout->addWidget(m_removeButton);
    m_layout->addWidget(m_importButton);
    m_layout->addWidget(m_exportButton);

    connect(m_copyButton, SIGNAL(clicked()),
            this, SLOT(slotCopyClicked()));
    connect(m_editButton, SIGNAL(clicked()),
            this, SLOT(slotEditClicked()));
    connect(m_removeButton, SIGNAL(clicked()),
            this, SLOT(slotRemoveClicked()));
    connect(m_importButton, SIGNAL(clicked()),
            this, SLOT(slotImportClicked()));
    connect(m_exportButton, SIGNAL(clicked()),
            this, SLOT(slotExportClicked()));
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

        m_exportButton->setEnabled(false);
        m_importButton->setEnabled(false);
        m_comboBox->clear();
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
            m_exportButton->setEnabled(true);
            m_importButton->setEnabled(true);
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

    if (!m_comboBox || index < 0 || index >= m_comboBox->count())
        return;
    TextEditor::ICodeStylePreferences *delegate =
            m_comboBox->itemData(index).value<TextEditor::ICodeStylePreferences *>();

    const bool wasBlocked = blockSignals(true);
    m_codeStyle->setCurrentDelegate(delegate);
    blockSignals(wasBlocked);
}

void CodeStyleSelectorWidget::slotCurrentDelegateChanged(TextEditor::ICodeStylePreferences *delegate)
{
    m_ignoreGuiSignals = true;
    if (m_comboBox) {
        m_comboBox->setCurrentIndex(m_comboBox->findData(QVariant::fromValue(delegate)));
        m_comboBox->setToolTip(m_comboBox->currentText());
    }
    m_ignoreGuiSignals = false;

    const bool enableEdit = delegate && !delegate->isReadOnly() && !delegate->currentDelegate();
    m_editButton->setEnabled(enableEdit);
    m_removeButton->setEnabled(enableEdit);
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
    copy->setDisplayName(newName);
    if (copy)
        m_codeStyle->setCurrentDelegate(copy);
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
        codeStyle->setTabSettings(dialogCodeStyle->tabSettings());
        codeStyle->setValue(dialogCodeStyle->value());
        codeStyle->setDisplayName(dialog.displayName());
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
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Import Code Style"), QString::null,
                             tr("Code styles (*.xml);;All files (*)"));
    if (!fileName.isEmpty()) {
        CodeStylePool *codeStylePool = m_codeStyle->delegatingPool();
        ICodeStylePreferences *importedStyle = codeStylePool->importCodeStyle(fileName);
        if (importedStyle)
            m_codeStyle->setCurrentDelegate(importedStyle);
        else
            QMessageBox::warning(this, tr("Import Code Style"),
                                 tr("Cannot import code style"));
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
        codeStylePool->exportCodeStyle(fileName, currentPreferences);
    }
}

void CodeStyleSelectorWidget::slotCodeStyleAdded(ICodeStylePreferences *codeStylePreferences)
{
    if (codeStylePreferences == m_codeStyle
            || codeStylePreferences->id() == m_codeStyle->id())
        return;

    const QVariant data = QVariant::fromValue(codeStylePreferences);
    const QString name = displayName(codeStylePreferences);
    m_comboBox->addItem(name, data);
    m_comboBox->setItemData(m_comboBox->count() - 1, name, Qt::ToolTipRole);
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
    m_comboBox->removeItem(m_comboBox->findData(QVariant::fromValue(codeStylePreferences)));
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

    m_comboBox->setToolTip(m_comboBox->currentText());
}

void CodeStyleSelectorWidget::updateName(ICodeStylePreferences *codeStyle)
{
    const int idx = m_comboBox->findData(QVariant::fromValue(codeStyle));
    if (idx < 0)
        return;

    const QString name = displayName(codeStyle);
    m_comboBox->setItemText(idx, name);
    m_comboBox->setItemData(idx, name, Qt::ToolTipRole);
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
