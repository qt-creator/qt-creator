// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "checkablemessagebox.h"

#include "hostosinfo.h"
#include "qtcassert.h"
#include "utilstr.h"

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

static QMessageBox::StandardButton exec(
    QWidget *parent,
    QMessageBox::Icon icon,
    const QString &title,
    const QString &text,
    std::optional<CheckableMessageBox::Decider> decider,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton,
    QMessageBox::StandardButton acceptButton,
    QMap<QMessageBox::StandardButton, QString> buttonTextOverrides)
{
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setIcon(icon);
    msgBox.setText(text);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setTextInteractionFlags(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse);

    if (HostOsInfo::isMacHost()) {
        // Message boxes on macOS cannot display links.
        // If the message contains a link, we need to disable native dialogs.
        if (text.contains("<a ")) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
            msgBox.setOptions(QMessageBox::Option::DontUseNativeDialog);
#endif
        }
    }

    if (decider) {
        if (!CheckableMessageBox::shouldAskAgain(*decider))
            return acceptButton;

        msgBox.setCheckBox(new QCheckBox);
        msgBox.checkBox()->setChecked(false);

        std::visit(
            [&msgBox](auto &&decider) {
                msgBox.checkBox()->setText(decider.text);
            },
            *decider);
    }

    msgBox.setStandardButtons(buttons);
    msgBox.setDefaultButton(defaultButton);
    for (auto it = buttonTextOverrides.constBegin(); it != buttonTextOverrides.constEnd(); ++it)
        msgBox.button(it.key())->setText(it.value());
    msgBox.exec();

    QMessageBox::StandardButton clickedBtn = msgBox.standardButton(msgBox.clickedButton());

    if (decider && msgBox.checkBox()->isChecked()
        && (acceptButton == QMessageBox::NoButton || clickedBtn == acceptButton))
        CheckableMessageBox::doNotAskAgain(*decider);
    return clickedBtn;
}

QMessageBox::StandardButton CheckableMessageBox::question(
    QWidget *parent,
    const QString &title,
    const QString &question,
    std::optional<Decider> decider,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton,
    QMessageBox::StandardButton acceptButton,
    QMap<QMessageBox::StandardButton, QString> buttonTextOverrides)
{
    return exec(parent,
                QMessageBox::Question,
                title,
                question,
                decider,
                buttons,
                defaultButton,
                acceptButton,
                buttonTextOverrides);
}

QMessageBox::StandardButton CheckableMessageBox::information(
    QWidget *parent,
    const QString &title,
    const QString &text,
    std::optional<Decider> decider,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton,
    QMap<QMessageBox::StandardButton, QString> buttonTextOverrides)
{
    return exec(parent,
                QMessageBox::Information,
                title,
                text,
                decider,
                buttons,
                defaultButton,
                QMessageBox::NoButton,
                buttonTextOverrides);
}

void CheckableMessageBox::doNotAskAgain(Decider &decider)
{
    std::visit(
        [](auto &&decider) {
            using T = std::decay_t<decltype(decider)>;
            if constexpr (std::is_same_v<T, BoolDecision>) {
                decider.doNotAskAgain = true;
            } else if constexpr (std::is_same_v<T, SettingsDecision>) {
                decider.settings->beginGroup(QLatin1String(kDoNotAskAgainKey));
                decider.settings->setValue(decider.settingsSubKey, true);
                decider.settings->endGroup();
            } else if constexpr (std::is_same_v<T, AspectDecision>) {
                decider.aspect.setValue(true);
            }
        },
        decider);
}

bool CheckableMessageBox::shouldAskAgain(const Decider &decider)
{
    bool result = std::visit(
        [](auto &&decider) {
            using T = std::decay_t<decltype(decider)>;
            if constexpr (std::is_same_v<T, BoolDecision>) {
                return !decider.doNotAskAgain;
            } else if constexpr (std::is_same_v<T, SettingsDecision>) {
                decider.settings->beginGroup(QLatin1String(kDoNotAskAgainKey));
                bool shouldNotAsk = decider.settings->value(decider.settingsSubKey, false).toBool();
                decider.settings->endGroup();
                return !shouldNotAsk;
            } else if constexpr (std::is_same_v<T, AspectDecision>) {
                return !decider.aspect.value();
            }
        },
        decider);

    return result;
}

bool CheckableMessageBox::shouldAskAgain(QSettings *settings, const QString &key)
{
    return shouldAskAgain(make_decider(settings, key));
}

void CheckableMessageBox::doNotAskAgain(QSettings *settings, const QString &key)
{
    Decider decider = make_decider(settings, key);
    return doNotAskAgain(decider);
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
    settings->beginGroup(QLatin1String(kDoNotAskAgainKey));
    const bool hasSuppressed = !settings->childKeys().isEmpty()
                               || !settings->childGroups().isEmpty();
    settings->endGroup();
    return hasSuppressed;
}

/*!
    Returns the standard \gui {Do not ask again} check box text.
    \sa doNotAskAgainQuestion()
*/
QString CheckableMessageBox::msgDoNotAskAgain()
{
    return Tr::tr("Do not &ask again");
}

/*!
    Returns the standard \gui {Do not show again} check box text.
    \sa doNotShowAgainInformation()
*/
QString CheckableMessageBox::msgDoNotShowAgain()
{
    return Tr::tr("Do not &show again");
}

} // namespace Utils
