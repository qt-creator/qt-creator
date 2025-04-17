// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findtoolwindow.h"

#include "ifindfilter.h"
#include "findplugin.h"
#include "../coreplugintr.h"
#include "../icore.h"

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStringListModel>

using namespace Utils;

namespace Core::Internal {

static FindToolWindow *m_instance = nullptr;

static Result<> validateRegExp(FancyLineEdit *edit)
{
    if (edit->text().isEmpty())
        return ResultError(Tr::tr("Empty search term."));

    if (Find::hasFindFlag(FindRegularExpression)) {
        QRegularExpression regexp(edit->text());
        if (!regexp.isValid())
            return ResultError(regexp.errorString());
    }

    return ResultOk;
}

static bool isChildOf(QWidget *parent, QWidget *child)
{
    QWidget *w = child;
    while (w) {
        if (w == parent)
            return true;
        w = w->parentWidget();
    }
    return false;
}

static QList<QWidget *> tabChain(QWidget *parent, QWidget *start, int limit = 50)
{
    int guard = 0;
    QWidget *w = start;
    QList<QWidget *> result;
    QSet<QWidget *> seen;
    while (w && isChildOf(parent, w) && guard < limit) {
        if (!Utils::insert(seen, w))
            break;
        // setTabOrder does nothing if any widget has NoFocus, so filter out
        if (w->focusPolicy() != Qt::NoFocus)
            result.append(w);
        w = w->nextInFocusChain();
        ++guard;
    }
    return result;
}

static QWidget *lastInTabChain(QWidget *parent, int limit = 50)
{
    const QList<QWidget *> chain = tabChain(parent, parent, limit);
    return chain.isEmpty() ? nullptr : chain.constLast();
}

FindToolWindow::FindToolWindow(QWidget *parent)
    : QWidget(parent)
    , m_findCompleter(new QCompleter(this))
    , m_currentFilter(nullptr)
    , m_configWidget(nullptr)
{
    m_instance = this;

    m_searchButton = new QPushButton;
    m_searchButton->setText(Tr::tr("&Search", nullptr));
    m_searchButton->setDefault(true);

    m_replaceButton = new QPushButton;
    m_replaceButton->setText(Tr::tr("Search && &Replace", nullptr));

    m_searchTerm = new FancyLineEdit;
    m_searchTerm->setFiltering(true);
    m_searchTerm->setPlaceholderText({});

    m_searchLabel = new QLabel;
    m_searchLabel->setText(Tr::tr("Search f&or:", nullptr));
    m_searchLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
    m_searchLabel->setBuddy(m_searchTerm);

    m_filterList = new QComboBox;
    m_filterList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_filterList->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    m_optionsWidget = new QWidget;

    m_matchCase = new QCheckBox;
    m_matchCase->setText(Tr::tr("&Case sensitive", nullptr));

    m_wholeWords = new QCheckBox;
    m_wholeWords->setText(Tr::tr("Whole words o&nly", nullptr));

    m_ignoreBinaryFiles = new QCheckBox;
    m_ignoreBinaryFiles->setText(Tr::tr("Ignore binary files", nullptr));

    m_regExp = new QCheckBox;
    m_regExp->setText(Tr::tr("Use re&gular expressions", nullptr));

    auto label = new QLabel;
    label->setText(Tr::tr("Sco&pe:", nullptr));
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    label->setMinimumSize(QSize(80, 0));
    label->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
    label->setBuddy(m_filterList);

    m_uiConfigWidget = new QWidget;
    QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(10);
    m_uiConfigWidget->setSizePolicy(sizePolicy2);
    m_uiConfigWidget->setMinimumSize(QSize(800, 0));

    setFocusProxy(m_searchTerm);

    using namespace Layouting;

    Row {
        m_matchCase,
        m_wholeWords,
        m_regExp,
        m_ignoreBinaryFiles,
        st,
        noMargin
    }.attachTo(m_optionsWidget);

    Grid {
        label, m_filterList, br,
        m_searchLabel, m_searchTerm, br,
        empty, m_optionsWidget, br,
        Span(2, m_uiConfigWidget), br,
        Span(2, Row { st, m_searchButton, m_replaceButton }), br,
    }.attachTo(this);

    layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(m_searchButton, &QAbstractButton::clicked, this, &FindToolWindow::search);
    connect(m_replaceButton, &QAbstractButton::clicked, this, &FindToolWindow::replace);
    connect(m_matchCase, &QAbstractButton::toggled, Find::instance(), &Find::setCaseSensitive);
    connect(m_wholeWords, &QAbstractButton::toggled, Find::instance(), &Find::setWholeWord);
    connect(m_ignoreBinaryFiles, &QAbstractButton::toggled, Find::instance(), &Find::setIgnoreBinaryFiles);
    connect(m_regExp, &QAbstractButton::toggled, Find::instance(), &Find::setRegularExpression);
    connect(m_filterList, &QComboBox::activated, this, &FindToolWindow::setCurrentFilterIndex);

    m_findCompleter->setModel(Find::findCompletionModel());
    m_searchTerm->setSpecialCompleter(m_findCompleter);
    m_searchTerm->installEventFilter(this);
    connect(m_findCompleter, QOverload<const QModelIndex &>::of(&QCompleter::activated),
            this, &FindToolWindow::findCompleterActivated);

    m_searchTerm->setValidationFunction(validateRegExp);
    connect(Find::instance(), &Find::findFlagsChanged,
            m_searchTerm, &Utils::FancyLineEdit::validate);
    connect(m_searchTerm, &Utils::FancyLineEdit::validChanged,
            this, &FindToolWindow::updateButtonStates);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_uiConfigWidget->setLayout(layout);
    updateButtonStates();

    connect(Find::instance(), &Find::findFlagsChanged, this, &FindToolWindow::updateFindFlags);
}

FindToolWindow::~FindToolWindow()
{
    qDeleteAll(m_configWidgets);
}

FindToolWindow *FindToolWindow::instance()
{
    return m_instance;
}

bool FindToolWindow::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto ke = static_cast<QKeyEvent *>(event);
        if ((ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
                && (ke->modifiers() == Qt::NoModifier || ke->modifiers() == Qt::KeypadModifier)) {
            ke->accept();
            if (m_searchButton->isEnabled())
                search();
            return true;
        }
    }
    return QWidget::event(event);
}

