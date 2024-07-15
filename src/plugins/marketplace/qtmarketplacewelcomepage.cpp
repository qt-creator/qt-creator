// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtmarketplacewelcomepage.h"

#include "marketplacetr.h"
#include "productlistmodel.h"

#include <coreplugin/welcomepagehelper.h>

#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QLabel>
#include <QLineEdit>
#include <QShowEvent>
#include <QVBoxLayout>

namespace Marketplace {
namespace Internal {

using namespace Utils;

QString QtMarketplaceWelcomePage::title() const
{
    return Tr::tr("Marketplace");
}

int QtMarketplaceWelcomePage::priority() const
{
    return 60;
}

Utils::Id QtMarketplaceWelcomePage::id() const
{
    return "Marketplace";
}

class QtMarketplacePageWidget : public QWidget
{
public:
    QtMarketplacePageWidget()
    {
        m_searcher = new Core::SearchBox(this);
        m_searcher->setPlaceholderText(Tr::tr("Search in Marketplace..."));

        m_errorLabel = new QLabel(this);
        m_errorLabel->setVisible(false);

        m_sectionedProducts = new SectionedProducts(this);
        auto progressIndicator = new Utils::ProgressIndicator(ProgressIndicatorSize::Large, this);
        progressIndicator->attachToWidget(m_sectionedProducts);
        progressIndicator->hide();

        using namespace StyleHelper::SpacingTokens;

        using namespace Layouting;
        Column {
            Row {
                m_searcher,
                m_errorLabel,
                customMargins(0, 0, ExVPaddingGapXl, 0),
            },
            m_sectionedProducts,
            spacing(VPaddingL),
            customMargins(ExVPaddingGapXl, ExVPaddingGapXl, 0, 0),
        }.attachTo(this);

        connect(m_sectionedProducts, &SectionedProducts::toggleProgressIndicator,
                progressIndicator, &Utils::ProgressIndicator::setVisible);
        connect(m_sectionedProducts, &SectionedProducts::errorOccurred, this,
                [this, progressIndicator](int, const QString &message) {
            progressIndicator->hide();
            progressIndicator->deleteLater();
            m_errorLabel->setAlignment(Qt::AlignHCenter);
            QFont f(m_errorLabel->font());
            f.setPixelSize(20);
            m_errorLabel->setFont(f);
          const QString txt = Tr::tr(
                        "<p>Could not fetch data from Qt Marketplace.</p>"
                        "<p><small><i>Error: %1</i></small></p>").arg(message);
            m_errorLabel->setText(txt);
            m_errorLabel->setVisible(true);
            m_searcher->setVisible(false);
          
        });

        connect(m_searcher,
                &QLineEdit::textChanged,
                m_sectionedProducts,
                &SectionedProducts::setSearchStringDelayed);
        connect(m_sectionedProducts, &SectionedProducts::tagClicked,
                this, &QtMarketplacePageWidget::onTagClicked);
    }

    void showEvent(QShowEvent *event) override
    {
        if (!m_initialized) {
            m_initialized = true;
            m_sectionedProducts->updateCollections();
        }
        QWidget::showEvent(event);
    }

    void onTagClicked(const QString &tag)
    {
        const QString text = m_searcher->text();
        m_searcher->setText((text.startsWith("tag:\"") ? text.trimmed() + " " : QString())
                            + QString("tag:\"%1\" ").arg(tag));
    }

private:
    SectionedProducts *m_sectionedProducts = nullptr;
    QLabel *m_errorLabel = nullptr;
    QLineEdit *m_searcher = nullptr;
    bool m_initialized = false;
};


QWidget *QtMarketplaceWelcomePage::createWidget() const
{
    return new QtMarketplacePageWidget;
}

} // namespace Internal
} // namespace Marketplace
