// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QColorDialog>

#include <utils/qtcassert.h>

#include "backgroundcolorselection.h"
#include "edit3dviewconfig.h"

using namespace QmlDesigner;

void BackgroundColorSelection::showBackgroundColorSelectionWidget(QWidget *parent,
                                                                  ColorType colorType,
                                                                  AbstractView *view,
                                                                  const AuxiliaryDataKeyView &auxProp,
                                                                  const std::function<void()> &colorSelected)
{
    if (m_dialog)
        return;

    m_dialog = BackgroundColorSelection::createColorDialog(parent, colorType, view, auxProp, colorSelected);
    QTC_ASSERT(m_dialog, return);

    QObject::connect(m_dialog, &QWidget::destroyed, m_dialog, [&]() {
        m_dialog = nullptr;
    });
}

QColorDialog *BackgroundColorSelection::createColorDialog(QWidget *parent,
                                                          ColorType colorType,
                                                          AbstractView *view,
                                                          const AuxiliaryDataKeyView &auxProp,
                                                          const std::function<void()> &colorSelected)
{
    auto dialog = new QColorDialog(parent);

    dialog->setModal(true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QStringList colorNames;
    if (colorType == ColorType::BackGroundColor)
        colorNames = designerSettings().edit3DViewBackgroundColor();
    else
        colorNames = {designerSettings().edit3DViewGridLineColor()};

    QList<QColor> oldColorConfig = Utils::transform(colorNames, &QColor::fromString);

    if (!oldColorConfig.isEmpty()) {
        dialog->setCurrentColor(oldColorConfig.first());
        QObject::connect(dialog, &QColorDialog::rejected, dialog,
                         [auxProp, oldColorConfig, view]() {
                             Edit3DViewConfig::setColors(view, auxProp, oldColorConfig);
                         });
    }

    dialog->show();

    QObject::connect(dialog, &QColorDialog::currentColorChanged, dialog,
                     [auxProp, view](const QColor &color) {
                         Edit3DViewConfig::setColors(view, auxProp, {color});
                     });

    QObject::connect(dialog, &QColorDialog::colorSelected, dialog,
                     [colorType, colorSelected](const QColor &color) {
                         if (colorSelected)
                             colorSelected();

                        if (colorType == ColorType::BackGroundColor)
                            designerSettings().edit3DViewBackgroundColor.setValue({color.name()});
                        else
                            designerSettings().edit3DViewGridLineColor.setValue(color.name());
                     });

    return dialog;
}
