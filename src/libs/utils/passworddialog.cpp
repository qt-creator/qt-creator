// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "passworddialog.h"

#include "layoutbuilder.h"
#include "stylehelper.h"
#include "utilsicons.h"
#include "utilstr.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QEnterEvent>
#include <QLineEdit>
#include <QPainter>

namespace Utils {

ShowPasswordButton::ShowPasswordButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setAttribute(Qt::WA_Hover);
    setCheckable(true);
    setToolTip(Tr::tr("Show/Hide Password"));
}

void ShowPasswordButton::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    QIcon icon = isChecked() ? Utils::Icons::EYE_OPEN_TOOLBAR.icon()
                             : Utils::Icons::EYE_CLOSED_TOOLBAR.icon();
    QPainter p(this);
    QRect r(QPoint(), size());

    if (m_containsMouse && isEnabled())
        StyleHelper::drawPanelBgRect(&p, r, creatorTheme()->color(Theme::FancyToolButtonHoverColor));

    QWindow *window = this->window()->windowHandle();
    QSize s = icon.actualSize(window, QSize(32, 16));

    QPixmap px = icon.pixmap(s);
    QRect iRect(QPoint(), s);
    iRect.moveCenter(r.center());
    p.drawPixmap(iRect, px);
}

void ShowPasswordButton::enterEvent(QEnterEvent *e)
{
    m_containsMouse = true;
    e->accept();
    update();
}

void ShowPasswordButton::leaveEvent(QEvent *e)
{
    m_containsMouse = false;
    e->accept();
    update();
}

QSize ShowPasswordButton::sizeHint() const
{
    QWindow *window = this->window()->windowHandle();
    QSize s = Utils::Icons::EYE_OPEN_TOOLBAR.icon().actualSize(window, QSize(32, 16)) + QSize(8, 8);

    if (StyleHelper::toolbarStyle() == StyleHelper::ToolbarStyleRelaxed)
        s += QSize(5, 5);

    return s;
}

class PasswordDialogPrivate
{
public:
    QLineEdit *m_userNameLineEdit{nullptr};
    QLineEdit *m_passwordLineEdit{nullptr};
    QCheckBox *m_checkBox{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
};

PasswordDialog::PasswordDialog(const QString &title,
                               const QString &prompt,
                               const QString &doNotAskAgainLabel,
                               bool withUsername,
                               QWidget *parent)
    : QDialog(parent)
    , d(new PasswordDialogPrivate)
{
    setWindowTitle(title);

    d->m_passwordLineEdit = new QLineEdit();
    d->m_passwordLineEdit->setEchoMode(QLineEdit::Password);

    if (withUsername)
        d->m_userNameLineEdit = new QLineEdit();

    d->m_checkBox = new QCheckBox();
    d->m_checkBox->setText(doNotAskAgainLabel);

    auto *showPasswordButton = new ShowPasswordButton;

    connect(showPasswordButton, &QAbstractButton::toggled, this, [this, showPasswordButton] {
        d->m_passwordLineEdit->setEchoMode(showPasswordButton->isChecked() ? QLineEdit::Normal
                                                                           : QLineEdit::Password);
    });

    d->m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(d->m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(d->m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    using namespace Layouting;

    // clang-format off
    Column {
        prompt,
        If {
            withUsername, {
                Form {
                    Tr::tr("User:"), d->m_userNameLineEdit, br,
                    Tr::tr("Password:"), Row { d->m_passwordLineEdit, showPasswordButton }, br,
                }
            }, {
                Row {
                    d->m_passwordLineEdit, showPasswordButton,
                },
            }
        },
        Row {
            d->m_checkBox,
            d->m_buttonBox,
        }
    }.attachTo(this);
    // clang-format on

    d->m_passwordLineEdit->setMinimumWidth(d->m_passwordLineEdit->width() / 2);
    setFixedSize(sizeHint());
}

PasswordDialog::~PasswordDialog() = default;

void PasswordDialog::setUser(const QString &user)
{
    QTC_ASSERT(d->m_userNameLineEdit, return);
    d->m_userNameLineEdit->setText(user);
}

QString PasswordDialog::user() const
{
    QTC_ASSERT(d->m_userNameLineEdit, return {});
    return d->m_userNameLineEdit->text();
}

QString PasswordDialog::password() const
{
    return d->m_passwordLineEdit->text();
}

std::optional<QPair<QString, QString>> PasswordDialog::getUserAndPassword(
    const QString &title,
    const QString &prompt,
    const QString &doNotAskAgainLabel,
    const QString &userName,
    const CheckableDecider &decider,
    QWidget *parent)
{
    if (!decider.shouldAskAgain())
        return std::nullopt;

    PasswordDialog dialog(title, prompt, doNotAskAgainLabel, true, parent);

    dialog.setUser(userName);

    if (dialog.exec() == QDialog::Accepted)
        return qMakePair(dialog.user(), dialog.password());

    if (dialog.d->m_checkBox->isChecked())
        decider.doNotAskAgain();

    return std::nullopt;
}

std::optional<QString> PasswordDialog::getPassword(const QString &title,
                                                   const QString &prompt,
                                                   const QString &doNotAskAgainLabel,
                                                   const CheckableDecider &decider,
                                                   QWidget *parent)
{
    if (!decider.shouldAskAgain())
        return std::nullopt;

    PasswordDialog dialog(title, prompt, doNotAskAgainLabel, false, parent);

    if (dialog.exec() == QDialog::Accepted)
        return dialog.password();

    if (dialog.d->m_checkBox->isChecked())
        decider.doNotAskAgain();

    return std::nullopt;
}

} // namespace Utils
