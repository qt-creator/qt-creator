/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "docsettingspage.h"
#include "helpconstants.h"
#include "helpmanager.h"

#include <QtCore/QCoreApplication>

#include <QtGui/QFileDialog>
#include <QtGui/QKeyEvent>
#include <QtGui/QMessageBox>

#include <QtHelp/QHelpEngineCore>

using namespace Help::Internal;

DocSettingsPage::DocSettingsPage()
{
}

QString DocSettingsPage::id() const
{
    return QLatin1String("B.Documentation");
}

QString DocSettingsPage::displayName() const
{
    return tr("Documentation");
}

QString DocSettingsPage::category() const
{
    return QLatin1String(Help::Constants::HELP_CATEGORY);
}

QString DocSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Help", Help::Constants::HELP_TR_CATEGORY);
}

QWidget *DocSettingsPage::createPage(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);
    m_ui.setupUi(widget);

    connect(m_ui.addButton, SIGNAL(clicked()), this, SLOT(addDocumentation()));
    connect(m_ui.removeButton, SIGNAL(clicked()), this, SLOT(removeDocumentation()));

    m_ui.docsListWidget->installEventFilter(this);

    QHelpEngineCore *engine = &HelpManager::helpEngineCore();
    const QStringList &nameSpaces = engine->registeredDocumentations();
    foreach (const QString &nameSpace, nameSpaces)
        addItem(nameSpace, engine->documentationFileName(nameSpace));

    m_filesToRegister.clear();
    m_filesToUnregister.clear();

    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_ui.groupBox->title();
    return widget;
}

void DocSettingsPage::addDocumentation()
{
    const QStringList &files =
        QFileDialog::getOpenFileNames(m_ui.addButton->parentWidget(),
        tr("Add Documentation"), m_recentDialogPath, tr("Qt Help Files (*.qch)"));

    if (files.isEmpty())
        return;
    m_recentDialogPath = QFileInfo(files.first()).canonicalPath();

    const QHelpEngineCore &engine = HelpManager::helpEngineCore();
    const QStringList &nameSpaces = engine.registeredDocumentations();

    foreach (const QString &file, files) {
        const QString &nameSpace = engine.namespaceName(file);
        if (nameSpace.isEmpty())
            continue;

        if (m_filesToUnregister.value(nameSpace) != QDir::cleanPath(file)) {
            if (!m_filesToRegister.contains(nameSpace) && !nameSpaces.contains(nameSpace)) {
                addItem(nameSpace, file);
                m_filesToRegister.insert(nameSpace, QDir::cleanPath(file));
            }
        } else {
            addItem(nameSpace, file);
            m_filesToUnregister.remove(nameSpace);
        }
    }
}

void DocSettingsPage::removeDocumentation()
{
    removeDocumentation(m_ui.docsListWidget->selectedItems());
}

void DocSettingsPage::apply()
{
    emit dialogAccepted();
    emit documentationChanged();

    m_filesToRegister.clear();
    m_filesToUnregister.clear();
}

bool DocSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

QStringList DocSettingsPage::docsToRegister() const
{
    return m_filesToRegister.values();
}

QStringList DocSettingsPage::docsToUnregister() const
{
    return m_filesToUnregister.keys();
}

bool DocSettingsPage::eventFilter(QObject *object, QEvent *event)
{
    if (object != m_ui.docsListWidget)
        return Core::IOptionsPage::eventFilter(object, event);

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        switch (ke->key()) {
            case Qt::Key_Delete:
                removeDocumentation(m_ui.docsListWidget->selectedItems());
            break;
            default: break;
        }
    }

    return Core::IOptionsPage::eventFilter(object, event);
}

void DocSettingsPage::removeDocumentation(const QList<QListWidgetItem*> items)
{
    if (items.isEmpty())
        return;

    int row = 0;
    QHelpEngineCore *engine = &HelpManager::helpEngineCore();
    foreach (QListWidgetItem* item, items) {
        const QString &nameSpace = item->text();
        const QString &docPath = engine->documentationFileName(nameSpace);

        if (m_filesToRegister.value(nameSpace) != docPath) {
            if (!m_filesToUnregister.contains(nameSpace))
                m_filesToUnregister.insert(nameSpace, docPath);
        } else {
            m_filesToRegister.remove(nameSpace);
        }

        row = m_ui.docsListWidget->row(item);
        delete m_ui.docsListWidget->takeItem(row);
    }

    m_ui.docsListWidget->setCurrentRow(qMax(row - 1, 0),
        QItemSelectionModel::ClearAndSelect);
}

void DocSettingsPage::addItem(const QString &nameSpace, const QString &fileName)
{
    QListWidgetItem* item = new QListWidgetItem(nameSpace);
    item->setToolTip(fileName);
    m_ui.docsListWidget->addItem(item);
}
