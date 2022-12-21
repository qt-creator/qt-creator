// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QCompleter;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace Core { class IFindFilter; }
namespace Utils { class FancyLineEdit; }

namespace Core::Internal {

class FindToolWindow : public QWidget
{
    Q_OBJECT

public:
    explicit FindToolWindow(QWidget *parent = nullptr);
    ~FindToolWindow() override;
    static FindToolWindow *instance();

    void setFindFilters(const QList<IFindFilter *> &filters);
    QList<IFindFilter *> findFilters() const;

    void setFindText(const QString &text);
    void setCurrentFilter(IFindFilter *filter);
    void readSettings();
    void writeSettings();

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void search();
    void replace();
    void setCurrentFilterIndex(int index);
    void updateButtonStates();
    void updateFindFlags();
    void updateFindFilterName(IFindFilter *filter);
    static void findCompleterActivated(const QModelIndex &index);

    void acceptAndGetParameters(QString *term, IFindFilter **filter);

    QList<IFindFilter *> m_filters;
    QCompleter *m_findCompleter;
    QWidgetList m_configWidgets;
    IFindFilter *m_currentFilter;
    QWidget *m_configWidget;

    QWidget *m_uiConfigWidget;
    QPushButton *m_searchButton;
    QPushButton *m_replaceButton;
    QLabel *m_searchLabel;
    QComboBox *m_filterList;
    QWidget *m_optionsWidget;
    QCheckBox *m_matchCase;
    QCheckBox *m_wholeWords;
    QCheckBox *m_regExp;
    Utils::FancyLineEdit *m_searchTerm;
};

} // Core::Internal
