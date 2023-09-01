// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "errorlistmodel.h"
#include "error.h"
#include "frame.h"
#include "stack.h"
#include "../valgrindtr.h"

#include <debugger/analyzer/diagnosticlocation.h>

#include <utils/qtcassert.h>

#include <QDir>
#include <QList>

namespace Valgrind::XmlProtocol {

class ErrorItem : public Utils::TreeItem
{
public:
    ErrorItem(const ErrorListModel *model, const Error &error);

    const ErrorListModel *modelPrivate() const { return m_model; }
    Error error() const { return m_error; }

private:
    QVariant data(int column, int role) const override;

    const ErrorListModel * const m_model;
    const Error m_error;
};

class StackItem : public Utils::TreeItem
{
public:
    StackItem(const Stack &stack);

private:
    QVariant data(int column, int role) const override;

    const ErrorItem *getErrorItem() const;

    const Stack m_stack;
};

class FrameItem : public Utils::TreeItem
{
public:
    FrameItem(const Frame &frame);

private:
    QVariant data(int column, int role) const override;

    const ErrorItem *getErrorItem() const;

    const Frame m_frame;
};


ErrorListModel::ErrorListModel(QObject *parent)
    : Utils::TreeModel<>(parent)
{
    setHeader({Tr::tr("Issue"), Tr::tr("Location")});
}

Frame ErrorListModel::findRelevantFrame(const Error &error) const
{
    if (m_relevantFrameFinder)
        return m_relevantFrameFinder(error);
    const QList<Stack> stacks = error.stacks();
    if (stacks.isEmpty())
        return Frame();
    const Stack &stack = stacks[0];
    const QList<Frame> frames = stack.frames();
    if (!frames.isEmpty())
        return frames.first();
    return Frame();
}

static QString makeFrameName(const Frame &frame, bool withLocation)
{
    const QString d = frame.directory();
    const QString f = frame.fileName();
    const QString fn = frame.functionName();
    const QString fullPath = frame.filePath();

    QString path;
    if (!d.isEmpty() && !f.isEmpty())
        path = fullPath;
    else
        path = frame.object();

    if (QFileInfo::exists(path))
        path = QFileInfo(path).canonicalFilePath();

    if (frame.line() != -1)
        path += ':' + QString::number(frame.line());

    if (!fn.isEmpty()) {
        const QString location = withLocation || path == frame.object()
                ? QString::fromLatin1(" in %2").arg(path) : QString();
        return Tr::tr("%1%2").arg(fn, location);
    }
    if (!path.isEmpty())
        return path;
    return QString::fromLatin1("0x%1").arg(frame.instructionPointer(), 0, 16);
}

QString ErrorListModel::errorLocation(const Error &error) const
{
    return Tr::tr("in %1").arg(makeFrameName(findRelevantFrame(error), true));
}

void ErrorListModel::addError(const Error &error)
{
    rootItem()->appendChild(new ErrorItem(this, error));
}

ErrorListModel::RelevantFrameFinder ErrorListModel::relevantFrameFinder() const
{
    return m_relevantFrameFinder;
}

void ErrorListModel::setRelevantFrameFinder(const RelevantFrameFinder &relevantFrameFinder)
{
    m_relevantFrameFinder = relevantFrameFinder;
}


ErrorItem::ErrorItem(const ErrorListModel *model, const Error &error)
    : m_model(model), m_error(error)
{
    QTC_ASSERT(!m_error.stacks().isEmpty(), return);

    // If there's more than one stack, we simply map the real tree structure.
    // Otherwise, we skip the stack level, which has no useful information and would
    // just annoy the user.
    // The same goes for the frame level.
    if (m_error.stacks().count() > 1) {
        const QList<Stack> stacks = m_error.stacks();
        for (const Stack &s : stacks)
            appendChild(new StackItem(s));
    } else if (m_error.stacks().constFirst().frames().count() > 1) {
        const QList<Frame> frames = m_error.stacks().constFirst().frames();
        for (const Frame &f : frames)
            appendChild(new FrameItem(f));
    }
}

static QVariant locationData(int role, const Frame &frame)
{
    const Debugger::DiagnosticLocation location(Utils::FilePath::fromString(frame.filePath()),
                                                frame.line(),
                                                0);
    return Debugger::DetailedErrorView::locationData(role, location);
}

QVariant ErrorItem::data(int column, int role) const
{
    if (column == Debugger::DetailedErrorView::LocationColumn)
        return locationData(role, m_model->findRelevantFrame(m_error));

    // DiagnosticColumn
    switch (role) {
    case Debugger::DetailedErrorView::FullTextRole: {
        QString content;
        QTextStream stream(&content);

        stream << m_error.what() << "\n";
        stream << "  "
               << m_model->errorLocation(m_error)
               << "\n";

        const QList<Stack> stacks = m_error.stacks();
        for (const Stack &stack : stacks) {
            if (!stack.auxWhat().isEmpty())
                stream << stack.auxWhat();
            int i = 1;
            const QList<Frame> frames = stack.frames();
            for (const Frame &frame : frames)
                stream << "  " << i++ << ": " << makeFrameName(frame, true) << "\n";
        }

        stream.flush();
        return content;
    }
    case ErrorListModel::ErrorRole:
        return QVariant::fromValue<Error>(m_error);
    case Qt::DisplayRole:
        // If and only if there is exactly one frame, we have omitted creating a child item for it
        // (see the constructor) and display the function name in the error item instead.
        if (m_error.stacks().count() != 1 || m_error.stacks().constFirst().frames().count() != 1
                || m_error.stacks().constFirst().frames().constFirst().functionName().isEmpty()) {
            return m_error.what();
        }
        return Tr::tr("%1 in function %2")
                .arg(m_error.what(), m_error.stacks().constFirst().frames().constFirst().functionName());
    case Qt::ToolTipRole:
        return m_model->findRelevantFrame(m_error).toolTip();
    default:
        return QVariant();
    }
}


StackItem::StackItem(const Stack &stack) : m_stack(stack)
{
    const QList<Frame> frames = m_stack.frames();
    for (const Frame &f : frames)
        appendChild(new FrameItem(f));
}

QVariant StackItem::data(int column, int role) const
{
    const ErrorItem * const errorItem = getErrorItem();
    if (column == Debugger::DetailedErrorView::LocationColumn)
        return locationData(role, errorItem->modelPrivate()->findRelevantFrame(errorItem->error()));

    // DiagnosticColumn
    switch (role) {
    case ErrorListModel::ErrorRole:
        return QVariant::fromValue(errorItem->error());
    case Qt::DisplayRole:
        return m_stack.auxWhat().isEmpty() ? errorItem->error().what() : m_stack.auxWhat();
    case Qt::ToolTipRole:
        return errorItem->modelPrivate()->findRelevantFrame(errorItem->error()).toolTip();
    default:
        return QVariant();
    }
}

const ErrorItem *StackItem::getErrorItem() const
{
    return static_cast<ErrorItem *>(parent());
}


FrameItem::FrameItem(const Frame &frame) : m_frame(frame)
{
}

QVariant FrameItem::data(int column, int role) const
{
    if (column == Debugger::DetailedErrorView::LocationColumn)
        return locationData(role, m_frame);

    // DiagnosticColumn
    switch (role) {
    case ErrorListModel::ErrorRole:
        return QVariant::fromValue(getErrorItem()->error());
    case Qt::DisplayRole: {
        const int row = indexInParent() + 1;
        const int padding = static_cast<int>(std::log10(parent()->childCount()))
                - static_cast<int>(std::log10(row));
        return QString::fromLatin1("%1%2: %3")
                .arg(QString(padding, ' '))
                .arg(row)
                .arg(makeFrameName(m_frame, false));
    }
    case Qt::ToolTipRole:
        return m_frame.toolTip();
    default:
        return QVariant();
    }
}

const ErrorItem *FrameItem::getErrorItem() const
{
    for (const TreeItem *parentItem = parent(); parentItem; parentItem = parentItem->parent()) {
        auto const errorItem = dynamic_cast<const ErrorItem *>(parentItem);
        if (errorItem)
            return errorItem;
    }
    QTC_CHECK(false);
    return nullptr;
}

} // namespace Valgrind::XmlProtocol
