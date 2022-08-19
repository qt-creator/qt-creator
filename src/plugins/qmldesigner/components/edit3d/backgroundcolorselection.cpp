// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <QColorDialog>

#include <utils/qtcassert.h>

#include "backgroundcolorselection.h"
#include "edit3dviewconfig.h"

using namespace QmlDesigner;

void BackgroundColorSelection::showBackgroundColorSelectionWidget(QWidget *parent, const QByteArray &key,
                                                                  View3DActionCommand::Type cmdType)
{
    if (m_dialog)
        return;

    m_dialog = BackgroundColorSelection::createColorDialog(parent, key, cmdType);
    QTC_ASSERT(m_dialog, return);

    QObject::connect(m_dialog, &QWidget::destroyed, m_dialog, [&]() {
        m_dialog = nullptr;
    });
}

QColorDialog *BackgroundColorSelection::createColorDialog(QWidget *parent, const QByteArray &key,
                                                          View3DActionCommand::Type cmdType)
{
    auto dialog = new QColorDialog(parent);

    dialog->setModal(true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QList<QColor> oldColorConfig = Edit3DViewConfig::load(key);

    dialog->show();

    QObject::connect(dialog, &QColorDialog::currentColorChanged, dialog, [cmdType](const QColor &color) {
        Edit3DViewConfig::set(cmdType, color);
    });

    QObject::connect(dialog, &QColorDialog::colorSelected, dialog, [key](const QColor &color) {
        Edit3DViewConfig::save(key, color);
    });

    if (Edit3DViewConfig::isValid(oldColorConfig)) {
        QObject::connect(dialog, &QColorDialog::rejected, dialog, [cmdType, oldColorConfig]() {
            Edit3DViewConfig::set(cmdType, oldColorConfig);
        });
    }

    return dialog;
}
