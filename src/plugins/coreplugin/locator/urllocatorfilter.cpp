// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "urllocatorfilter.h"

#include "../coreplugintr.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

using namespace Utils;

namespace Core {
namespace Internal {

UrlFilterOptions::UrlFilterOptions(UrlLocatorFilter *filter, QWidget *parent)
    : QDialog(parent)
    , m_filter(filter)
{
    setWindowTitle(ILocatorFilter::msgConfigureDialogTitle());
    resize(600, 400);

    auto nameLabel = new QLabel(Tr::tr("Name:"));
    nameLabel->setVisible(filter->isCustomFilter());

    nameEdit = new QLineEdit;
    nameEdit->setText(filter->displayName());
    nameEdit->selectAll();
    nameEdit->setVisible(filter->isCustomFilter());

    listWidget = new QListWidget;
    listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    listWidget->setToolTip(
        Tr::tr("Add \"%1\" placeholder for the query string.\nDouble-click to edit item."));
    const QStringList remoteUrls = m_filter->remoteUrls();
    for (const QString &url : remoteUrls) {
        auto item = new QListWidgetItem(url);
        listWidget->addItem(item);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    auto add = new QPushButton(Tr::tr("Add"));
    remove = new QPushButton(Tr::tr("Remove"));
    moveUp = new QPushButton(Tr::tr("Move Up"));
    moveDown = new QPushButton(Tr::tr("Move Down"));

    auto prefixLabel = new QLabel;
    prefixLabel->setText(Core::ILocatorFilter::msgPrefixLabel());
    prefixLabel->setToolTip(Core::ILocatorFilter::msgPrefixToolTip());

    shortcutEdit = new QLineEdit;
    shortcutEdit->setText(m_filter->shortcutString());

    includeByDefault = new QCheckBox;
    includeByDefault->setText(Core::ILocatorFilter::msgIncludeByDefault());
    includeByDefault->setToolTip(Core::ILocatorFilter::msgIncludeByDefaultToolTip());
    includeByDefault->setChecked(m_filter->isIncludedByDefault());

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

    using namespace Layouting;

    Column buttons { add, remove, moveUp, moveDown, st };

    Grid {
        nameLabel, nameEdit, br,
        Column { Tr::tr("URLs:"), st }, Row { listWidget, buttons}, br,
        prefixLabel, Row { shortcutEdit, includeByDefault, st }, br,
        Span(2, buttonBox)
    }.attachTo(this);

    connect(add, &QPushButton::clicked, this, &UrlFilterOptions::addNewItem);
    connect(remove, &QPushButton::clicked, this, &UrlFilterOptions::removeItem);
    connect(moveUp, &QPushButton::clicked, this, &UrlFilterOptions::moveItemUp);
    connect(moveDown, &QPushButton::clicked, this, &UrlFilterOptions::moveItemDown);
    connect(listWidget,
            &QListWidget::currentItemChanged,
            this,
            &UrlFilterOptions::updateActionButtons);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateActionButtons();
}

void UrlFilterOptions::addNewItem()
{
    QListWidgetItem *item = new QListWidgetItem("https://www.example.com/search?query=%1");
    listWidget->addItem(item);
    item->setSelected(true);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    listWidget->setCurrentItem(item);
    listWidget->editItem(item);
}

void UrlFilterOptions::removeItem()
{
    if (QListWidgetItem *item = listWidget->currentItem()) {
        listWidget->removeItemWidget(item);
        delete item;
    }
}

void UrlFilterOptions::moveItemUp()
{
    const int row = listWidget->currentRow();
    if (row > 0) {
        QListWidgetItem *item = listWidget->takeItem(row);
        listWidget->insertItem(row - 1, item);
        listWidget->setCurrentRow(row - 1);
    }
}

void UrlFilterOptions::moveItemDown()
{
    const int row = listWidget->currentRow();
    if (row >= 0 && row < listWidget->count() - 1) {
        QListWidgetItem *item = listWidget->takeItem(row);
        listWidget->insertItem(row + 1, item);
        listWidget->setCurrentRow(row + 1);
    }
}

void UrlFilterOptions::updateActionButtons()
{
    remove->setEnabled(listWidget->currentItem());
    const int row = listWidget->currentRow();
    moveUp->setEnabled(row > 0);
    moveDown->setEnabled(row >= 0 && row < listWidget->count() - 1);
}

} // namespace Internal

// -- UrlLocatorFilter

/*!
    \class Core::UrlLocatorFilter
    \inmodule QtCreator
    \internal
*/

UrlLocatorFilter::UrlLocatorFilter(Id id)
    : UrlLocatorFilter(Tr::tr("URL Template"), id)
{}

UrlLocatorFilter::UrlLocatorFilter(const QString &displayName, Id id)
{
    setId(id);
    m_defaultDisplayName = displayName;
    setDisplayName(displayName);
}

LocatorMatcherTasks UrlLocatorFilter::matchers()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage, urls = remoteUrls()] {
        const QString input = storage->input();
        LocatorFilterEntries entries;
        for (const QString &url : urls) {
            const QString name = url.arg(input);
            LocatorFilterEntry entry;
            entry.displayName = name;
            entry.acceptor = [name] {
                if (!name.isEmpty())
                    QDesktopServices::openUrl(name);
                return AcceptResult();
            };
            entry.highlightInfo = {int(name.lastIndexOf(input)), int(input.length())};
            entries.append(entry);
        }
        storage->reportOutput(entries);
    };
    return {{Sync(onSetup), storage}};
}

