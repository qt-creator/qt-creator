/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
