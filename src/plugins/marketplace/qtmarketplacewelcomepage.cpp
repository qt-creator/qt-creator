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

#include <QDesktopServices>
#include <QLabel>
#include <QLineEdit>
#include <QShowEvent>

using namespace Core;
using namespace Utils;

namespace Marketplace::Internal {

class QtMarketplacePageWidget final : public QWidget
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
            const QString txt
                    = Tr::tr(
                        "<p>Could not fetch data from Qt Marketplace.</p><p>Try with your browser "
                        "instead: <a href='https://marketplace.qt.io'>https://marketplace.qt.io</a>"
                        "</p><br/><p><small><i>Error: %1</i></small></p>").arg(message);
            m_errorLabel->setText(txt);
            m_errorLabel->setVisible(true);
            m_searcher->setVisible(false);
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

    void showEvent(QShowEvent *event) final
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

// QtMarketplaceWelcomePage

class QtMarketplaceWelcomePage final : public IWelcomePage
{
public:
    QtMarketplaceWelcomePage() = default;

    QString title() const final { return Tr::tr("Marketplace"); }
    int priority() const final { return 60; }
    Utils::Id id() const final { return "Marketplace"; }
    QWidget *createWidget() const final { return new QtMarketplacePageWidget; }
};

void setupQtMarketPlaceWelcomePage(QObject *guard)
{
    auto page = new QtMarketplaceWelcomePage;
    page->setParent(guard);
}

} // Marketplace::Internal
