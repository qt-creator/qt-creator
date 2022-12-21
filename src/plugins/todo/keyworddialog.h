// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>
#include <QSet>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QListWidget;
QT_END_NAMESPACE

namespace Utils { class QtColorButton; }

namespace Todo {
namespace Internal {

class Keyword;
enum class IconType;

class KeywordDialog : public QDialog
{
    Q_OBJECT
public:
    KeywordDialog(const Keyword &keyword, const QSet<QString> &alreadyUsedKeywordNames,
                  QWidget *parent = nullptr);
    ~KeywordDialog() override;

    Keyword keyword();

private:
    void colorSelected(const QColor &color);
    void acceptButtonClicked();
    void setupListWidget(IconType selectedIcon);
    void setupColorWidgets(const QColor &color);
    bool canAccept();
    bool isKeywordNameCorrect();
    bool isKeywordNameAlreadyUsed();
    void showError(const QString &text);
    QString keywordName();

    QSet<QString> m_alreadyUsedKeywordNames;

    QListWidget *m_listWidget;
    QLineEdit *m_colorEdit;
    Utils::QtColorButton *m_colorButton;
    QLineEdit *m_keywordNameEdit;
    QLabel *m_errorLabel;
    QDialogButtonBox *m_buttonBox;
};

} // namespace Internal
} // namespace Todo
