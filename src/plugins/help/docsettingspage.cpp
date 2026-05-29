// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "docsettingspage.h"

#include "helpmanager.h"
#include "helptr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QVariant>
#include <QVector>

#include <algorithm>

using namespace Utils;

namespace Help::Internal {

class DocEntry final
{
public:
    QString name;
    QString fileName;
    QString nameSpace;

    friend bool operator<(const DocEntry &d1, const DocEntry &d2) { return d1.name < d2.name; }
};

class DocModel final : public QAbstractListModel
{
public:
    using DocEntries = QVector<DocEntry>;

    DocModel() = default;
    void setEntries(const DocEntries &e) { m_docEntries = e; }

    int rowCount(const QModelIndex & = QModelIndex()) const final { return m_docEntries.size(); }
    QVariant data(const QModelIndex &index, int role) const final;

    void insertEntry(const DocEntry &e);
    void removeAt(int row);

    const DocEntry &entryAt(int row) const { return m_docEntries.at(row); }

private:
    DocEntries m_docEntries;
};

class DocSettingsPageWidget final : public Core::IOptionsPageWidget
{
public:
    DocSettingsPageWidget();

private:
    void apply() final;

    void addDocumentation();

    bool eventFilter(QObject *object, QEvent *event) final;
    void removeDocumentation(const QList<QModelIndex> &items);

    QList<QModelIndex> currentSelection() const;

    FilePath m_recentDialogPath;

    using NameSpaceToPathHash = QMultiHash<QString, QString>;
    NameSpaceToPathHash m_filesToRegister;
    QHash<QString, bool> m_filesToRegisterUserManaged;
    NameSpaceToPathHash m_filesToUnregister;

    QSortFilterProxyModel m_proxyModel;
    DocModel m_model;

    QListView m_docsListView;
    QPushButton m_addButton;
    QPushButton m_removeButton;
    FancyLineEdit m_filterLineEdit;
};

static DocEntry createEntry(const QString &nameSpace, const QString &fileName, bool userManaged)
{
    DocEntry result;
    result.name = userManaged ? nameSpace : Tr::tr("%1 (auto-detected)").arg(nameSpace);
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

DocSettingsPageWidget::DocSettingsPageWidget()
{
    setToolTip(Tr::tr("Add and remove compressed help files, .qch."));

    const QStringList nameSpaces = HelpManager::registeredNamespaces();
    const QSet<FilePath> userDocumentationPaths = HelpManager::userDocumentationPaths();

    DocModel::DocEntries entries;
    entries.reserve(nameSpaces.size());
    for (const QString &nameSpace : nameSpaces) {
        const FilePath filePath = HelpManager::fileFromNamespace(nameSpace);
        bool user = userDocumentationPaths.contains(filePath);
        entries.append(createEntry(nameSpace, filePath.path(), user));
        m_filesToRegister.insert(nameSpace, filePath.path());
        m_filesToRegisterUserManaged.insert(nameSpace, user);
    }
    std::stable_sort(entries.begin(), entries.end());
    m_model.setEntries(entries);

    m_proxyModel.setSourceModel(&m_model);

    m_docsListView.setModel(&m_proxyModel);
    m_docsListView.installEventFilter(this);

    m_docsListView.setObjectName("docsListView");
    m_docsListView.setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_docsListView.setUniformItemSizes(true);

    m_filterLineEdit.setFiltering(true);

    m_addButton.setObjectName("addButton");
    m_addButton.setText(Tr::tr("Add..."));

    m_removeButton.setObjectName("removeButton");
    m_removeButton.setText(Tr::tr("Remove"));

    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Registered Documentation")),
            Row {
                Column {
                    m_filterLineEdit,
                    m_docsListView
                },
                Column {
                    m_addButton,
                    m_removeButton,
                    st
                }
            }
        }
    }.attachTo(this);

    connect(&m_filterLineEdit, &QLineEdit::textChanged,
            &m_proxyModel, &QSortFilterProxyModel::setFilterFixedString);

    connect(&m_addButton, &QAbstractButton::clicked,
            this, &DocSettingsPageWidget::addDocumentation);

    connect(&m_removeButton, &QAbstractButton::clicked, this, [this] {
        removeDocumentation(currentSelection());
    });

    installMarkSettingsDirtyTrigger(&m_model);
}

