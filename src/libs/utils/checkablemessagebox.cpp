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

#include "checkablemessagebox.h"
#include "qtcassert.h"

#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QTextEdit>

/*!
    \class Utils::CheckableMessageBox

    \brief The CheckableMessageBox class implements a message box suitable for
    questions with a
     "Do not ask me again" checkbox.

    Emulates the QMessageBox API with
    static conveniences. The message label can open external URLs.
*/

static const char kDoNotAskAgainKey[] = "DoNotAskAgain";

namespace Utils {

class CheckableMessageBoxPrivate
{
public:
    CheckableMessageBoxPrivate(QDialog *q)
    {
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

        pixmapLabel = new QLabel(q);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(pixmapLabel->sizePolicy().hasHeightForWidth());
        pixmapLabel->setSizePolicy(sizePolicy);
        pixmapLabel->setVisible(false);
        pixmapLabel->setFocusPolicy(Qt::NoFocus);

        auto pixmapSpacer =
            new QSpacerItem(0, 5, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        messageLabel = new QLabel(q);
        messageLabel->setMinimumSize(QSize(300, 0));
        messageLabel->setWordWrap(true);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse);
        messageLabel->setFocusPolicy(Qt::NoFocus);
        messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

        checkBox = new QCheckBox(q);
        checkBox->setText(CheckableMessageBox::tr("Do not ask again"));

        const QString showText = CheckableMessageBox::tr("Show Details...");
        detailsButton = new QPushButton(showText, q);
        detailsButton->setAutoDefault(false);
        detailsButton->hide();
        detailsText = new QTextEdit(q);
        detailsText->hide();
        QObject::connect(detailsButton, &QPushButton::clicked, detailsText, [this, showText] {
            detailsText->setVisible(!detailsText->isVisible());
            detailsButton->setText(
                detailsText->isVisible() ? CheckableMessageBox::tr("Hide Details...") : showText);
        });

        buttonBox = new QDialogButtonBox(q);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        auto verticalLayout = new QVBoxLayout();
        verticalLayout->addWidget(pixmapLabel);
        verticalLayout->addItem(pixmapSpacer);

        auto horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->addLayout(verticalLayout);
        horizontalLayout_2->addWidget(messageLabel, 10);

        auto horizontalLayout = new QHBoxLayout();
        horizontalLayout->addWidget(checkBox);
        horizontalLayout->addStretch(10);

        auto detailsButtonLayout = new QHBoxLayout;
        detailsButtonLayout->addWidget(detailsButton);
        detailsButtonLayout->addStretch(10);

        auto verticalLayout_2 = new QVBoxLayout(q);
        verticalLayout_2->addLayout(horizontalLayout_2);
        verticalLayout_2->addLayout(horizontalLayout);
        verticalLayout_2->addLayout(detailsButtonLayout);
        verticalLayout_2->addWidget(detailsText, 10);
        verticalLayout_2->addStretch(1);
        verticalLayout_2->addWidget(buttonBox);
    }

    QLabel *pixmapLabel = nullptr;
    QLabel *messageLabel = nullptr;
    QCheckBox *checkBox = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
    QAbstractButton *clickedButton = nullptr;
    QPushButton *detailsButton = nullptr;
    QTextEdit *detailsText = nullptr;
    QMessageBox::Icon icon = QMessageBox::NoIcon;
};

CheckableMessageBox::CheckableMessageBox(QWidget *parent) :
    QDialog(parent),
    d(new CheckableMessageBoxPrivate(this))
{
    setModal(true);
    connect(d->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(d->buttonBox, &QDialogButtonBox::clicked,
            this, [this](QAbstractButton *b) { d->clickedButton = b; });
}

CheckableMessageBox::~CheckableMessageBox()
{
    delete d;
}

QAbstractButton *CheckableMessageBox::clickedButton() const
{
    return d->clickedButton;
}

QDialogButtonBox::StandardButton CheckableMessageBox::clickedStandardButton() const
{
    if (d->clickedButton)
        return d->buttonBox->standardButton(d->clickedButton);
    return QDialogButtonBox::NoButton;
}

QString CheckableMessageBox::text() const
{
    return d->messageLabel->text();
}

void CheckableMessageBox::setText(const QString &t)
{
    d->messageLabel->setText(t);
}

QMessageBox::Icon CheckableMessageBox::icon() const
{
    return d->icon;
}

// See QMessageBoxPrivate::standardIcon
static QPixmap pixmapForIcon(QMessageBox::Icon icon, QWidget *w)
{
    const QStyle *style = w ? w->style() : QApplication::style();
    const int iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, w);
    QIcon tmpIcon;
    switch (icon) {
    case QMessageBox::Information:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxInformation, nullptr, w);
        break;
    case QMessageBox::Warning:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, w);
        break;
    case QMessageBox::Critical:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxCritical, nullptr, w);
        break;
    case QMessageBox::Question:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxQuestion, nullptr, w);
        break;
    default:
        break;
    }
    if (!tmpIcon.isNull()) {
        QWindow *window = nullptr;
        if (w) {
            window = w->windowHandle();
            if (!window) {
                if (const QWidget *nativeParent = w->nativeParentWidget())
                    window = nativeParent->windowHandle();
            }
        }
        return tmpIcon.pixmap(window, QSize(iconSize, iconSize));
    }
    return QPixmap();
}

