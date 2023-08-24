// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscomponentnamedialog.h"
#include "qmljseditortr.h"

#include <utils/classnamevalidatinglineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>

using namespace QmlJSEditor::Internal;

ComponentNameDialog::ComponentNameDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(Tr::tr("Move Component into Separate File"));
    m_componentNameEdit = new Utils::ClassNameValidatingLineEdit;
    m_componentNameEdit->setObjectName("componentNameEdit");
    m_componentNameEdit->setPlaceholderText(Tr::tr("Component Name"));
    m_messageLabel = new QLabel;
    m_pathEdit = new Utils::PathChooser;
    m_label = new QLabel;
    m_listWidget = new QListWidget;
    m_plainTextEdit = new QPlainTextEdit;
    m_checkBox = new QCheckBox(Tr::tr("ui.qml file"));
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    using namespace Layouting;
    Column {
        Form {
            Tr::tr("Component name:"), m_componentNameEdit, br,
            empty, m_messageLabel, br,
            Tr::tr("Path:"), m_pathEdit, br,
        },
        m_label,
        Row { m_listWidget, m_plainTextEdit },
        Row { m_checkBox, m_buttonBox },
    }.attachTo(this);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ComponentNameDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &ComponentNameDialog::reject);
    connect(m_pathEdit, &Utils::PathChooser::rawPathChanged, this, &ComponentNameDialog::validate);
    connect(m_pathEdit, &Utils::PathChooser::validChanged, this, &ComponentNameDialog::validate);
    connect(m_componentNameEdit, &QLineEdit::textChanged, this, &ComponentNameDialog::validate);
}

bool ComponentNameDialog::go(QString *proposedName,
                             QString *proposedPath,
                             QString *proposedSuffix,
                             const QStringList &properties,
                             const QStringList &sourcePreview,
                             const QString &oldFileName,
                             QStringList *result,
                             QWidget *parent)
{
    Q_ASSERT(proposedName);
    Q_ASSERT(proposedPath);

    const bool isUiFile = QFileInfo(oldFileName).completeSuffix() == "ui.qml";

    ComponentNameDialog d(parent);
    d.m_componentNameEdit->setNamespacesEnabled(false);
    d.m_componentNameEdit->setLowerCaseFileName(false);
    d.m_componentNameEdit->setForceFirstCapitalLetter(true);
    if (proposedName->isEmpty())
        *proposedName = QLatin1String("MyComponent");
    d.m_componentNameEdit->setText(*proposedName);
    d.m_pathEdit->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    d.m_pathEdit->setHistoryCompleter("QmlJs.Component.History");
    d.m_pathEdit->setPath(*proposedPath);
    d.m_label->setText(Tr::tr("Property assignments for %1:").arg(oldFileName));
    d.m_checkBox->setChecked(isUiFile);
    d.m_checkBox->setVisible(isUiFile);
    d.m_sourcePreview = sourcePreview;

    d.setProperties(properties);

    d.generateCodePreview();

    d.connect(d.m_listWidget, &QListWidget::itemChanged, &d, &ComponentNameDialog::generateCodePreview);
    d.connect(d.m_componentNameEdit, &QLineEdit::textChanged, &d, &ComponentNameDialog::generateCodePreview);

    if (QDialog::Accepted == d.exec()) {
        *proposedName = d.m_componentNameEdit->text();
        *proposedPath = d.m_pathEdit->filePath().toString();

        if (d.m_checkBox->isChecked())
            *proposedSuffix = "ui.qml";
        else
            *proposedSuffix = "qml";

        if (result)
            *result = d.propertiesToKeep();
        return true;
    }

    return false;
}

void ComponentNameDialog::setProperties(const QStringList &properties)
{
    m_listWidget->addItems(properties);

    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem *item = m_listWidget->item(i);
        item->setFlags(Qt::ItemIsUserCheckable | Qt:: ItemIsEnabled);
        if (item->text() == QLatin1String("x")
                || item->text() == QLatin1String("y"))
            m_listWidget->item(i)->setCheckState(Qt::Checked);
        else
            m_listWidget->item(i)->setCheckState(Qt::Unchecked);
    }
}

QStringList ComponentNameDialog::propertiesToKeep() const
{
    QStringList result;
    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem *item = m_listWidget->item(i);

        if (item->checkState() == Qt::Checked)
            result.append(item->text());
    }

    return result;
}

void ComponentNameDialog::generateCodePreview()
{
     const QString componentName = m_componentNameEdit->text();

     m_plainTextEdit->clear();
     m_plainTextEdit->appendPlainText(componentName + QLatin1String(" {"));
     if (!m_sourcePreview.first().isEmpty())
        m_plainTextEdit->appendPlainText(m_sourcePreview.first());

     for (int i = 0; i < m_listWidget->count(); ++i) {
         QListWidgetItem *item = m_listWidget->item(i);

         if (item->checkState() == Qt::Checked)
             m_plainTextEdit->appendPlainText(m_sourcePreview.at(i + 1));
     }

     m_plainTextEdit->appendPlainText(QLatin1String("}"));
}

void ComponentNameDialog::validate()
{
    const QString message = isValid();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(message.isEmpty());
    m_messageLabel->setText(message);
}

QString ComponentNameDialog::isValid() const
{
    if (!m_componentNameEdit->isValid())
        return m_componentNameEdit->errorMessage();

    QString compName = m_componentNameEdit->text();
    if (compName.isEmpty() || !compName[0].isUpper())
        return Tr::tr("Invalid component name.");

    if (!m_pathEdit->isValid())
        return Tr::tr("Invalid path.");

    if (m_pathEdit->filePath().pathAppended(compName + ".qml").exists())
        return Tr::tr("Component already exists.");

    return QString();
}
