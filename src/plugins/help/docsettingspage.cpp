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

#include "docsettingspage.h"
#include "helpconstants.h"

#include <coreplugin/helpmanager.h>

#include <QCoreApplication>

#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>

using namespace Core;
using namespace Help::Internal;

DocSettingsPage::DocSettingsPage()
{
    setId("B.Documentation");
    setDisplayName(tr("Documentation"));
    setCategory(Help::Constants::HELP_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Help", Help::Constants::HELP_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Help::Constants::HELP_CATEGORY_ICON));
}

QWidget *DocSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;
        m_ui.setupUi(m_widget);

        connect(m_ui.addButton, SIGNAL(clicked()), this, SLOT(addDocumentation()));
        connect(m_ui.removeButton, SIGNAL(clicked()), this, SLOT(removeDocumentation()));

        m_ui.docsListWidget->installEventFilter(this);

        const QStringList nameSpaces = HelpManager::registeredNamespaces();
        const QSet<QString> userDocumentationPaths = HelpManager::userDocumentationPaths();
        foreach (const QString &nameSpace, nameSpaces) {
            const QString filePath = HelpManager::fileFromNamespace(nameSpace);
            bool user = userDocumentationPaths.contains(filePath);
            addItem(nameSpace, filePath, user);
            m_filesToRegister.insert(nameSpace, filePath);
            m_filesToRegisterUserManaged.insert(nameSpace, user);
        }

        m_filesToUnregister.clear();
    }
    return m_widget;
}

void DocSettingsPage::addDocumentation()
{
    const QStringList &files =
        QFileDialog::getOpenFileNames(m_ui.addButton->parentWidget(),
        tr("Add Documentation"), m_recentDialogPath, tr("Qt Help Files (*.qch)"));

    if (files.isEmpty())
        return;
    m_recentDialogPath = QFileInfo(files.first()).canonicalPath();

    NameSpaceToPathHash docsUnableToRegister;
    foreach (const QString &file, files) {
        const QString filePath = QDir::cleanPath(file);
        const QString &nameSpace = HelpManager::namespaceFromFile(filePath);
        if (nameSpace.isEmpty()) {
            docsUnableToRegister.insertMulti(QLatin1String("UnknownNamespace"),
                QDir::toNativeSeparators(filePath));
            continue;
        }

        if (m_filesToRegister.contains(nameSpace)) {
            docsUnableToRegister.insert(nameSpace, QDir::toNativeSeparators(filePath));
            continue;
        }

        addItem(nameSpace, file, true/*user managed*/);
        m_filesToRegister.insert(nameSpace, filePath);
        m_filesToRegisterUserManaged.insert(nameSpace, true/*user managed*/);

        // If the files to unregister contains the namespace, grab a copy of all paths added and try to
        // remove the current file path. Afterwards remove the whole entry and add the clean list back.
        // Possible outcome:
        //      - might not add the entry back at all if we register the same file again
        //      - might add the entry back with paths to other files with the same namespace
        // The reason to do this is, if we remove a file with a given namespace/ path and re-add another
        // file with the same namespace but a different path, we need to unregister the namespace before
        // we can register the new one. Help engine allows just one registered namespace.
        if (m_filesToUnregister.contains(nameSpace)) {
            QSet<QString> values = m_filesToUnregister.values(nameSpace).toSet();
            values.remove(filePath);
            m_filesToUnregister.remove(nameSpace);
            foreach (const QString &value, values)
                m_filesToUnregister.insertMulti(nameSpace, value);
        }
    }

    QString formatedFail;
    if (docsUnableToRegister.contains(QLatin1String("UnknownNamespace"))) {
        formatedFail += QString::fromLatin1("<ul><li><b>%1</b>").arg(tr("Invalid documentation file:"));
        foreach (const QString &value, docsUnableToRegister.values(QLatin1String("UnknownNamespace")))
            formatedFail += QString::fromLatin1("<ul><li>%2</li></ul>").arg(value);
        formatedFail += QLatin1String("</li></ul>");
        docsUnableToRegister.remove(QLatin1String("UnknownNamespace"));
    }

    if (!docsUnableToRegister.isEmpty()) {
        formatedFail += QString::fromLatin1("<ul><li><b>%1</b>").arg(tr("Namespace already registered:"));
        const NameSpaceToPathHash::ConstIterator cend = docsUnableToRegister.constEnd();
        for (NameSpaceToPathHash::ConstIterator it = docsUnableToRegister.constBegin(); it != cend; ++it) {
            formatedFail += QString::fromLatin1("<ul><li>%1 - %2</li></ul>").arg(it.key(), it.value());
        }
        formatedFail += QLatin1String("</li></ul>");
    }

    if (!formatedFail.isEmpty()) {
        QMessageBox::information(m_ui.addButton->parentWidget(), tr("Registration failed"),
            tr("Unable to register documentation.") + formatedFail, QMessageBox::Ok);
    }
}

void DocSettingsPage::removeDocumentation()
{
    removeDocumentation(m_ui.docsListWidget->selectedItems());
}

void DocSettingsPage::apply()
{
    HelpManager::unregisterDocumentation(m_filesToUnregister.keys());
    QStringList files;
    auto it = m_filesToRegisterUserManaged.constBegin();
    while (it != m_filesToRegisterUserManaged.constEnd()) {
        if (it.value()/*userManaged*/)
            files << m_filesToRegister.value(it.key());
        ++it;
    }
    HelpManager::registerUserDocumentation(files);

    m_filesToUnregister.clear();
}

void DocSettingsPage::finish()
{
    delete m_widget;
}

bool DocSettingsPage::eventFilter(QObject *object, QEvent *event)
{
    if (object != m_ui.docsListWidget)
        return IOptionsPage::eventFilter(object, event);

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        switch (ke->key()) {
            case Qt::Key_Delete:
                removeDocumentation(m_ui.docsListWidget->selectedItems());
            break;
            default: break;
        }
    }

    return IOptionsPage::eventFilter(object, event);
}

void DocSettingsPage::removeDocumentation(const QList<QListWidgetItem*> &items)
{
    if (items.isEmpty())
        return;

    int row = 0;
    foreach (QListWidgetItem* item, items) {
        const QString nameSpace = item->data(Qt::UserRole).toString();

        m_filesToRegister.remove(nameSpace);
        m_filesToRegisterUserManaged.remove(nameSpace);
        m_filesToUnregister.insertMulti(nameSpace, QDir::cleanPath(HelpManager::fileFromNamespace(nameSpace)));

        row = m_ui.docsListWidget->row(item);
        delete m_ui.docsListWidget->takeItem(row);
    }

    m_ui.docsListWidget->setCurrentRow(qMax(row - 1, 0),
        QItemSelectionModel::ClearAndSelect);
}

void DocSettingsPage::addItem(const QString &nameSpace, const QString &fileName, bool userManaged)
{
    const QString name = userManaged ? nameSpace : tr("%1 (auto-detected)").arg(nameSpace);
    QListWidgetItem* item = new QListWidgetItem(name);
    item->setToolTip(fileName);
    item->setData(Qt::UserRole, nameSpace);
    m_ui.docsListWidget->addItem(item);
}
