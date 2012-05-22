/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "docsettingspage.h"
#include "helpconstants.h"

#include <coreplugin/helpmanager.h>

#include <QCoreApplication>

#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>

using namespace Help::Internal;

DocSettingsPage::DocSettingsPage()
{
    setId(QLatin1String("B.Documentation"));
    setDisplayName(tr("Documentation"));
    setCategory(QLatin1String(Help::Constants::HELP_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("Help", Help::Constants::HELP_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Help::Constants::HELP_CATEGORY_ICON));
}

QWidget *DocSettingsPage::createPage(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);
    m_ui.setupUi(widget);

    connect(m_ui.addButton, SIGNAL(clicked()), this, SLOT(addDocumentation()));
    connect(m_ui.removeButton, SIGNAL(clicked()), this, SLOT(removeDocumentation()));

    m_ui.docsListWidget->installEventFilter(this);

    Core::HelpManager *manager = Core::HelpManager::instance();
    const QStringList &nameSpaces = manager->registeredNamespaces();
    foreach (const QString &nameSpace, nameSpaces)
        addItem(nameSpace, manager->fileFromNamespace(nameSpace));

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

    Core::HelpManager *manager = Core::HelpManager::instance();
    const QStringList &nameSpaces = manager->registeredNamespaces();

    foreach (const QString &file, files) {
        const QString &nameSpace = manager->namespaceFromFile(file);
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
    Core::HelpManager *manager = Core::HelpManager::instance();

    manager->unregisterDocumentation(m_filesToUnregister.keys());
    manager->registerDocumentation(m_filesToRegister.values());

    m_filesToRegister.clear();
    m_filesToUnregister.clear();
}

bool DocSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
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
    Core::HelpManager *manager = Core::HelpManager::instance();
    foreach (QListWidgetItem* item, items) {
        const QString &nameSpace = item->text();
        const QString &docPath = manager->fileFromNamespace(nameSpace);

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
