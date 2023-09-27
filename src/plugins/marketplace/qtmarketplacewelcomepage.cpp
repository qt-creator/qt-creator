// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtmarketplacewelcomepage.h"

#include "marketplacetr.h"
#include "productlistmodel.h"

#include <coreplugin/welcomepagehelper.h>

#include <utils/fancylineedit.h>
#include <utils/progressindicator.h>
#include <utils/theme/theme.h>
#include <utils/qtcassert.h>

#include <QDesktopServices>
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
        auto searchBox = new Core::SearchBox(this);
        m_searcher = searchBox->m_lineEdit;
        m_searcher->setPlaceholderText(Tr::tr("Search in Marketplace..."));

        auto vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(0, 0, 0, Core::WelcomePageHelpers::ItemGap);
        vbox->setSpacing(Core::WelcomePageHelpers::ItemGap);

        auto searchBar = Core::WelcomePageHelpers::panelBar();
        auto hbox = new QHBoxLayout(searchBar);
        hbox->setContentsMargins(Core::WelcomePageHelpers::HSpacing, 0,
                                 Core::WelcomePageHelpers::HSpacing, 0);
        hbox->addWidget(searchBox);
        vbox->addWidget(searchBar);
        m_errorLabel = new QLabel(this);
        m_errorLabel->setVisible(false);
        vbox->addWidget(m_errorLabel);

        auto resultWidget = new QWidget(this);
        auto resultHBox = new QHBoxLayout(resultWidget);
        resultHBox->setContentsMargins(Core::WelcomePageHelpers::HSpacing, 0, 0, 0);
        m_sectionedProducts = new SectionedProducts(this);
        auto progressIndicator = new Utils::ProgressIndicator(ProgressIndicatorSize::Large, this);
        progressIndicator->attachToWidget(m_sectionedProducts);
        progressIndicator->hide();
        resultHBox->addWidget(m_sectionedProducts);
        vbox->addWidget(resultWidget);

        connect(m_sectionedProducts, &SectionedProducts::toggleProgressIndicator,
                progressIndicator, &Utils::ProgressIndicator::setVisible);
        connect(m_sectionedProducts, &SectionedProducts::errorOccurred, this,
                [this, progressIndicator, searchBox](int, const QString &message) {
            progressIndicator->hide();
            progressIndicator->deleteLater();
            m_errorLabel->setAlignment(Qt::AlignHCenter);
            QFont f(m_errorLabel->font());
            f.setPixelSize(20);
            m_errorLabel->setFont(f);
            const QString txt
                    = Tr::tr(
                        "<p>Could not fetch data from Qt Marketplace.</p><p>Try with your browser "
                        "instead: <a href='https://marketplace.qt.io'>https://marketplace.qt.io</a>"
                        "</p><br/><p><small><i>Error: %1</i></small></p>").arg(message);
            m_errorLabel->setText(txt);
            m_errorLabel->setVisible(true);
            searchBox->setVisible(false);
            connect(m_errorLabel, &QLabel::linkActivated,
                    this, []() { QDesktopServices::openUrl(QUrl("https://marketplace.qt.io")); });
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
