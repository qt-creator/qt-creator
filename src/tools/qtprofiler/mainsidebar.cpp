// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mainsidebar.h"

#include <profiler/profilertr.h>

#include <tracing/timelineformatdata.h>

#include <utils/filepath.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/widgets.h>

#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

using namespace Profiler;
using namespace Utils;
using namespace Utils::StyleHelper;
using namespace std::chrono;

namespace QtProfiler {

enum Role {
    FilePathRole = Qt::UserRole,
    DurationRole,
    FormatRole,
};

static QString formatDisplayName(Format format)
{
    switch (format) {
    case Format::Qml: return Tr::tr("QML Trace");
    case Format::Ctf: return Tr::tr("Common Trace Format");
    case Format::Sampler: return Tr::tr("Sampler Trace");
    case Format::Combined: return Tr::tr("Combined Trace");
    }
    Q_UNREACHABLE_RETURN({});
}

static QString captionText(const QModelIndex &index)
{
    QStringList parts;
    const QVariant formatV = index.data(FormatRole);
    if (formatV.isValid())
        parts << formatDisplayName(static_cast<Format>(formatV.toInt()));
    const QVariant durationV = index.data(DurationRole);
    if (durationV.isValid()) {
        const nanoseconds ns = duration_cast<nanoseconds>(milliseconds(durationV.toLongLong()));
        if (ns.count() > 0)
            parts << Timeline::formatTime(ns.count());
    }
    return parts.join(QStringLiteral(", "));
}

class TraceItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();

        const QRect bgR = option.rect.adjusted(SpacingTokens::PaddingHL, 0,
                                               -SpacingTokens::PaddingHL, 0);
        if (option.state & (QStyle::State_Selected | QStyle::State_MouseOver))
            StyleHelper::drawCardBg(painter, bgR, creatorColor(Theme::Token_Foreground_Subtle));

        const QRect textR = bgR.adjusted(SpacingTokens::PaddingHM, SpacingTokens::PaddingVXs,
                                         -SpacingTokens::PaddingHM, -SpacingTokens::PaddingVXs);

        const TextFormat &activeTitleTf = option.state & QStyle::State_Selected ? titleSelectedTf
                                                                               : titleTf;
        const int titleHeight = activeTitleTf.lineHeight();
        const QRect titleR(textR.left(), textR.top(), textR.width(), titleHeight);
        painter->setFont(activeTitleTf.font());
        painter->setPen(activeTitleTf.color());
        const QString titleElided = painter->fontMetrics().elidedText(
            index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textR.width());
        painter->drawText(titleR, activeTitleTf.drawTextFlags, titleElided);

        const QRect captionR(textR.left(), textR.top() + titleHeight + SpacingTokens::GapVXs,
                             textR.width(), captionTf.lineHeight());
        painter->setFont(captionTf.font());
        painter->setPen(captionTf.color());
        const QString captionElided = painter->fontMetrics().elidedText(
            captionText(index), Qt::ElideRight, captionR.width());
        painter->drawText(captionR, captionTf.drawTextFlags, captionElided);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        const int height =
            SpacingTokens::PaddingVXs
            + titleTf.lineHeight()
            + SpacingTokens::GapVXs
            + captionTf.lineHeight() * 1.25 // Hack: reserve more space
            + SpacingTokens::PaddingVXs;
        return {0, height};
    }

private:
    constexpr static TextFormat titleTf {
        .themeColor = Theme::Token_Text_Muted,
        .uiElement = UiElementBody2,
        .drawTextFlags = Qt::AlignVCenter | Qt::TextDontClip,
    };
    constexpr static TextFormat titleSelectedTf {
        .themeColor = Theme::Token_Text_Default,
        .uiElement = titleTf.uiElement,
        .drawTextFlags = titleTf.drawTextFlags,
    };
    constexpr static TextFormat captionTf {
        .themeColor = Theme::Token_Text_Muted,
        .uiElement = UiElementCaption,
        .drawTextFlags = titleTf.drawTextFlags,
    };
};

MainSidebar::MainSidebar(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundColor(this, Theme::Token_Background_Muted);

    auto titleBar = new Utils::StyledBar;
    auto titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(SpacingTokens::PaddingHXs, 0, SpacingTokens::PaddingHXs, 0);
    titleBarLayout->addWidget(new QLabel(Tr::tr("Traces")));

    m_list = new QListWidget;
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setItemDelegate(new TraceItemDelegate(m_list));
    m_list->setMouseTracking(true);
    m_list->setSpacing(SpacingTokens::GapVXs);
    m_list->viewport()->setAutoFillBackground(false);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(SpacingTokens::GapHM);
    layout->addWidget(titleBar);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        if (current)
            emit traceActivated(FilePath::fromVariant(current->data(FilePathRole)));
    });
}

void MainSidebar::addTrace(const FilePath &filePath)
{
    const QVariant filePathV = filePath.toVariant();

    // Programmatic selection must not trigger a reload, so block currentItemChanged.
    const QSignalBlocker blocker(m_list);

    if (QListWidgetItem *item = MainSidebar::traceItem(filePath)) {
        m_list->setCurrentItem(item);
        return;
    }

    auto item = new QListWidgetItem(filePath.fileName());
    item->setToolTip(filePath.toUserOutput());
    item->setData(FilePathRole, filePathV);
    m_list->addItem(item);
    m_list->setCurrentItem(item);
}

void MainSidebar::setTraceFormat(const FilePath &filePath, Format format)
{
    if (QListWidgetItem *item = traceItem(filePath))
        item->setData(FormatRole, static_cast<int>(format));
}

void MainSidebar::setTraceDuration(const FilePath &filePath, std::chrono::milliseconds ms)
{
    if (ms.count() <= 0)
        return; // Avoid temporarily resetting it to zero on reload
    if (QListWidgetItem *item = traceItem(filePath))
        item->setData(DurationRole, qint64(ms.count()));
}

bool MainSidebar::removeCurrentTrace()
{
    QListWidgetItem *current = m_list->currentItem();
    if (!current)
        return false;

    // takeItem() drops the row and selects a neighbour, emitting currentItemChanged
    // (and thus traceActivated) for the new selection, or nullptr if none remain.
    delete m_list->takeItem(m_list->row(current));
    return m_list->currentItem() != nullptr;
}

QListWidgetItem *MainSidebar::traceItem(const Utils::FilePath &filePath) const
{
    const QVariant filePathV = filePath.toVariant();

    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        if (item->data(FilePathRole) == filePathV)
            return item;
    }

    return {};
}

FilePath MainSidebar::currentTrace() const
{
    if (QListWidgetItem *item = m_list->currentItem())
        return FilePath::fromVariant(item->data(FilePathRole));
    return {};
}

} // namespace QtProfiler