const char kDisplayNameKey[] = "displayName";
const char kRemoteUrlsKey[] = "remoteUrls";

void UrlLocatorFilter::saveState(QJsonObject &object) const
{
    if (displayName() != m_defaultDisplayName)
        object.insert(kDisplayNameKey, displayName());
    if (m_remoteUrls != m_defaultUrls)
        object.insert(kRemoteUrlsKey, QJsonArray::fromStringList(m_remoteUrls));
}

void UrlLocatorFilter::restoreState(const QJsonObject &object)
{
    setDisplayName(object.value(kDisplayNameKey).toString(m_defaultDisplayName));
    m_remoteUrls = Utils::transform(object.value(kRemoteUrlsKey)
                                        .toArray(QJsonArray::fromStringList(m_defaultUrls))
                                        .toVariantList(),
                                    &QVariant::toString);
}

void UrlLocatorFilter::restoreState(const QByteArray &state)
{
    if (isOldSetting(state)) {
        // TODO read old settings, remove some time after Qt Creator 4.15
        QDataStream in(state);

        QString value;
        in >> value;
        m_remoteUrls = value.split('^', Qt::SkipEmptyParts);

        QString shortcut;
        in >> shortcut;
        setShortcutString(shortcut);

        bool defaultFilter;
        in >> defaultFilter;
        setIncludedByDefault(defaultFilter);

        if (!in.atEnd()) {
            QString name;
            in >> name;
            setDisplayName(name);
        }
    } else {
        ILocatorFilter::restoreState(state);
    }
}

bool UrlLocatorFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    Internal::UrlFilterOptions optionsDialog(this, parent);
    if (optionsDialog.exec() == QDialog::Accepted) {
        m_remoteUrls.clear();
        setIncludedByDefault(optionsDialog.includeByDefault->isChecked());
        setShortcutString(optionsDialog.shortcutEdit->text().trimmed());
        for (int i = 0; i < optionsDialog.listWidget->count(); ++i)
            m_remoteUrls.append(optionsDialog.listWidget->item(i)->text());
        if (isCustomFilter())
            setDisplayName(optionsDialog.nameEdit->text());
        return true;
    }
    return true;
}

void UrlLocatorFilter::addDefaultUrl(const QString &urlTemplate)
{
    m_remoteUrls.append(urlTemplate);
    m_defaultUrls.append(urlTemplate);
}

} // namespace Core
