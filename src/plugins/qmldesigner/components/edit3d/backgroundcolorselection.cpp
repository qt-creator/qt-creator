// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QColorDialog>

#include <utils/qtcassert.h>

#include "backgroundcolorselection.h"
#include "edit3dviewconfig.h"

using namespace QmlDesigner;

void BackgroundColorSelection::showBackgroundColorSelectionWidget(QWidget *parent,
                                                                  const QByteArray &key,
                                                                  AbstractView *view,
                                                                  View3DActionType actionType,
                                                                  const std::function<void()> &colorSelected)
{
    if (m_dialog)
        return;

    m_dialog = BackgroundColorSelection::createColorDialog(parent, key, view, actionType, colorSelected);
    QTC_ASSERT(m_dialog, return);

    QObject::connect(m_dialog, &QWidget::destroyed, m_dialog, [&]() {
        m_dialog = nullptr;
    });
}

QColorDialog *BackgroundColorSelection::createColorDialog(QWidget *parent,
                                                          const QByteArray &key,
                                                          AbstractView *view,
                                                          View3DActionType actionType,
                                                          const std::function<void()> &colorSelected)
{
    auto dialog = new QColorDialog(parent);

    dialog->setModal(true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QList<QColor> oldColorConfig = Edit3DViewConfig::loadColor(key);

    dialog->show();

    QObject::connect(dialog, &QColorDialog::currentColorChanged, dialog,
                     [actionType, view](const QColor &color) {
                         Edit3DViewConfig::setColors(view, actionType, {color});
                     });

    QObject::connect(dialog, &QColorDialog::colorSelected, dialog,
                     [key, colorSelected](const QColor &color) {
                         if (colorSelected)
                             colorSelected();

                         Edit3DViewConfig::saveColors(key, {color});
                     });

    if (Edit3DViewConfig::colorsValid(oldColorConfig)) {
        QObject::connect(dialog, &QColorDialog::rejected, dialog,
                         [actionType, oldColorConfig, view]() {
                             Edit3DViewConfig::setColors(view, actionType, oldColorConfig);
                         });
    }

    return dialog;
}