void CheckableMessageBox::setIcon(QMessageBox::Icon icon)
{
    d->icon = icon;
    const QPixmap pixmap = pixmapForIcon(icon, this);
    d->pixmapLabel->setPixmap(pixmap);
    d->pixmapLabel->setVisible(!pixmap.isNull());
}

bool CheckableMessageBox::isChecked() const
{
    return d->checkBox->isChecked();
}

void CheckableMessageBox::setChecked(bool s)
{
    d->checkBox->setChecked(s);
}

QString CheckableMessageBox::checkBoxText() const
{
    return d->checkBox->text();
}

void CheckableMessageBox::setCheckBoxText(const QString &t)
{
    d->checkBox->setText(t);
}

bool CheckableMessageBox::isCheckBoxVisible() const
{
    return d->checkBox->isVisible();
}

void CheckableMessageBox::setCheckBoxVisible(bool v)
{
    d->checkBox->setVisible(v);
}

QString CheckableMessageBox::detailedText() const
{
    return d->detailsText->toPlainText();
}

void CheckableMessageBox::setDetailedText(const QString &text)
{
    d->detailsText->setText(text);
    if (!text.isEmpty())
        d->detailsButton->setVisible(true);
}

QDialogButtonBox::StandardButtons CheckableMessageBox::standardButtons() const
{
    return d->buttonBox->standardButtons();
}

void CheckableMessageBox::setStandardButtons(QDialogButtonBox::StandardButtons s)
{
    d->buttonBox->setStandardButtons(s);
}

QPushButton *CheckableMessageBox::button(QDialogButtonBox::StandardButton b) const
{
    return d->buttonBox->button(b);
}

QPushButton *CheckableMessageBox::addButton(const QString &text, QDialogButtonBox::ButtonRole role)
{
    return d->buttonBox->addButton(text, role);
}

QDialogButtonBox::StandardButton CheckableMessageBox::defaultButton() const
{
    const QList<QAbstractButton *> buttons = d->buttonBox->buttons();
    for (QAbstractButton *b : buttons)
        if (auto *pb = qobject_cast<QPushButton *>(b))
            if (pb->isDefault())
               return d->buttonBox->standardButton(pb);
    return QDialogButtonBox::NoButton;
}

void CheckableMessageBox::setDefaultButton(QDialogButtonBox::StandardButton s)
{
    if (QPushButton *b = d->buttonBox->button(s)) {
        b->setDefault(true);
        b->setFocus();
    }
}

QDialogButtonBox::StandardButton
CheckableMessageBox::question(QWidget *parent,
                              const QString &title,
                              const QString &question,
                              const QString &checkBoxText,
                              bool *checkBoxSetting,
                              QDialogButtonBox::StandardButtons buttons,
                              QDialogButtonBox::StandardButton defaultButton)
{
    CheckableMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setIcon(QMessageBox::Question);
    mb.setText(question);
    mb.setCheckBoxText(checkBoxText);
    mb.setChecked(*checkBoxSetting);
    mb.setStandardButtons(buttons);
    mb.setDefaultButton(defaultButton);
    mb.exec();
    *checkBoxSetting = mb.isChecked();
    return mb.clickedStandardButton();
}

QDialogButtonBox::StandardButton
CheckableMessageBox::information(QWidget *parent,
                              const QString &title,
                              const QString &text,
                              const QString &checkBoxText,
                              bool *checkBoxSetting,
                              QDialogButtonBox::StandardButtons buttons,
                              QDialogButtonBox::StandardButton defaultButton)
{
    CheckableMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setIcon(QMessageBox::Information);
    mb.setText(text);
    mb.setCheckBoxText(checkBoxText);
    mb.setChecked(*checkBoxSetting);
    mb.setStandardButtons(buttons);
    mb.setDefaultButton(defaultButton);
    mb.exec();
    *checkBoxSetting = mb.isChecked();
    return mb.clickedStandardButton();
}

QMessageBox::StandardButton CheckableMessageBox::dialogButtonBoxToMessageBoxButton(QDialogButtonBox::StandardButton db)
{
    return static_cast<QMessageBox::StandardButton>(int(db));
}

bool CheckableMessageBox::shouldAskAgain(QSettings *settings, const QString &settingsSubKey)
{
    if (QTC_GUARD(settings)) {
        settings->beginGroup(QLatin1String(kDoNotAskAgainKey));
        bool shouldNotAsk = settings->value(settingsSubKey, false).toBool();
        settings->endGroup();
        if (shouldNotAsk)
            return false;
    }
    return true;
}

enum DoNotAskAgainType{Question, Information};

