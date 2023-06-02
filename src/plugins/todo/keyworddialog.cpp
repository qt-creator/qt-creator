// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "keyworddialog.h"

#include "keyword.h"
#include "lineparser.h"
#include "todotr.h"

#include <utils/layoutbuilder.h>
#include <utils/qtcolorbutton.h>

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>

namespace Todo {
namespace Internal {

KeywordDialog::KeywordDialog(const Keyword &keyword, const QSet<QString> &alreadyUsedKeywordNames,
                             QWidget *parent) :
    QDialog(parent),
    m_alreadyUsedKeywordNames(alreadyUsedKeywordNames)
{
    setWindowTitle(Tr::tr("Keyword"));

    m_listWidget = new QListWidget(this);

    m_colorEdit = new QLineEdit;
    m_colorEdit->setInputMask(QString::fromUtf8("\\#HHHHHH; "));
    m_colorEdit->setText(QString::fromUtf8("#000000"));

    m_colorButton = new Utils::QtColorButton;
    m_colorButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_colorButton->setMinimumSize(QSize(64, 0));

    m_keywordNameEdit = new QLineEdit(keyword.name);

    m_errorLabel = new QLabel(Tr::tr("errorLabel"), this);
    m_errorLabel->setStyleSheet(QString::fromUtf8("color: red;"));
    m_errorLabel->hide();

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setOrientation(Qt::Horizontal);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        new QLabel(Tr::tr("Icon")),
        m_listWidget,
        Row {
            Group {
                title(Tr::tr("Color")),
                Row { m_colorEdit, m_colorButton }
            },
            Group {
                title(Tr::tr("Keyword")),
                Column { m_keywordNameEdit }
            }
        },
        m_errorLabel,
        m_buttonBox
    }.attachTo(this);

    setupListWidget(keyword.iconType);
    setupColorWidgets(keyword.color);

    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &KeywordDialog::acceptButtonClicked);
    connect(m_keywordNameEdit, &QLineEdit::textChanged, m_errorLabel, &QWidget::hide);
}

KeywordDialog::~KeywordDialog() = default;

Keyword KeywordDialog::keyword()
{
    Keyword result;
    result.name = keywordName();
    result.iconType = static_cast<IconType>(m_listWidget->currentItem()->data(Qt::UserRole).toInt());
    result.color = m_colorEdit->text();

    return result;
}

void KeywordDialog::colorSelected(const QColor &color)
{
    m_colorEdit->setText(color.name());
}

void KeywordDialog::acceptButtonClicked()
{
    if (canAccept())
        accept();
}

void KeywordDialog::setupListWidget(IconType selectedIcon)
{
    m_listWidget->setViewMode(QListWidget::IconMode);
    m_listWidget->setDragEnabled(false);

    QListWidgetItem *item = new QListWidgetItem(icon(IconType::Info), "information");
    item->setData(Qt::UserRole, static_cast<int>(IconType::Info));
    m_listWidget->addItem(item);

    item = new QListWidgetItem(icon(IconType::Warning), "warning");
    item->setData(Qt::UserRole, static_cast<int>(IconType::Warning));
    m_listWidget->addItem(item);

    item = new QListWidgetItem(icon(IconType::Error), "error");
    item->setData(Qt::UserRole, static_cast<int>(IconType::Error));
    m_listWidget->addItem(item);

    item = new QListWidgetItem(icon(IconType::Bug), "bug");
    item->setData(Qt::UserRole, static_cast<int>(IconType::Bug));
    m_listWidget->addItem(item);

    item = new QListWidgetItem(icon(IconType::Todo), "todo");
    item->setData(Qt::UserRole, static_cast<int>(IconType::Todo));
    m_listWidget->addItem(item);

    for (int i = 0; i < m_listWidget->count(); ++i) {
        item = m_listWidget->item(i);
        if (static_cast<IconType>(item->data(Qt::UserRole).toInt()) == selectedIcon) {
            m_listWidget->setCurrentItem(item);
            break;
        }
    }
}

void KeywordDialog::setupColorWidgets(const QColor &color)
{
    m_colorButton->setColor(color);
    m_colorEdit->setText(color.name());
    connect(m_colorButton, &Utils::QtColorButton::colorChanged, this, &KeywordDialog::colorSelected);
}

bool KeywordDialog::canAccept()
{
    if (!isKeywordNameCorrect()) {
        showError(Tr::tr("Keyword cannot be empty, contain spaces, colons, slashes or asterisks."));
        return false;
    }

    if (isKeywordNameAlreadyUsed()) {
        showError(Tr::tr("There is already a keyword with this name."));
        return false;
    }

    return true;
}

bool KeywordDialog::isKeywordNameCorrect()
{
    // Make sure keyword is not empty and contains no spaces or colons

    QString name = keywordName();

    if (name.isEmpty())
        return false;

    for (const QChar i : name)
        if (LineParser::isKeywordSeparator(i))
            return false;

    return true;
}

bool KeywordDialog::isKeywordNameAlreadyUsed()
{
    return m_alreadyUsedKeywordNames.contains(keywordName());
}

void KeywordDialog::showError(const QString &text)
{
    m_errorLabel->setText(text);
    m_errorLabel->show();
}

QString KeywordDialog::keywordName()
{
    return m_keywordNameEdit->text().trimmed();
}

} // namespace Internal
} // namespace Todo
