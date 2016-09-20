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

#include "docsettingspage.h"
#include "helpconstants.h"

#include <coreplugin/helpmanager.h>
#include <utils/algorithm.h>

#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QDir>
#include <QSortFilterProxyModel>
#include <QVariant>
#include <QVector>

#include <algorithm>

using namespace Core;

namespace Help {
namespace Internal {

class DocEntry
{
public:
    QString name;
    QString fileName;
    QString nameSpace;
};

static bool operator<(const DocEntry &d1, const DocEntry &d2)
{ return d1.name < d2.name; }

static DocEntry createEntry(const QString &nameSpace, const QString &fileName, bool userManaged)
{
    DocEntry result;
    result.name = userManaged ? nameSpace : DocSettingsPage::tr("%1 (auto-detected)").arg(nameSpace);
    result.fileName = fileName;
    result.nameSpace = nameSpace;
    return result;
}

class DocModel : public QAbstractListModel {
public:
    typedef QVector<DocEntry> DocEntries;

    explicit DocModel(const DocEntries &e = DocEntries(), QObject *parent = nullptr)
        : QAbstractListModel(parent), m_docEntries(e) {}

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_docEntries.size(); }
    QVariant data(const QModelIndex &index, int role) const override;

    void insertEntry(const DocEntry &e);
    void removeAt(int row);

    const DocEntry &entryAt(int row) const { return m_docEntries.at(row); }

private:
    DocEntries m_docEntries;
};

QVariant DocModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    const int row = index.row();
    if (index.isValid() && row < m_docEntries.size()) {
        switch (role) {
        case Qt::DisplayRole:
            result = QVariant(m_docEntries.at(row).name);
            break;
        case Qt::ToolTipRole:
            result = QVariant(QDir::toNativeSeparators(m_docEntries.at(row).fileName));
            break;
        case Qt::UserRole:
            result = QVariant(m_docEntries.at(row).nameSpace);
            break;
        default:
            break;
        }
    }
    return result;
}

void DocModel::insertEntry(const DocEntry &e)
{
    const auto it = std::lower_bound(m_docEntries.begin(), m_docEntries.end(), e);
    const int index = int(it - m_docEntries.begin());
    beginInsertRows(QModelIndex(), index, index);
    m_docEntries.insert(it, e);
    endInsertRows();
}

void DocModel::removeAt(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_docEntries.removeAt(row);
    endRemoveRows();
}

} // namespace Internal
} // namespace Help

using namespace Help::Internal;

