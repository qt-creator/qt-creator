/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qbskitinformation.h"

#include "customqbspropertiesdialog.h"

#include <projectexplorer/kitconfigwidget.h>
#include <projectexplorer/kitmanager.h>

#include <qbs.h>

#include <QLabel>
#include <QPushButton>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

class ConfigWidget final : public KitConfigWidget
{
    Q_OBJECT
public:
    ConfigWidget(Kit *kit, const KitInformation *kitInfo)
        : KitConfigWidget(kit, kitInfo),
          m_contentLabel(new QLabel),
          m_changeButton(new QPushButton(tr("Change...")))
    {
        connect(m_changeButton, &QPushButton::clicked, this, &ConfigWidget::changeProperties);
    }

private:
    QString displayName() const override { return QbsKitInformation::displayName(); }
    void makeReadOnly() override { m_changeButton->setEnabled(false); }
    void refresh() override { m_contentLabel->setText(QbsKitInformation::representation(kit())); }
    QWidget *mainWidget() const override { return m_contentLabel; }
    QWidget *buttonWidget() const override { return m_changeButton; }

    void changeProperties()
    {
        CustomQbsPropertiesDialog dlg(QbsKitInformation::properties(kit()));
        if (dlg.exec() == QDialog::Accepted)
            QbsKitInformation::setProperties(kit(), dlg.properties());
    }

    QLabel * const m_contentLabel;
    QPushButton * const m_changeButton;
};

QString QbsKitInformation::displayName()
{
    return tr("Additional Qbs Profile Settings");
}

QString QbsKitInformation::representation(const Kit *kit)
{
    const QVariantMap props = properties(kit);
    QString repr;
    for (auto it = props.begin(); it != props.end(); ++it) {
        if (!repr.isEmpty())
            repr += ' ';
        repr += it.key() + ':' + qbs::settingsValueToRepresentation(it.value());
    }
    return repr;
}

QVariantMap QbsKitInformation::properties(const Kit *kit)
{
    return kit->value(id()).toMap();
}

void QbsKitInformation::setProperties(Kit *kit, const QVariantMap &properties)
{
    kit->setValue(id(), properties);
}

Core::Id QbsKitInformation::id()
{
    return "Qbs.KitInformation";
}

QVariant QbsKitInformation::defaultValue(const Kit *) const { return QString(); }
QList<Task> QbsKitInformation::validate(const Kit *) const { return QList<Task>(); }

KitInformation::ItemList QbsKitInformation::toUserOutput(const Kit *k) const
{
    return ItemList({qMakePair(displayName(), representation(k))});
}

KitConfigWidget *QbsKitInformation::createConfigWidget(Kit *k) const
{
    return new ConfigWidget(k, this);
}

} // namespace Internal
} // namespace QbsProjectManager

#include <qbskitinformation.moc>
