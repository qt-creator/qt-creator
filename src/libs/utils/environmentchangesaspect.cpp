// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentchangesaspect.h"

#include "elidinglabel.h"
#include "environment.h"
#include "environmentdialog.h"
#include "layoutbuilder.h"
#include "utilstr.h"

#include <QPushButton>
#include <QSizePolicy>

namespace Utils {

void EnvironmentChangesAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto label = createLabel();
    if (label)
        parent.addItem(label);

    auto changesLabel = new ElidingLabel();
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    changesLabel->setSizePolicy(sizePolicy);
    changesLabel->setElideMode(Qt::ElideRight);
    auto updateChangesLabel = [this, changesLabel]() {
        const EnvironmentItems items = volatileValue().itemsFromUser();
        changesLabel->setText(EnvironmentItem::toShortSummary(items, false));
    };
    updateChangesLabel();
    connect(this, &EnvironmentChangesAspect::volatileValueChanged, this, updateChangesLabel);
    parent.addItem(changesLabel);

    QPushButton *changeButton = new QPushButton(Tr::tr("Change..."));
    changeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    parent.addItem(changeButton);
    connect(changeButton, &QPushButton::clicked, this, [changeButton, this]() {
        std::optional<EnvironmentChanges> changes
            = runEnvironmentItemsDialog(changeButton, volatileValue());
        if (changes)
            setVolatileValue(*changes);
    });
}

} // namespace Utils
