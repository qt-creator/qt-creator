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
#include "helpmanager.h"
#include "ui_docsettingspage.h"

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

class DocModel : public QAbstractListModel
{
public:
    using DocEntries = QVector<DocEntry>;

    DocModel() = default;
    void setEntries(const DocEntries &e) { m_docEntries = e; }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_docEntries.size(); }
    QVariant data(const QModelIndex &index, int role) const override;

    void insertEntry(const DocEntry &e);
    void removeAt(int row);

    const DocEntry &entryAt(int row) const { return m_docEntries.at(row); }

private:
    DocEntries m_docEntries;
};

class DocSettingsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Help::DocSettingsPageWidget)

public:
    DocSettingsPageWidget();

private:
    void apply() final;

    void addDocumentation();

    bool eventFilter(QObject *object, QEvent *event) final;
    void removeDocumentation(const QList<QModelIndex> &items);

    QList<QModelIndex> currentSelection() const;

    Ui::DocSettingsPage m_ui;

    QString m_recentDialogPath;

    using NameSpaceToPathHash = QMultiHash<QString, QString>;
    NameSpaceToPathHash m_filesToRegister;
    QHash<QString, bool> m_filesToRegisterUserManaged;
    NameSpaceToPathHash m_filesToUnregister;

    QSortFilterProxyModel m_proxyModel;
    DocModel m_model;
};

static DocEntry createEntry(const QString &nameSpace, const QString &fileName, bool userManaged)
{
    DocEntry result;
    result.name = userManaged ? nameSpace
                              : DocSettingsPageWidget::tr("%1 (auto-detected)").arg(nameSpace);
    result.fileName = fileName;
    result.nameSpace = nameSpace;
    return result;
}

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

using namespace Help::Internal;

DocSettingsPageWidget::DocSettingsPageWidget()
{
    m_ui.setupUi(this);

    const QStringList nameSpaces = HelpManager::registeredNamespaces();
    const QSet<QString> userDocumentationPaths = HelpManager::userDocumentationPaths();

    DocModel::DocEntries entries;
    entries.reserve(nameSpaces.size());
    for (const QString &nameSpace : nameSpaces) {
        const QString filePath = HelpManager::fileFromNamespace(nameSpace);
        bool user = userDocumentationPaths.contains(filePath);
        entries.append(createEntry(nameSpace, filePath, user));
        m_filesToRegister.insert(nameSpace, filePath);
        m_filesToRegisterUserManaged.insert(nameSpace, user);
    }
    std::stable_sort(entries.begin(), entries.end());
    m_model.setEntries(entries);

    m_proxyModel.setSourceModel(&m_model);
    m_ui.docsListView->setModel(&m_proxyModel);
    m_ui.filterLineEdit->setFiltering(true);
    connect(m_ui.filterLineEdit,
            &QLineEdit::textChanged,
            &m_proxyModel,
            &QSortFilterProxyModel::setFilterFixedString);

    connect(m_ui.addButton,
            &QAbstractButton::clicked,
            this,
            &DocSettingsPageWidget::addDocumentation);
    connect(m_ui.removeButton, &QAbstractButton::clicked, this, [this]() {
        removeDocumentation(currentSelection());
    });

    m_ui.docsListView->installEventFilter(this);
}

void DocSettingsPageWidget::addDocumentation()
{
    const QStringList &files =
        QFileDialog::getOpenFileNames(m_ui.addButton->parentWidget(),
        tr("Add Documentation"), m_recentDialogPath, tr("Qt Help Files (*.qch)"));

    if (files.isEmpty())
        return;
    m_recentDialogPath = QFileInfo(files.first()).canonicalPath();

    NameSpaceToPathHash docsUnableToRegister;
    for (const QString &file : files) {
        const QString filePath = QDir::cleanPath(file);
        const QString &nameSpace = HelpManager::namespaceFromFile(filePath);
        if (nameSpace.isEmpty()) {
            docsUnableToRegister.insert("UnknownNamespace", QDir::toNativeSeparators(filePath));
            continue;
        }

        if (m_filesToRegister.contains(nameSpace)) {
            docsUnableToRegister.insert(nameSpace, QDir::toNativeSeparators(filePath));
            continue;
        }

        m_model.insertEntry(createEntry(nameSpace, file, true /* user managed */));

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
            QSet<QString> values = Utils::toSet(m_filesToUnregister.values(nameSpace));
            values.remove(filePath);
            m_filesToUnregister.remove(nameSpace);
            foreach (const QString &value, values)
                m_filesToUnregister.insert(nameSpace, value);
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
        QMessageBox::information(m_ui.addButton->parentWidget(), tr("Registration Failed"),
            tr("Unable to register documentation.") + formatedFail, QMessageBox::Ok);
    }
}

void DocSettingsPageWidget::apply()
{
    HelpManager::unregisterNamespaces(m_filesToUnregister.keys());
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

bool DocSettingsPageWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object != m_ui.docsListView)
        return IOptionsPageWidget::eventFilter(object, event);

    if (event->type() == QEvent::KeyPress) {
        auto ke = static_cast<const QKeyEvent*>(event);
        switch (ke->key()) {
            case Qt::Key_Backspace:
            case Qt::Key_Delete:
                removeDocumentation(currentSelection());
            break;
            default: break;
        }
    }

    return IOptionsPageWidget::eventFilter(object, event);
}

void DocSettingsPageWidget::removeDocumentation(const QList<QModelIndex> &items)
{
    if (items.isEmpty())
        return;

    QList<QModelIndex> itemsByDecreasingRow = items;
    Utils::sort(itemsByDecreasingRow, [](const QModelIndex &i1, const QModelIndex &i2) {
        return i1.row() > i2.row();
    });
    for (const QModelIndex &item : qAsConst(itemsByDecreasingRow)) {
        const int row = item.row();
        const QString nameSpace = m_model.entryAt(row).nameSpace;

        m_filesToRegister.remove(nameSpace);
        m_filesToRegisterUserManaged.remove(nameSpace);
        m_filesToUnregister.insert(nameSpace, QDir::cleanPath(HelpManager::fileFromNamespace(nameSpace)));

        m_model.removeAt(row);
    }

    const int newlySelectedRow = qMax(itemsByDecreasingRow.last().row() - 1, 0);
    const QModelIndex index = m_proxyModel.mapFromSource(m_model.index(newlySelectedRow));
    m_ui.docsListView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
}

QList<QModelIndex> DocSettingsPageWidget::currentSelection() const
{
    return Utils::transform(m_ui.docsListView->selectionModel()->selectedRows(),
            [this](const QModelIndex &index) { return m_proxyModel.mapToSource(index); });
}

DocSettingsPage::DocSettingsPage()
{
    setId("B.Documentation");
    setDisplayName(DocSettingsPageWidget::tr("Documentation"));
    setCategory(Help::Constants::HELP_CATEGORY);
    setWidgetCreator([] { return new DocSettingsPageWidget; });
}

} // namespace Help
