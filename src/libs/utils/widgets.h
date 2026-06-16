// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "utils_global.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCursor>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTabBar>
#include <QToolButton>

QT_BEGIN_NAMESPACE
class QGraphicsOpacityEffect;
class QHeaderView;
class QMenu;
class QMouseEvent;
class QProgressDialog;
class QTimer;
QT_END_NAMESPACE

namespace Utils {
class FilePath;

class QTCREATOR_UTILS_EXPORT FadingPanel : public QWidget
{
public:
    using QWidget::QWidget;

    virtual void fadeTo(qreal value) = 0;
    virtual void setOpacity(qreal value) = 0;
};

class QTCREATOR_UTILS_EXPORT FadingWidget : public FadingPanel
{
public:
    FadingWidget(QWidget *parent = nullptr);
    void fadeTo(qreal value) override;
    qreal opacity();
    void setOpacity(qreal value) override;
protected:
    QGraphicsOpacityEffect * const m_opacityEffect;
};

class QTCREATOR_UTILS_EXPORT ExpandButton : public QToolButton
{
public:
    ExpandButton(QWidget *parent = nullptr);
};

class QTCREATOR_UTILS_EXPORT DetailsButton : public ExpandButton
{
public:
    DetailsButton(QWidget *parent = nullptr);
    QSize sizeHint() const override;
    static QColor outlineColor();

private:
    void paintEvent(QPaintEvent *e) override;
    const int spacing = 6;
};

class QTCREATOR_UTILS_EXPORT DocumentTabBar : public QTabBar
{
public:
    using QTabBar::QTabBar;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};

class QTCREATOR_UTILS_EXPORT FakeToolTip : public QWidget
{
public:
    explicit FakeToolTip(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
};

class QTCREATOR_UTILS_EXPORT FileCrumbLabel : public QLabel
{
    Q_OBJECT
public:
    FileCrumbLabel(QWidget *parent = nullptr);

    void setPath(const FilePath &path);

signals:
    void pathClicked(const FilePath &path);
};

class QTCREATOR_UTILS_EXPORT HeaderViewStretcher : public QObject
{
    const int m_columnToStretch;
public:
    explicit HeaderViewStretcher(QHeaderView *headerView, int columnToStretch);

    void stretch();
    void softStretch();
    bool eventFilter(QObject *obj, QEvent *ev) override;
};

class QTCREATOR_UTILS_EXPORT OptionPushButton : public QPushButton
{
public:
    OptionPushButton(QWidget *parent = nullptr);
    OptionPushButton(const QString &text, QWidget *parent = nullptr);

    void setOptionalMenu(QMenu *menu);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};

class QTCREATOR_UTILS_EXPORT OverrideCursor
{
public:
    OverrideCursor(const QCursor &cursor);
    ~OverrideCursor();
    void set();
    void reset();
private:
    bool m_set = true;
    QCursor m_cursor;
};

class QTCREATOR_UTILS_EXPORT RemoveFileDialog : public QDialog
{
public:
    explicit RemoveFileDialog(const FilePath &filePath);

    void setDeleteFileVisible(bool visible);
    bool isDeleteFileChecked() const;

private:
    QCheckBox *m_deleteFileCheckBox;
};

class QTCREATOR_UTILS_EXPORT StatusLabel : public QLabel
{
public:
    explicit StatusLabel(QWidget *parent = nullptr);

    void showStatusMessage(const QString &message, int timeoutMS = 5000);
    void clearStatusMessage();

private:
    void slotTimeout();
    void stopTimer();

    QTimer *m_timer = nullptr;
    QString m_lastPermanentStatusMessage;
};

class QTCREATOR_UTILS_EXPORT StyledBar : public QWidget
{
public:
    StyledBar(QWidget *parent = nullptr);
    void setSingleRow(bool singleRow);
    bool isSingleRow() const;

    void setLightColored(bool lightColored);
    bool isLightColored() const;

protected:
    void paintEvent(QPaintEvent *event) override;
};

class QTCREATOR_UTILS_EXPORT StyledSeparator : public QWidget
{
public:
    StyledSeparator(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

// Documentation inside.
class QTCREATOR_UTILS_EXPORT TextFieldCheckBox : public QCheckBox
{
    Q_PROPERTY(QString compareText READ text WRITE setText)
    Q_PROPERTY(QString trueText READ trueText WRITE setTrueText)
    Q_PROPERTY(QString falseText READ falseText WRITE setFalseText)
    Q_OBJECT

public:
    explicit TextFieldCheckBox(const QString &text, QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &s);

    void setTrueText(const QString &t)  { m_trueText = t;    }
    QString trueText() const            { return m_trueText; }
    void setFalseText(const QString &t) { m_falseText = t; }
    QString falseText() const              { return m_falseText; }

signals:
    void textChanged(const QString &);

private:
    void slotStateChanged(int);

    QString m_trueText;
    QString m_falseText;
};

// Documentation inside.
class QTCREATOR_UTILS_EXPORT TextFieldComboBox : public QComboBox
{
    Q_PROPERTY(QString indexText READ text WRITE setText)
    Q_OBJECT

public:
    explicit TextFieldComboBox(QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &s);

    void setItems(const QStringList &displayTexts, const QStringList &values);

signals:
    void text4Changed(const QString &); // Do not conflict with Qt 3 compat signal.

private:
    void slotCurrentIndexChanged(int);

    inline QString valueAt(int) const;
};

QTCREATOR_UTILS_EXPORT QAction *execMenuAtWidget(QMenu *menu, QWidget *widget);
QTCREATOR_UTILS_EXPORT void addToolTipsToMenu(QMenu *menu);

QTCREATOR_UTILS_EXPORT QProgressDialog *createProgressDialog(
    int maxValue, const QString &windowTitle, const QString &labelText);

namespace AsynchronousMessageBox {
QTCREATOR_UTILS_EXPORT QWidget *warning(const QString &title, const QString &desciption);
QTCREATOR_UTILS_EXPORT QWidget *information(const QString &title, const QString &desciption);
QTCREATOR_UTILS_EXPORT QWidget *critical(const QString &title, const QString &desciption);
}

} // namespace Utils