void DocSettingsPageWidget::addDocumentation()
{
    const FilePaths files = FileUtils::getOpenFilePaths(Tr::tr("Add Documentation"),
                                                        m_recentDialogPath,
                                                        Tr::tr("Qt Help Files (*.qch)"));

    if (files.isEmpty())
        return;
    m_recentDialogPath = files.first().canonicalPath();

    NameSpaceToPathHash docsUnableToRegister;
    for (const FilePath &file : files) {
        const QString filePath = file.cleanPath().toUrlishString();
        const QString &nameSpace = HelpManager::namespaceFromFile(filePath);
        if (nameSpace.isEmpty()) {
            docsUnableToRegister.insert("UnknownNamespace", file.toUserOutput());
            continue;
        }

        if (m_filesToRegister.contains(nameSpace)) {
            docsUnableToRegister.insert(nameSpace, file.toUserOutput());
            continue;
        }

        m_model.insertEntry(createEntry(nameSpace, file.toUrlishString(), true /* user managed */));

        m_filesToRegister.insert(nameSpace, filePath);
        m_filesToRegisterUserManaged.insert(nameSpace, true/*user managed*/);
        markSettingsDirty();

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
            for (const QString &value : std::as_const(values))
                m_filesToUnregister.insert(nameSpace, value);
        }
    }

    QString formatedFail;
    if (docsUnableToRegister.contains("UnknownNamespace")) {
        formatedFail += QString::fromLatin1("<ul><li><b>%1</b>")
                            .arg(Tr::tr("Invalid documentation file:"));
        const QStringList values = docsUnableToRegister.values("UnknownNamespace");
        for (const QString &value : values)
            formatedFail += QString::fromLatin1("<ul><li>%2</li></ul>").arg(value);
        formatedFail += "</li></ul>";
        docsUnableToRegister.remove("UnknownNamespace");
    }

    if (!docsUnableToRegister.isEmpty()) {
        formatedFail += QString::fromLatin1("<ul><li><b>%1</b>")
                            .arg(Tr::tr("Namespace already registered:"));
        const NameSpaceToPathHash::ConstIterator cend = docsUnableToRegister.constEnd();
        for (NameSpaceToPathHash::ConstIterator it = docsUnableToRegister.constBegin(); it != cend; ++it) {
            formatedFail += QString::fromLatin1("<ul><li>%1 - %2</li></ul>").arg(it.key(), it.value());
        }
        formatedFail += "</li></ul>";
    }

    if (!formatedFail.isEmpty()) {
        QMessageBox::information(Core::ICore::dialogParent(),
                                 Tr::tr("Registration Failed"),
                                 Tr::tr("Unable to register documentation.") + formatedFail,
                                 QMessageBox::Ok);
    }
}

void DocSettingsPageWidget::apply()
{
    HelpManager::instance()->unregisterDocumentation(
        transform(m_filesToUnregister.values(), &FilePath::fromString));
    FilePaths files;
    auto it = m_filesToRegisterUserManaged.constBegin();
    while (it != m_filesToRegisterUserManaged.constEnd()) {
        if (it.value()/*userManaged*/)
            files << FilePath::fromString(m_filesToRegister.value(it.key()));
        ++it;
    }
    HelpManager::registerUserDocumentation(files);

    m_filesToUnregister.clear();
}

bool DocSettingsPageWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object != &m_docsListView)
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

    const QList<QModelIndex> itemsByDecreasingRow = Utils::sorted(items,
            [](const QModelIndex &i1, const QModelIndex &i2) { return i1.row() > i2.row(); });
    for (const QModelIndex &item : itemsByDecreasingRow) {
        const int row = item.row();
        const QString nameSpace = m_model.entryAt(row).nameSpace;

        m_filesToRegister.remove(nameSpace);
        m_filesToRegisterUserManaged.remove(nameSpace);
        m_filesToUnregister
            .insert(nameSpace, HelpManager::fileFromNamespace(nameSpace).cleanPath().path());

        m_model.removeAt(row);
    }

    const int newlySelectedRow = qMax(itemsByDecreasingRow.last().row() - 1, 0);
    const QModelIndex index = m_proxyModel.mapFromSource(m_model.index(newlySelectedRow));
    m_docsListView.selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
}

QList<QModelIndex> DocSettingsPageWidget::currentSelection() const
{
    return Utils::transform(m_docsListView.selectionModel()->selectedRows(),
                            [this](const QModelIndex &index) {
                                return m_proxyModel.mapToSource(index);
                            });
}

class DocSettingsPage final : public Core::IOptionsPage
{
public:
    DocSettingsPage()
    {
        setId("B.Documentation");
        setDisplayName(Tr::tr("Documentation"));
        setCategory(Core::Constants::HELP_CATEGORY);
        setWidgetCreator([] { return new DocSettingsPageWidget; });
    }
};

void setupDocSettingsPage()
{
    static DocSettingsPage theDocSettingsPage;
}

} // namespace Help::Internal
