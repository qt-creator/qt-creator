// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbskitinformation.h"

#include "customqbspropertiesdialog.h"
#include "qbsprofilemanager.h"
#include "qbsprojectmanagertr.h"

#include <projectexplorer/kitmanager.h>

#include <utils/elidinglabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QPushButton>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

class AspectWidget final : public KitAspectWidget
{
public:
    AspectWidget(Kit *kit, const KitAspect *kitInfo)
        : KitAspectWidget(kit, kitInfo),
          m_contentLabel(createSubWidget<Utils::ElidingLabel>()),
          m_changeButton(createSubWidget<QPushButton>(Tr::tr("Change...")))
    {
        connect(m_changeButton, &QPushButton::clicked, this, &AspectWidget::changeProperties);
    }

private:
    void makeReadOnly() override { m_changeButton->setEnabled(false); }
    void refresh() override { m_contentLabel->setText(QbsKitAspect::representation(kit())); }

    void addToLayout(Utils::LayoutBuilder &builder) override
    {
        addMutableAction(m_contentLabel);
        builder.addItem(m_contentLabel);
        builder.addItem(m_changeButton);
    }

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
    setDisplayName(Tr::tr("Additional Qbs Profile Settings"));
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
    return {{displayName(), representation(k)}};
}

KitAspectWidget *QbsKitAspect::createConfigWidget(Kit *k) const
{
    return new AspectWidget(k, this);
}

} // namespace Internal
} // namespace QbsProjectManager