void initDoNotAskAgainMessageBox(CheckableMessageBox &messageBox, const QString &title,
                                 const QString &text, QDialogButtonBox::StandardButtons buttons,
                                 QDialogButtonBox::StandardButton defaultButton,
                                 DoNotAskAgainType type)
{
    messageBox.setWindowTitle(title);
    messageBox.setIcon(type == Information ? QMessageBox::Information : QMessageBox::Question);
    messageBox.setText(text);
    messageBox.setCheckBoxVisible(true);
    messageBox.setCheckBoxText(type == Information ? CheckableMessageBox::msgDoNotShowAgain()
                                                   : CheckableMessageBox::msgDoNotAskAgain());
    messageBox.setChecked(false);
    messageBox.setStandardButtons(buttons);
    messageBox.setDefaultButton(defaultButton);
}

void CheckableMessageBox::doNotAskAgain(QSettings *settings, const QString &settingsSubKey)
{
    if (!settings)
        return;

    settings->beginGroup(QLatin1String(kDoNotAskAgainKey));
    settings->setValue(settingsSubKey, true);
    settings->endGroup();
}

/*!
    Shows a message box with given \a title and \a text, and a \gui {Do not ask again} check box.
    If the user checks the check box and accepts the dialog with the \a acceptButton,
    further invocations of this function with the same \a settings and \a settingsSubKey will not
    show the dialog, but instantly return \a acceptButton.

    Returns the clicked button, or QDialogButtonBox::NoButton if the user rejects the dialog
    with the escape key, or \a acceptButton if the dialog is suppressed.
*/
QDialogButtonBox::StandardButton
CheckableMessageBox::doNotAskAgainQuestion(QWidget *parent, const QString &title,
                                           const QString &text, QSettings *settings,
                                           const QString &settingsSubKey,
                                           QDialogButtonBox::StandardButtons buttons,
                                           QDialogButtonBox::StandardButton defaultButton,
                                           QDialogButtonBox::StandardButton acceptButton)

{
    if (!shouldAskAgain(settings, settingsSubKey))
        return acceptButton;

    CheckableMessageBox messageBox(parent);
    initDoNotAskAgainMessageBox(messageBox, title, text, buttons, defaultButton, Question);
    messageBox.exec();
    if (messageBox.isChecked() && (messageBox.clickedStandardButton() == acceptButton))
        doNotAskAgain(settings, settingsSubKey);

    return messageBox.clickedStandardButton();
}

/*!
    Shows a message box with given \a title and \a text, and a \gui {Do not show again} check box.
    If the user checks the check box and quits the dialog, further invocations of this
    function with the same \a settings and \a settingsSubKey will not show the dialog, but instantly return.

    Returns the clicked button, or QDialogButtonBox::NoButton if the user rejects the dialog
    with the escape key, or \a defaultButton if the dialog is suppressed.
*/
QDialogButtonBox::StandardButton
CheckableMessageBox::doNotShowAgainInformation(QWidget *parent, const QString &title,
                                           const QString &text, QSettings *settings,
                                           const QString &settingsSubKey,
                                           QDialogButtonBox::StandardButtons buttons,
                                           QDialogButtonBox::StandardButton defaultButton)

{
    if (!shouldAskAgain(settings, settingsSubKey))
            return defaultButton;

    CheckableMessageBox messageBox(parent);
    initDoNotAskAgainMessageBox(messageBox, title, text, buttons, defaultButton, Information);
    messageBox.exec();
    if (messageBox.isChecked())
        doNotAskAgain(settings, settingsSubKey);

    return messageBox.clickedStandardButton();
}

/*!
    Resets all suppression settings for doNotAskAgainQuestion() found in \a settings,
    so all these message boxes are shown again.
 */
void CheckableMessageBox::resetAllDoNotAskAgainQuestions(QSettings *settings)
{
    QTC_ASSERT(settings, return);
    settings->beginGroup(QLatin1String(kDoNotAskAgainKey));
    settings->remove(QString());
    settings->endGroup();
}

/*!
    Returns whether any message boxes from doNotAskAgainQuestion() are suppressed
    in the \a settings.
*/
bool CheckableMessageBox::hasSuppressedQuestions(QSettings *settings)
{
    QTC_ASSERT(settings, return false);
    bool hasSuppressed = false;
    settings->beginGroup(QLatin1String(kDoNotAskAgainKey));
    const QStringList childKeys = settings->childKeys();
    for (const QString &subKey : childKeys) {
        if (settings->value(subKey, false).toBool()) {
            hasSuppressed = true;
            break;
        }
    }
    settings->endGroup();
    return hasSuppressed;
}

/*!
    Returns the standard \gui {Do not ask again} check box text.
    \sa doNotAskAgainQuestion()
*/
QString CheckableMessageBox::msgDoNotAskAgain()
{
    return QApplication::translate("Utils::CheckableMessageBox", "Do not &ask again");
}

/*!
    Returns the standard \gui {Do not show again} check box text.
    \sa doNotShowAgainInformation()
*/
QString CheckableMessageBox::msgDoNotShowAgain()
{
    return QApplication::translate("Utils::CheckableMessageBox", "Do not &show again");
}

} // namespace Utils
