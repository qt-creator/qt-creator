// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pluginerroroverview.h"

#include "extensionsystemtr.h"
#include "plugindetailsview.h"
#include "pluginmanager.h"
#include "pluginspec.h"

#include <utils/algorithm.h>
#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>

#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>

using namespace Utils;

namespace ExtensionSystem {

class  PluginErrorOverview : public QDialog
{
public:
    PluginErrorOverview();
};

PluginErrorOverview::PluginErrorOverview()
    : QDialog(dialogParent())
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    QListWidget *pluginList = new QListWidget(this);

    const auto showPluginDetails = [this](QListWidgetItem *item) {
        QTC_ASSERT(item, return);
        auto spec = item->data(Qt::UserRole).value<PluginSpec *>();
        QTC_ASSERT(spec, return);
        PluginDetailsView::showModal(this, spec);
    };

    QTextEdit *pluginError = new QTextEdit(this);
    pluginError->setReadOnly(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::NoButton);
    buttonBox->addButton(Tr::tr("Continue"), QDialogButtonBox::AcceptRole);

    QPushButton *detailsButton = buttonBox->addButton(Tr::tr("Details"), QDialogButtonBox::HelpRole);
    connect(detailsButton, &QPushButton::clicked, this, [pluginList, showPluginDetails] {
        QListWidgetItem *item = pluginList->currentItem();
        showPluginDetails(item);
    });
    connect(pluginList,
            &QListWidget::itemDoubleClicked,
            this,
            [showPluginDetails](QListWidgetItem *item) { showPluginDetails(item); });

    connect(pluginList, &QListWidget::currentItemChanged, this, [pluginError](QListWidgetItem *item) {
        if (item)
            pluginError->setText(item->data(Qt::UserRole).value<PluginSpec *>()->errorString());
        else
            pluginError->clear();
    });
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    using namespace Layouting;

    auto createLabel = [this](const QString &text) {
        QLabel *label = new QLabel(text, this);
        label->setWordWrap(true);
        return label;
    };

    Column {
        createLabel(Tr::tr("The following plugins have errors and cannot be loaded:")),
        pluginList,
        createLabel(Tr::tr("Details:")),
        pluginError,
        buttonBox
    }.attachTo(this);

    for (PluginSpec *spec : PluginManager::plugins()) {
        // only show errors on startup if plugin is enabled.
        if (spec->hasError() && spec->isEffectivelyEnabled()) {
            const QString name = spec->displayName();
            QListWidgetItem *item = new QListWidgetItem(name);
            item->setData(Qt::UserRole, QVariant::fromValue(spec));
            pluginList->addItem(item);
        }
    }

    if (pluginList->count() > 0)
        pluginList->setCurrentRow(0);

    resize(434, 361);
}

void showPluginErrorOverview()
{
    (new ExtensionSystem::PluginErrorOverview)->show();
}

} // namespace ExtensionSystem
