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

#include "backgroundcolorselection.h"

#include <nodeinstanceview.h>
#include <utils/qtcassert.h>
#include <view3dactioncommand.h>
#include <qmldesignerplugin.h>

using namespace QmlDesigner;

namespace {
QList<QColor> readBackgroundColorConfiguration()
{
    QVariant var = QmlDesigner::DesignerSettings::getValue(
        QmlDesigner::DesignerSettingsKey::EDIT3DVIEW_BACKGROUND_COLOR);

    if (!var.isValid())
        return {};

    auto colorNameList = var.value<QList<QString>>();
    QTC_ASSERT(colorNameList.size() == 2, return {});

    return {colorNameList[0], colorNameList[1]};
}

void setBackgroundColorConfiguration(const QList<QColor> &colorConfig)
{
    auto view = QmlDesignerPlugin::instance()->viewManager().nodeInstanceView();
    View3DActionCommand cmd(View3DActionCommand::SelectBackgroundColor,
                            QVariant::fromValue(colorConfig));
    view->view3DAction(cmd);
}

void saveBackgroundColorConfiguration(const QList<QColor> &colorConfig)
{
    QList<QString> colorsSaved = {colorConfig[0].name(), colorConfig[1].name()};
    QmlDesigner::DesignerSettings::setValue(
        QmlDesigner::DesignerSettingsKey::EDIT3DVIEW_BACKGROUND_COLOR,
        QVariant::fromValue(colorsSaved));
}

} // namespace

QColorDialog *BackgroundColorSelection::createDialog(QWidget *parent)
{
    auto dialog = new QColorDialog(parent);

    dialog->setModal(true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    const QList<QColor> oldColorConfig = readBackgroundColorConfiguration();

    dialog->show();

    QObject::connect(dialog, &QColorDialog::currentColorChanged, dialog, [](const QColor &color) {
        setBackgroundColorConfiguration({color, color});
    });

    QObject::connect(dialog, &QColorDialog::colorSelected, dialog, [](const QColor &color) {
        saveBackgroundColorConfiguration({color, color});
    });

    if (!oldColorConfig.isEmpty()) {
        QObject::connect(dialog, &QColorDialog::rejected, dialog, [oldColorConfig]() {
            setBackgroundColorConfiguration(oldColorConfig);
        });
    }

    return dialog;
}

void BackgroundColorSelection::showBackgroundColorSelectionWidget(QWidget *parent)
{
    if (m_dialog)
        return;

    m_dialog = BackgroundColorSelection::createDialog(parent);
    QTC_ASSERT(m_dialog, return);

    QObject::connect(m_dialog, &QWidget::destroyed, m_dialog, [&]() {
        m_dialog = nullptr;
    });
}