DocSettingsPage::DocSettingsPage()
{
    setId("B.Documentation");
    setDisplayName(tr("Documentation"));
    setCategory(Help::Constants::HELP_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Help", Help::Constants::HELP_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(Help::Constants::HELP_CATEGORY_ICON));
}

QWidget *DocSettingsPage::widget()
{
    if (!m_widget) {
        const QStringList nameSpaces = HelpManager::registeredNamespaces();
        const QSet<QString> userDocumentationPaths = HelpManager::userDocumentationPaths();
        DocModel::DocEntries entries;
        entries.reserve(nameSpaces.size());
        foreach (const QString &nameSpace, nameSpaces) {
            const QString filePath = HelpManager::fileFromNamespace(nameSpace);
            bool user = userDocumentationPaths.contains(filePath);
            entries.append(createEntry(nameSpace, filePath, user));
            m_filesToRegister.insert(nameSpace, filePath);
            m_filesToRegisterUserManaged.insert(nameSpace, user);
        }
        std::stable_sort(entries.begin(), entries.end());

        m_filesToUnregister.clear();

        m_widget = new QWidget;
        m_ui.setupUi(m_widget);
        m_model = new DocModel(entries, m_ui.docsListView);
        m_proxyModel = new QSortFilterProxyModel(m_ui.docsListView);
        m_proxyModel->setSourceModel(m_model);
        m_ui.docsListView->setModel(m_proxyModel);
        m_ui.filterLineEdit->setFiltering(true);
        connect(m_ui.filterLineEdit, &QLineEdit::textChanged,
                m_proxyModel, &QSortFilterProxyModel::setFilterFixedString);

        connect(m_ui.addButton, &QAbstractButton::clicked, this, &DocSettingsPage::addDocumentation);
        connect(m_ui.removeButton, &QAbstractButton::clicked, this,
                [this] () { removeDocumentation(currentSelection()); });

        m_ui.docsListView->installEventFilter(this);
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
            docsUnableToRegister.insertMulti("UnknownNamespace", QDir::toNativeSeparators(filePath));
            continue;
        }

        if (m_filesToRegister.contains(nameSpace)) {
            docsUnableToRegister.insert(nameSpace, QDir::toNativeSeparators(filePath));
            continue;
        }

        m_model->insertEntry(createEntry(nameSpace, file, true /* user managed */));

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
    if (docsUnableToRegister.contains("UnknownNamespace")) {
        formatedFail += QString::fromLatin1("<ul><li><b>%1</b>").arg(tr("Invalid documentation file:"));
        foreach (const QString &value, docsUnableToRegister.values("UnknownNamespace"))
            formatedFail += QString::fromLatin1("<ul><li>%2</li></ul>").arg(value);
        formatedFail += "</li></ul>";
        docsUnableToRegister.remove("UnknownNamespace");
    }

    if (!docsUnableToRegister.isEmpty()) {
        formatedFail += QString::fromLatin1("<ul><li><b>%1</b>").arg(tr("Namespace already registered:"));
        const NameSpaceToPathHash::ConstIterator cend = docsUnableToRegister.constEnd();
        for (NameSpaceToPathHash::ConstIterator it = docsUnableToRegister.constBegin(); it != cend; ++it) {
            formatedFail += QString::fromLatin1("<ul><li>%1 - %2</li></ul>").arg(it.key(), it.value());
        }
        formatedFail += "</li></ul>";
    }

    if (!formatedFail.isEmpty()) {
        QMessageBox::information(m_ui.addButton->parentWidget(), tr("Registration failed"),
            tr("Unable to register documentation.") + formatedFail, QMessageBox::Ok);
    }
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
    if (object != m_ui.docsListView)
        return IOptionsPage::eventFilter(object, event);

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        switch (ke->key()) {
            case Qt::Key_Delete:
                removeDocumentation(currentSelection());
            break;
            default: break;
        }
    }

    return IOptionsPage::eventFilter(object, event);
}

void DocSettingsPage::removeDocumentation(const QList<QModelIndex> &items)
{
    if (items.isEmpty())
        return;

    QList<QModelIndex> itemsByDecreasingRow = items;
    Utils::sort(itemsByDecreasingRow, [](const QModelIndex &i1, const QModelIndex &i2) {
        return i1.row() > i2.row();
    });
    foreach (const QModelIndex &item, itemsByDecreasingRow) {
        const int row = item.row();
        const QString nameSpace = m_model->entryAt(row).nameSpace;

        m_filesToRegister.remove(nameSpace);
        m_filesToRegisterUserManaged.remove(nameSpace);
        m_filesToUnregister.insertMulti(nameSpace, QDir::cleanPath(HelpManager::fileFromNamespace(nameSpace)));

        m_model->removeAt(row);
    }

    const int newlySelectedRow = qMax(itemsByDecreasingRow.last().row() - 1, 0);
    const QModelIndex index = m_proxyModel->mapFromSource(m_model->index(newlySelectedRow));
    m_ui.docsListView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
}

QList<QModelIndex> DocSettingsPage::currentSelection() const
{
    QModelIndexList result;
    Q_ASSERT(!m_widget.isNull());
    foreach (const QModelIndex &index, m_ui.docsListView->selectionModel()->selectedRows())
        result.append(m_proxyModel->mapToSource(index));
    return result;
}
