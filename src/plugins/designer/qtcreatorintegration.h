/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <QtGlobal>
#include <QDesignerIntegration>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Designer {
namespace Internal {

class QtCreatorIntegration : public QDesignerIntegration
{
    Q_OBJECT

public:
    explicit QtCreatorIntegration(QDesignerFormEditorInterface *core, QObject *parent = 0);

    QWidget *containerWindow(QWidget *widget) const override;

    bool supportsToSlotNavigation() { return true; }

    void updateSelection() override;

signals:
    void creatorHelpRequested(const QUrl &url);

private:
    void slotNavigateToSlot(const QString &objectName, const QString &signalSignature, const QStringList &parameterNames);
    void slotDesignerHelpRequested(const QString &manual, const QString &document);
    void slotSyncSettingsToDesigner();

    bool navigateToSlot(const QString &objectName,
                        const QString &signalSignature,
                        const QStringList &parameterNames,
                        QString *errorMessage);
};

} // namespace Internal
} // namespace Designer
