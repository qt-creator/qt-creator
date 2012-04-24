/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "toolchainconfigwidget.h"
#include "toolchain.h"

#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QString>

#include <QFormLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// ToolChainConfigWidgetPrivate
// --------------------------------------------------------------------------

class ToolChainConfigWidgetPrivate
{
public:
    ToolChainConfigWidgetPrivate(ToolChain *tc) :
        m_toolChain(tc), m_errorLabel(0)
    {
        QTC_CHECK(tc);
    }

    ToolChain *m_toolChain;
    QLabel *m_errorLabel;
};

} // namespace Internal

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

ToolChainConfigWidget::ToolChainConfigWidget(ToolChain *tc) :
    d(new Internal::ToolChainConfigWidgetPrivate(tc))
{
}

void ToolChainConfigWidget::setDisplayName(const QString &name)
{
    d->m_toolChain->setDisplayName(name);
}

ToolChain *ToolChainConfigWidget::toolChain() const
{
    return d->m_toolChain;
}

void ToolChainConfigWidget::makeReadOnly()
{ }

void ToolChainConfigWidget::addErrorLabel(QFormLayout *lt)
{
    if (!d->m_errorLabel) {
        d->m_errorLabel = new QLabel;
        d->m_errorLabel->setVisible(false);
    }
    lt->addRow(d->m_errorLabel);
}

void ToolChainConfigWidget::addErrorLabel(QGridLayout *lt, int row, int column, int colSpan)
{
    if (!d->m_errorLabel) {
        d->m_errorLabel = new QLabel;
        d->m_errorLabel->setVisible(false);
    }
    lt->addWidget(d->m_errorLabel, row, column, 1, colSpan);
}

void ToolChainConfigWidget::setErrorMessage(const QString &m)
{
    QTC_ASSERT(d->m_errorLabel, return);
    if (m.isEmpty()) {
        clearErrorMessage();
    } else {
        d->m_errorLabel->setText(m);
        d->m_errorLabel->setStyleSheet(QLatin1String("background-color: \"red\""));
        d->m_errorLabel->setVisible(true);
    }
}

void ToolChainConfigWidget::clearErrorMessage()
{
    QTC_ASSERT(d->m_errorLabel, return);
    d->m_errorLabel->clear();
    d->m_errorLabel->setStyleSheet(QString());
    d->m_errorLabel->setVisible(false);
}

} // namespace ProjectExplorer
