/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "errorlistmodel.h"
#include "error.h"
#include "frame.h"
#include "stack.h"
#include "modelhelpers.h"

#include <analyzerbase/diagnosticlocation.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QVector>

#include <cmath>

namespace Valgrind {
namespace XmlProtocol {

class ErrorListModelPrivate
{
public:
    QVariant errorData(const QModelIndex &index, int role) const;
    QSharedPointer<const ErrorListModel::RelevantFrameFinder> relevantFrameFinder;
    Frame findRelevantFrame(const Error &error) const;
    QString errorLocation(const Error &error) const;
};

class ErrorItem : public Utils::TreeItem
{
public:
    ErrorItem(const ErrorListModelPrivate *modelPrivate, const Error &error);

    const ErrorListModelPrivate *modelPrivate() const { return m_modelPrivate; }
    Error error() const { return m_error; }

private:
    QVariant data(int column, int role) const override;

    const ErrorListModelPrivate * const m_modelPrivate;
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
    : Utils::TreeModel(parent)
    , d(new ErrorListModelPrivate)
{
    setHeader(QStringList() << tr("Issue") << tr("Location"));
}

ErrorListModel::~ErrorListModel()
{
    delete d;
}

QSharedPointer<const ErrorListModel::RelevantFrameFinder> ErrorListModel::relevantFrameFinder() const
{
    return d->relevantFrameFinder;
}

void ErrorListModel::setRelevantFrameFinder(const QSharedPointer<const RelevantFrameFinder> &finder)
{
    d->relevantFrameFinder = finder;
}

Frame ErrorListModelPrivate::findRelevantFrame(const Error &error) const
{
    if (relevantFrameFinder)
        return relevantFrameFinder->findRelevant(error);
    const QVector<Stack> stacks = error.stacks();
    if (stacks.isEmpty())
        return Frame();
    const Stack &stack = stacks[0];
    const QVector<Frame> frames = stack.frames();
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

    if (QFile::exists(path))
        path = QFileInfo(path).canonicalFilePath();

    if (frame.line() != -1)
        path += QLatin1Char(':') + QString::number(frame.line());

    if (!fn.isEmpty()) {
        const QString location = withLocation || path == frame.object()
                ? QString::fromLatin1(" in %2").arg(path) : QString();
        return QCoreApplication::translate("Valgrind::Internal", "%1%2").arg(fn, location);
    }
    if (!path.isEmpty())
        return path;
    return QString::fromLatin1("0x%1").arg(frame.instructionPointer(), 0, 16);
}

QString ErrorListModelPrivate::errorLocation(const Error &error) const
{
    return QCoreApplication::translate("Valgrind::Internal", "in %1").
            arg(makeFrameName(findRelevantFrame(error), true));
}

void ErrorListModel::addError(const Error &error)
{
    rootItem()->appendChild(new ErrorItem(d, error));
}


ErrorItem::ErrorItem(const ErrorListModelPrivate *modelPrivate, const Error &error)
    : m_modelPrivate(modelPrivate), m_error(error)
{
    QTC_ASSERT(!m_error.stacks().isEmpty(), return);

    // If there's more than one stack, we simply map the real tree structure.
    // Otherwise, we skip the stack level, which has no useful information and would
    // just annoy the user.
    // The same goes for the frame level.
    if (m_error.stacks().count() > 1) {
        foreach (const Stack &s, m_error.stacks())
            appendChild(new StackItem(s));
    } else if (m_error.stacks().first().frames().count() > 1) {
        foreach (const Frame &f, m_error.stacks().first().frames())
            appendChild(new FrameItem(f));
    }
}

static QVariant location(const Frame &frame, int role)
{
    switch (role) {
    case Analyzer::DetailedErrorView::LocationRole:
        return QVariant::fromValue(Analyzer::DiagnosticLocation(frame.filePath(), frame.line(), 0));
    case Qt::ToolTipRole:
        return frame.filePath().isEmpty() ? QVariant() : QVariant(frame.filePath());
    default:
        return QVariant();
    }
}

QVariant ErrorItem::data(int column, int role) const
{
    if (column == Analyzer::DetailedErrorView::LocationColumn) {
        const Frame frame = m_modelPrivate->findRelevantFrame(m_error);
        return location(frame, role);
    }

    // DiagnosticColumn
    switch (role) {
    case Analyzer::DetailedErrorView::FullTextRole: {
        QString content;
        QTextStream stream(&content);

        stream << m_error.what() << "\n";
        stream << "  "
               << m_modelPrivate->errorLocation(m_error)
               << "\n";

        foreach (const Stack &stack, m_error.stacks()) {
            if (!stack.auxWhat().isEmpty())
                stream << stack.auxWhat();
            int i = 1;
            foreach (const Frame &frame, stack.frames())
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
        if (m_error.stacks().count() != 1 || m_error.stacks().first().frames().count() != 1
                || m_error.stacks().first().frames().first().functionName().isEmpty()) {
            return m_error.what();
        }
        return ErrorListModel::tr("%1 in function %2")
                .arg(m_error.what(), m_error.stacks().first().frames().first().functionName());
    case Qt::ToolTipRole:
        return toolTipForFrame(m_modelPrivate->findRelevantFrame(m_error));
    default:
        return QVariant();
    }
}


StackItem::StackItem(const Stack &stack) : m_stack(stack)
{
    foreach (const Frame &f, m_stack.frames())
        appendChild(new FrameItem(f));
}

QVariant StackItem::data(int column, int role) const
{
    const ErrorItem * const errorItem = getErrorItem();
    if (column == Analyzer::DetailedErrorView::LocationColumn)
        return location(errorItem->modelPrivate()->findRelevantFrame(errorItem->error()), role);

    // DiagnosticColumn
    switch (role) {
    case ErrorListModel::ErrorRole:
        return QVariant::fromValue(errorItem->error());
    case Qt::DisplayRole:
        return m_stack.auxWhat().isEmpty() ? errorItem->error().what() : m_stack.auxWhat();
    case Qt::ToolTipRole:
        return toolTipForFrame(errorItem->modelPrivate()->findRelevantFrame(errorItem->error()));
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
    if (column == Analyzer::DetailedErrorView::LocationColumn)
        return location(m_frame, role);

    // DiagnosticColumn
    switch (role) {
    case ErrorListModel::ErrorRole:
        return QVariant::fromValue(getErrorItem()->error());
    case Qt::DisplayRole: {
        const int row = parent()->children().indexOf(const_cast<FrameItem *>(this)) + 1;
        const int padding = static_cast<int>(std::log10(parent()->rowCount()))
                - static_cast<int>(std::log10(row));
        return QString::fromLatin1("%1%2: %3")
                .arg(QString(padding, QLatin1Char(' ')))
                .arg(row)
                .arg(makeFrameName(m_frame, false));
    }
    case Qt::ToolTipRole:
        return toolTipForFrame(m_frame);
    default:
        return QVariant();
    }
}

const ErrorItem *FrameItem::getErrorItem() const
{
    for (const TreeItem *parentItem = parent(); parentItem; parentItem = parentItem->parent()) {
        const ErrorItem * const errorItem = dynamic_cast<const ErrorItem *>(parentItem);
        if (errorItem)
            return errorItem;
    }
    QTC_CHECK(false);
    return nullptr;
}

} // namespace XmlProtocol
} // namespace Valgrind
