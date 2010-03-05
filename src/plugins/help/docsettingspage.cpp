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

#include <QtCore/QCoreApplication>

#include <QtGui/QFileDialog>
#include <QtGui/QKeyEvent>
#include <QtGui/QMessageBox>

#include <QtHelp/QHelpEngine>

using namespace Help::Internal;

DocSettingsPage::DocSettingsPage(QHelpEngine *helpEngine)
    : m_helpEngine(helpEngine),
      m_registeredDocs(false)
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
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);

    connect(m_ui.addButton, SIGNAL(clicked()),
            this, SLOT(addDocumentation()));
    connect(m_ui.removeButton, SIGNAL(clicked()),
            this, SLOT(removeDocumentation()));

    m_ui.docsListWidget->installEventFilter(this);
    m_ui.docsListWidget->addItems(m_helpEngine->registeredDocumentations());
    m_registeredDocs = false;
    m_removeDocs.clear();
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_ui.groupBox->title();
    return w;
}

void DocSettingsPage::addDocumentation()
{
    QStringList files = QFileDialog::getOpenFileNames(m_ui.addButton->parentWidget(),
                            tr("Add Documentation"),
                            QString(), tr("Qt Help Files (*.qch)"));

    if (files.isEmpty())
        return;

    foreach (const QString &file, files) {
        QString nsName = QHelpEngineCore::namespaceName(file);
        if (nsName.isEmpty()) {
            QMessageBox::warning(m_ui.addButton->parentWidget(),
                                 tr("Add Documentation"),
                                 tr("The file %1 is not a valid Qt Help file!")
                                 .arg(file));
            continue;
        }
        m_helpEngine->registerDocumentation(file);
        m_ui.docsListWidget->addItem(nsName);
    }
    m_registeredDocs = true;
    emit documentationAdded();
}

void DocSettingsPage::removeDocumentation()
{
    removeDocumentation(m_ui.docsListWidget->selectedItems());
}

void DocSettingsPage::apply()
{
    emit dialogAccepted();
}

bool DocSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

bool DocSettingsPage::applyChanges()
{
    QStringList::const_iterator it = m_removeDocs.constBegin();
    while (it != m_removeDocs.constEnd()) {
        if (!m_helpEngine->unregisterDocumentation((*it))) {
            QMessageBox::warning(m_ui.addButton->parentWidget(),
                tr("Documentation"),
                tr("Cannot unregister documentation file %1!")
                .arg((*it)));
        }
        ++it;
    }

    bool success = m_registeredDocs || m_removeDocs.count();

    m_removeDocs.clear();
    m_registeredDocs = false;

    return success;
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
    foreach (QListWidgetItem* item, items) {
        m_removeDocs.append(item->text());
        row = m_ui.docsListWidget->row(item);
        delete m_ui.docsListWidget->takeItem(row);
    }

    m_ui.docsListWidget->setCurrentRow(qMax(row - 1, 0),
        QItemSelectionModel::ClearAndSelect);
}
