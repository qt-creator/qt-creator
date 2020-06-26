/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qtmarketplacewelcomepage.h"

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
    return tr("Marketplace");
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
        const int sideMargin = 27;
        auto searchBox = new Core::SearchBox(this);
        m_searcher = searchBox->m_lineEdit;
        m_searcher->setPlaceholderText(QtMarketplaceWelcomePage::tr("Search in Marketplace..."));

        auto vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(30, sideMargin, 0, 0);

        auto hbox = new QHBoxLayout;
        hbox->addWidget(searchBox);
        hbox->addSpacing(sideMargin);
        vbox->addItem(hbox);
        m_errorLabel = new QLabel(this);
        m_errorLabel->setVisible(false);
        vbox->addWidget(m_errorLabel);

        m_sectionedProducts = new SectionedProducts(this);
        auto progressIndicator = new Utils::ProgressIndicator(ProgressIndicatorSize::Large, this);
        progressIndicator->attachToWidget(m_sectionedProducts);
        progressIndicator->hide();
        vbox->addWidget(m_sectionedProducts);

        connect(m_sectionedProducts, &SectionedProducts::toggleProgressIndicator,
                progressIndicator, &Utils::ProgressIndicator::setVisible);
        connect(m_sectionedProducts, &SectionedProducts::errorOccurred,
                [this, progressIndicator, searchBox](int, const QString &message) {
            progressIndicator->hide();
            progressIndicator->deleteLater();
            m_errorLabel->setAlignment(Qt::AlignHCenter);
            QFont f(m_errorLabel->font());
            f.setPixelSize(20);
            m_errorLabel->setFont(f);
            const QString txt
                    = QtMarketplaceWelcomePage::tr(
                        "<p>Could not fetch data from Qt Marketplace.</p><p>Try with your browser "
                        "instead: <a href='https://marketplace.qt.io'>https://marketplace.qt.io</a>"
                        "</p><br/><p><small><i>Error: %1</i></small></p>").arg(message);
            m_errorLabel->setText(txt);
            m_errorLabel->setVisible(true);
            searchBox->setVisible(false);
            connect(m_errorLabel, &QLabel::linkActivated,
                    this, []() { QDesktopServices::openUrl(QUrl("https://marketplace.qt.io")); });
        });

        connect(m_searcher, &QLineEdit::textChanged,
                m_sectionedProducts, &SectionedProducts::setSearchString);
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

    void resizeEvent(QResizeEvent *ev) final
    {
        QWidget::resizeEvent(ev);
        m_sectionedProducts->setColumnCount(bestColumnCount());
    }

    int bestColumnCount() const
    {
        return qMax(1, width() / (Core::GridProxyModel::GridItemWidth
                                  + Core::GridProxyModel::GridItemGap));
    }

    void onTagClicked(const QString &tag)
    {
        QString text = m_searcher->text();
        m_searcher->setText(text + QString("tag:\"%1\" ").arg(tag));
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