bool FindToolWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_searchTerm && event->type() == QEvent::KeyPress) {
        auto ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Down) {
            if (m_searchTerm->text().isEmpty())
                m_findCompleter->setCompletionPrefix(QString());
            m_findCompleter->complete();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void FindToolWindow::updateButtonStates()
{
    bool filterEnabled = m_currentFilter && m_currentFilter->isEnabled();
    bool enabled = filterEnabled && (!m_currentFilter->showSearchTermInput()
                                     || m_searchTerm->isValid()) && m_currentFilter->isValid();
    m_searchButton->setEnabled(enabled);
    m_replaceButton->setEnabled(m_currentFilter
                                   && m_currentFilter->isReplaceSupported() && enabled);
    if (m_configWidget)
        m_configWidget->setEnabled(filterEnabled);

    Utils::FindFlags supportedFlags = m_currentFilter ? m_currentFilter->supportedFindFlags()
                                                      : Utils::FindFlags();

    if (m_currentFilter) {
        m_searchTerm->setVisible(m_currentFilter->showSearchTermInput());
        m_searchLabel->setVisible(m_currentFilter->showSearchTermInput());
        m_optionsWidget->setVisible(
            supportedFlags
            & (FindCaseSensitively | FindWholeWords | FindRegularExpression | DontFindBinaryFiles));
    }

    m_matchCase->setEnabled(filterEnabled && (supportedFlags & FindCaseSensitively));
    m_wholeWords->setEnabled(filterEnabled && (supportedFlags & FindWholeWords));
    m_ignoreBinaryFiles->setEnabled(filterEnabled && (supportedFlags & DontFindBinaryFiles));

    m_regExp->setEnabled(filterEnabled && (supportedFlags & FindRegularExpression));
    m_searchTerm->setEnabled(filterEnabled);
}

void FindToolWindow::updateFindFlags()
{
    m_matchCase->setChecked(Find::hasFindFlag(FindCaseSensitively));
    m_wholeWords->setChecked(Find::hasFindFlag(FindWholeWords));
    m_regExp->setChecked(Find::hasFindFlag(FindRegularExpression));
    m_ignoreBinaryFiles->setChecked(Find::hasFindFlag(DontFindBinaryFiles));
}


void FindToolWindow::setFindFilters(const QList<IFindFilter *> &filters)
{
    qDeleteAll(m_configWidgets);
    m_configWidgets.clear();
    for (IFindFilter *filter : std::as_const(m_filters))
        filter->disconnect(this);
    m_filters = filters;
    m_filterList->clear();
    QStringList names;
    for (IFindFilter *filter : filters) {
        names << filter->displayName();
        m_configWidgets.append(filter->createConfigWidget());
        connect(filter, &IFindFilter::displayNameChanged,
                this, [this, filter] { updateFindFilterName(filter); });
    }
    m_filterList->addItems(names);
    if (m_filters.size() > 0)
        setCurrentFilterIndex(0);
}

QList<IFindFilter *> FindToolWindow::findFilters() const
{
    return m_filters;
}

void FindToolWindow::updateFindFilterName(IFindFilter *filter)
{
    int index = m_filters.indexOf(filter);
    if (QTC_GUARD(index >= 0))
        m_filterList->setItemText(index, filter->displayName());
}

void FindToolWindow::setFindText(const QString &text)
{
    m_searchTerm->setText(text);
}

void FindToolWindow::setCurrentFilter(IFindFilter *filter)
{
    if (!filter)
        filter = m_currentFilter;
    int index = m_filters.indexOf(filter);
    if (index >= 0)
        setCurrentFilterIndex(index);
    updateFindFlags();
    m_searchTerm->setFocus();
    m_searchTerm->selectAll();
}

void FindToolWindow::setCurrentFilterIndex(int index)
{
    m_filterList->setCurrentIndex(index);
    for (int i = 0; i < m_configWidgets.size(); ++i) {
        QWidget *configWidget = m_configWidgets.at(i);
        if (i == index) {
            m_configWidget = configWidget;
            if (m_currentFilter) {
                disconnect(m_currentFilter, &IFindFilter::enabledChanged,
                           this, &FindToolWindow::updateButtonStates);
                disconnect(m_currentFilter, &IFindFilter::validChanged,
                           this, &FindToolWindow::updateButtonStates);
                disconnect(m_currentFilter, &IFindFilter::supportedFlagsChanged,
                           this, &FindToolWindow::updateButtonStates);
            }
            m_currentFilter = m_filters.at(i);
            connect(m_currentFilter, &IFindFilter::enabledChanged,
                    this, &FindToolWindow::updateButtonStates);
            connect(m_currentFilter, &IFindFilter::validChanged,
                    this, &FindToolWindow::updateButtonStates);
            connect(m_currentFilter, &IFindFilter::supportedFlagsChanged,
                    this, &FindToolWindow::updateButtonStates);
            updateButtonStates();
            if (m_configWidget)
                m_uiConfigWidget->layout()->addWidget(m_configWidget);
        } else {
            if (configWidget)
                configWidget->setParent(nullptr);
        }
    }
    QWidget *w = m_uiConfigWidget;
    while (w) {
        auto sa = qobject_cast<QScrollArea *>(w);
        if (sa) {
            sa->updateGeometry();
            break;
        }
        w = w->parentWidget();
    }
    for (w = m_configWidget ? m_configWidget : m_uiConfigWidget; w; w = w->parentWidget()) {
        if (w->layout())
            w->layout()->activate();
    }
    // fix tab order
    if (m_configWidget) {
        const QList<QWidget *> configWidgetChain = tabChain(m_configWidget, m_configWidget);
        if (!configWidgetChain.isEmpty()) {
            QWidget::setTabOrder(lastInTabChain(m_optionsWidget), configWidgetChain.first());
            for (int i = 0; i < configWidgetChain.size() - 1; ++i)
                QWidget::setTabOrder(configWidgetChain.at(i), configWidgetChain.at(i + 1));
            QWidget::setTabOrder(lastInTabChain(m_configWidget), m_searchButton);
        }
    }
}

void FindToolWindow::acceptAndGetParameters(QString *term, IFindFilter **filter)
{
    QTC_ASSERT(filter, return);
    *filter = nullptr;
    Find::updateFindCompletion(m_searchTerm->text(), Find::findFlags());
    int index = m_filterList->currentIndex();
    QString searchTerm = m_searchTerm->text();
    if (index >= 0)
        *filter = m_filters.at(index);
    if (term)
        *term = searchTerm;
    if (searchTerm.isEmpty() && *filter && !(*filter)->isValid())
        *filter = nullptr;
}

void FindToolWindow::search()
{
    QString term;
    IFindFilter *filter = nullptr;
    acceptAndGetParameters(&term, &filter);
    QTC_ASSERT(filter, return);
    filter->findAll(term, Find::findFlags());
}

void FindToolWindow::replace()
{
    QString term;
    IFindFilter *filter = nullptr;
    acceptAndGetParameters(&term, &filter);
    QTC_ASSERT(filter, return);
    filter->replaceAll(term, Find::findFlags());
}

void FindToolWindow::restore(const Utils::Store &s)
{
    const QString currentFilter = s.value("CurrentFilter").toString();
    for (int i = 0; i < m_filters.size(); ++i) {
        IFindFilter *filter = m_filters.at(i);
        filter->restore(storeFromVariant(s.value(filter->id().toUtf8())));
        if (filter->id() == currentFilter)
            setCurrentFilterIndex(i);
    }
}

Store FindToolWindow::save() const
{
    Store s;
    if (m_currentFilter && (m_filters.isEmpty() || m_filters.first() != m_currentFilter))
        s.insert("CurrentFilter", m_currentFilter->id());
    for (IFindFilter *filter : std::as_const(m_filters)) {
        const Store store = filter->save();
        if (!store.isEmpty())
            s.insert(filter->id().toUtf8(), variantFromStore(store));
    }
    return s;
}

void FindToolWindow::writeSettings()
{
    Utils::QtcSettings *settings = ICore::settings();
    settings->beginGroup("Find");
    settings->setValueWithDefault("CurrentFilter",
                                  m_currentFilter ? m_currentFilter->id() : QString());
    for (IFindFilter *filter : std::as_const(m_filters))
        filter->writeSettings(settings);
    settings->endGroup();
}

void FindToolWindow::readSettings()
{
    QtcSettings *settings = ICore::settings();
    settings->beginGroup("Find");
    const QString currentFilter = settings->value("CurrentFilter").toString();
    for (int i = 0; i < m_filters.size(); ++i) {
        IFindFilter *filter = m_filters.at(i);
        filter->readSettings(settings);
        if (filter->id() == currentFilter)
            setCurrentFilterIndex(i);
    }
    settings->endGroup();
}

void FindToolWindow::findCompleterActivated(const QModelIndex &index)
{
    const int findFlagsI = index.data(Find::CompletionModelFindFlagsRole).toInt();
    const FindFlags findFlags(findFlagsI);
    Find::setCaseSensitive(findFlags.testFlag(FindCaseSensitively));
    Find::setBackward(findFlags.testFlag(FindBackward));
    Find::setWholeWord(findFlags.testFlag(FindWholeWords));
    Find::setRegularExpression(findFlags.testFlag(FindRegularExpression));
    Find::setPreserveCase(findFlags.testFlag(FindPreserveCase));
}

} // Core::Internal
