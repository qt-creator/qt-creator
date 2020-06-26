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
#include "qbsprofilemanager.h"

#include <projectexplorer/kitmanager.h>

#include <utils/elidinglabel.h>
#include <utils/qtcassert.h>

#include <QPushButton>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

class AspectWidget final : public KitAspectWidget
{
    Q_DECLARE_TR_FUNCTIONS(QbsProjectManager::Internal::AspectWidget)
public:
    AspectWidget(Kit *kit, const KitAspect *kitInfo)
        : KitAspectWidget(kit, kitInfo),
          m_contentLabel(new Utils::ElidingLabel),
          m_changeButton(new QPushButton(tr("Change...")))
    {
        connect(m_changeButton, &QPushButton::clicked, this, &AspectWidget::changeProperties);
    }

private:
    void makeReadOnly() override { m_changeButton->setEnabled(false); }
    void refresh() override { m_contentLabel->setText(QbsKitAspect::representation(kit())); }
    QWidget *mainWidget() const override { return m_contentLabel; }
    QWidget *buttonWidget() const override { return m_changeButton; }

    void changeProperties()
    {
        CustomQbsPropertiesDialog dlg(QbsKitAspect::properties(kit()));
        if (dlg.exec() == QDialog::Accepted)
            QbsKitAspect::setProperties(kit(), dlg.properties());
    }

    QLabel * const m_contentLabel;
    QPushButton * const m_changeButton;
};

QbsKitAspect::QbsKitAspect()
{
    setObjectName(QLatin1String("QbsKitAspect"));
    setId(QbsKitAspect::id());
    setDisplayName(tr("Additional Qbs Profile Settings"));
    setPriority(22000);
}

QString QbsKitAspect::representation(const Kit *kit)
{
    const QVariantMap props = properties(kit);
    QString repr;
    for (auto it = props.begin(); it != props.end(); ++it) {
        if (!repr.isEmpty())
            repr += ' ';
        repr += it.key() + ':' + toJSLiteral(it.value());
    }
    return repr;
}

QVariantMap QbsKitAspect::properties(const Kit *kit)
{
    QTC_ASSERT(kit, return QVariantMap());
    return kit->value(id()).toMap();
}

void QbsKitAspect::setProperties(Kit *kit, const QVariantMap &properties)
{
    QTC_ASSERT(kit, return);
    kit->setValue(id(), properties);
}

Utils::Id QbsKitAspect::id()
{
    return "Qbs.KitInformation";
}

Tasks QbsKitAspect::validate(const Kit *) const { return {}; }

KitAspect::ItemList QbsKitAspect::toUserOutput(const Kit *k) const
{
    return ItemList({qMakePair(displayName(), representation(k))});
}

KitAspectWidget *QbsKitAspect::createConfigWidget(Kit *k) const
{
    return new AspectWidget(k, this);
}

} // namespace Internal
} // namespace QbsProjectManager
