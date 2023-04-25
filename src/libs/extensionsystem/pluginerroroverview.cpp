// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pluginerroroverview.h"

#include "extensionsystemtr.h"
#include "pluginmanager.h"
#include "pluginspec.h"

#include <utils/layoutbuilder.h>

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QTextEdit>

Q_DECLARE_METATYPE(ExtensionSystem::PluginSpec *)

namespace ExtensionSystem {

PluginErrorOverview::PluginErrorOverview(QWidget *parent)
    : QDialog(parent)
{
    QListWidget *pluginList = new QListWidget(this);

    QTextEdit *pluginError = new QTextEdit(this);
    pluginError->setReadOnly(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::NoButton);
    buttonBox->addButton(Tr::tr("Continue"), QDialogButtonBox::AcceptRole);

    connect(pluginList, &QListWidget::currentItemChanged,
            this, [pluginError](QListWidgetItem *item) {
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
            QListWidgetItem *item = new QListWidgetItem(spec->name());
            item->setData(Qt::UserRole, QVariant::fromValue(spec));
            pluginList->addItem(item);
        }
    }

    if (pluginList->count() > 0)
        pluginList->setCurrentRow(0);

    resize(434, 361);
}

} // namespace ExtensionSystem
