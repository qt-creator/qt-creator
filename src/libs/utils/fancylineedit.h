// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "result.h"
#include "storekey.h"

#include <QAbstractButton>
#include <QFuture>
#include <QLineEdit>

#include <functional>

QT_BEGIN_NAMESPACE
class QEvent;
class QKeySequence;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT CompletingLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    using QLineEdit::QLineEdit;

protected:
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
};

class FancyLineEditPrivate;

class QTCREATOR_UTILS_EXPORT FancyLineEdit : public CompletingLineEdit
{
    Q_OBJECT
public:
    enum Side {Left = 0, Right = 1};
    Q_ENUM(Side)

    explicit FancyLineEdit(QWidget *parent = nullptr);
    ~FancyLineEdit() override;

    void setTextKeepingActiveCursor(const QString &text);

    QIcon buttonIcon(Side side) const;
    void setButtonIcon(Side side, const QIcon &icon);

    QMenu *buttonMenu(Side side) const;
    void setButtonMenu(Side side, QMenu *menu);

    void setButtonVisible(Side side, bool visible);
    bool isButtonVisible(Side side) const;
    QAbstractButton *button(Side side) const;

    void setButtonToolTip(Side side, const QString &);
    void setButtonFocusPolicy(Side side, Qt::FocusPolicy policy);

    // Set whether tabbing in will trigger the menu.
    void setMenuTabFocusTrigger(Side side, bool v);
    bool hasMenuTabFocusTrigger(Side side) const;

    // Set if icon should be hidden when text is empty
    void setAutoHideButton(Side side, bool h);
    bool hasAutoHideButton(Side side) const;


    // Completion

    // Enable a history completer with a history of entries.
    void setHistoryCompleter(
        const Utils::Key &historyKey, bool restoreLastItemFromHistory = false, int maxLines = 6);
    // Sets a completer that is not a history completer.
    void setSpecialCompleter(QCompleter *completer);


    // Filtering

    // Enables filtering
    void setFiltering(bool on);


    //  Validation

    // line edit, (out)errorMessage -> valid?
    using AsyncValidationResult = Result<QString>;
    using AsyncValidationFuture = QFuture<AsyncValidationResult>;
    using AsyncValidationFunction = std::function<AsyncValidationFuture(QString)>;
    using SynchronousValidationFunction = std::function<Result<>(FancyLineEdit &)>;
    using SimpleSynchronousValidationFunction = std::function<Result<>(const QString &)>;
    using ValidationFunction = std::variant<
        AsyncValidationFunction,
        SynchronousValidationFunction,
        SimpleSynchronousValidationFunction
    >;

    enum State { Invalid, DisplayingPlaceholderText, Valid };

    State state() const;
    bool isValid() const;
    QString errorMessage() const;

    void setValidatePlaceHolder(bool on);

    void setValidationFunction(const ValidationFunction &fn);
    static ValidationFunction defaultValidationFunction();
    void validate();
    void onEditingFinished();

    static void setCamelCaseNavigationEnabled(bool enabled);
    static void setCompletionShortcut(const QKeySequence &shortcut);

    void setValueAlternatives(const QStringList &values);

protected:
    // Custom behaviour can be added here.
    virtual void handleChanged(const QString &) {}
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void buttonClicked(FancyLineEdit::Side side);
    void leftButtonClicked();
    void rightButtonClicked();

    void filterChanged(const QString &);

    void validChanged(bool validState);
    void validReturnPressed();

protected:
    void resizeEvent(QResizeEvent *e) override;

    virtual QString fixInputString(const QString &string);

private:
    void iconClicked(FancyLineEdit::Side);

    void handleValidationResult(AsyncValidationResult result, const QString &oldText);

    static Result<> validateWithValidator(FancyLineEdit &edit);
    // Unimplemented, to force the user to make a decision on
    // whether to use setHistoryCompleter() or setSpecialCompleter().
    void setCompleter(QCompleter *);

    void updateMargins();
    void updateButtonPositions();
    bool camelCaseBackward(bool mark);
    bool camelCaseForward(bool mark);
    friend class FancyLineEditPrivate;

    FancyLineEditPrivate *d;
};

struct ClassNameValidatingLineEditPrivate;

class QTCREATOR_UTILS_EXPORT ClassNameValidatingLineEdit : public FancyLineEdit
{
    Q_OBJECT
    Q_PROPERTY(bool namespacesEnabled READ namespacesEnabled WRITE setNamespacesEnabled DESIGNABLE true)
    Q_PROPERTY(bool lowerCaseFileName READ lowerCaseFileName WRITE setLowerCaseFileName)

public:
    explicit ClassNameValidatingLineEdit(QWidget *parent = nullptr);
    ~ClassNameValidatingLineEdit() override;

    bool namespacesEnabled() const;
    void setNamespacesEnabled(bool b);

    QString namespaceDelimiter();
    void setNamespaceDelimiter(const QString &delimiter);

    bool lowerCaseFileName() const;
    void setLowerCaseFileName(bool v);

    bool forceFirstCapitalLetter() const;
    void setForceFirstCapitalLetter(bool b);

    // Clean an input string to get a valid class name.
    static QString createClassName(const QString &name);

signals:
    // Will be emitted with a suggestion for a base name of the
    // source/header file of the class.
    void updateFileName(const QString &t);

protected:
    Result<> validateClassName(const QString &text) const;
    void handleChanged(const QString &t) override;
    QString fixInputString(const QString &string) override;

private:
    void updateRegExp() const;

    ClassNameValidatingLineEditPrivate *d;
};

class QTCREATOR_UTILS_EXPORT FileNameValidatingLineEdit : public FancyLineEdit
{
    Q_OBJECT
    Q_PROPERTY(bool allowDirectories READ allowDirectories WRITE setAllowDirectories)
    Q_PROPERTY(QStringList requiredExtensions READ requiredExtensions WRITE setRequiredExtensions)
    Q_PROPERTY(bool forceFirstCapitalLetter READ forceFirstCapitalLetter WRITE setForceFirstCapitalLetter)

public:
    explicit FileNameValidatingLineEdit(QWidget *parent = nullptr);

    static Result<> validateFileName(const QString &name, bool allowDirectories = false);

    static Result<> validateFileNameExtension(const QString &name,
                                              const QStringList &requiredExtensions = {});

    /**
     * Sets whether entering directories is allowed. This will enable the user
     * to enter slashes in the filename. Default is off.
     */
    bool allowDirectories() const;
    void setAllowDirectories(bool v);

    /**
     * Sets whether the first letter is forced to be a capital letter
     * Default is off.
     */
    bool forceFirstCapitalLetter() const;
    void setForceFirstCapitalLetter(bool b);

    /**
     * Sets a requred extension. If the extension is empty no extension is required.
     * Default is empty.
     */
    QStringList requiredExtensions() const;
    void setRequiredExtensions(const QStringList &extensionList);

protected:
    QString fixInputString(const QString &string) override;

private:
    bool m_allowDirectories;
    QStringList m_requiredExtensionList;
    bool m_forceFirstCapitalLetter;
};

} // namespace Utils
