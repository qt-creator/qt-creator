/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "memcheckerrorview.h"

#include "suppressiondialog.h"
#include "valgrindsettings.h"

#include "xmlprotocol/error.h"
#include "xmlprotocol/errorlistmodel.h"
#include "xmlprotocol/frame.h"
#include "xmlprotocol/stack.h"
#include "xmlprotocol/modelhelpers.h"
#include "xmlprotocol/suppression.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QDebug>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QPainter>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QVBoxLayout>

using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

class MemcheckErrorDelegate : public Analyzer::DetailedErrorDelegate
{
    Q_OBJECT

public:
    explicit MemcheckErrorDelegate(QListView *parent);

    void copy();

    SummaryLineInfo summaryInfo(const QModelIndex &index) const;

private:
    QWidget *createDetailsWidget(const QFont &font, const QModelIndex &errorIndex,
                                 QWidget *parent) const;
};

static QString makeFrameName(const Frame &frame, const QString &relativeTo,
                             bool link = true, const QString &linkAttr = QString())
{
    const QString d = frame.directory();
    const QString f = frame.file();
    const QString fn = frame.functionName();
    const QString fullPath = d + QDir::separator() + f;

    QString path;
    if (!d.isEmpty() && !f.isEmpty())
        path = fullPath;
    else
        path = frame.object();

    if (QFile::exists(path))
        path = QFileInfo(path).canonicalFilePath();

    if (path.startsWith(relativeTo))
        path.remove(0, relativeTo.length());

    if (frame.line() != -1)
        path += QLatin1Char(':') + QString::number(frame.line());

    path = path.toHtmlEscaped();

    if (link && !f.isEmpty() && QFile::exists(fullPath)) {
        // make a hyperlink label
        path = QString::fromLatin1("<a href=\"file://%1:%2\" %4>%3</a>")
                    .arg(fullPath, QString::number(frame.line()), path, linkAttr);
    }

    if (!fn.isEmpty())
        return QCoreApplication::translate("Valgrind::Internal", "%1 in %2").arg(fn.toHtmlEscaped(), path);
    if (!path.isEmpty())
        return path;
    return QString::fromLatin1("0x%1").arg(frame.instructionPointer(), 0, 16);
}

static QString relativeToPath()
{
    // The project for which we insert the snippet.
    const ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();

    QString relativeTo(project ? project->projectDirectory().toString() : QDir::homePath());
    if (!relativeTo.endsWith(QDir::separator()))
        relativeTo.append(QDir::separator());

    return relativeTo;
}

static QString errorLocation(const QModelIndex &index, const Error &error,
                      bool link = false, const QString &linkAttr = QString())
{
    if (!index.isValid())
        return QString();
    const ErrorListModel *model = 0;
    const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel *>(index.model());
    while (!model && proxy) {
        model = qobject_cast<const ErrorListModel *>(proxy->sourceModel());
        proxy = qobject_cast<const QAbstractProxyModel *>(proxy->sourceModel());
    }
    QTC_ASSERT(model, return QString());

    return QCoreApplication::translate("Valgrind::Internal", "in %1").
            arg(makeFrameName(model->findRelevantFrame(error), relativeToPath(),
                              link, linkAttr));
}

QWidget *MemcheckErrorDelegate::createDetailsWidget(const QFont & font, const QModelIndex &errorIndex, QWidget *parent) const
{
    QWidget *widget = new QWidget(parent);
    QVBoxLayout *layout = new QVBoxLayout;
    // code + white-space:pre so the padding (see below) works properly
    // don't include frameName here as it should wrap if required and pre-line is not supported
    // by Qt yet it seems
    const QString displayTextTemplate = QString::fromLatin1("<code style='white-space:pre'>%1:</code> %2");
    const QString relativeTo = relativeToPath();
    const Error error = errorIndex.data(ErrorListModel::ErrorRole).value<Error>();

    QLabel *errorLabel = new QLabel();
    errorLabel->setWordWrap(true);
    errorLabel->setContentsMargins(0, 0, 0, 0);
    errorLabel->setMargin(0);
    errorLabel->setIndent(0);
    QPalette p = errorLabel->palette();
    QColor lc = p.color(QPalette::Text);
    QString linkStyle = QString::fromLatin1("style=\"color:rgba(%1, %2, %3, %4);\"")
                            .arg(lc.red()).arg(lc.green()).arg(lc.blue()).arg(int(0.7 * 255));
    p.setBrush(QPalette::Text, p.highlightedText());
    errorLabel->setPalette(p);
    errorLabel->setText(QString::fromLatin1("%1&nbsp;&nbsp;<span %4>%2</span>")
                            .arg(error.what(), errorLocation(errorIndex, error, true, linkStyle),
                                 linkStyle));
    connect(errorLabel, &QLabel::linkActivated,
            this, &MemcheckErrorDelegate::openLinkInEditor);
    layout->addWidget(errorLabel);

    const QVector<Stack> stacks = error.stacks();
    for (int i = 0; i < stacks.count(); ++i) {
        const Stack &stack = stacks.at(i);
        // auxwhat for additional stacks
        if (i > 0) {
            QLabel *stackLabel = new QLabel(stack.auxWhat());
            stackLabel->setWordWrap(true);
            stackLabel->setContentsMargins(0, 0, 0, 0);
            stackLabel->setMargin(0);
            stackLabel->setIndent(0);
            QPalette p = stackLabel->palette();
            p.setBrush(QPalette::Text, p.highlightedText());
            stackLabel->setPalette(p);
            layout->addWidget(stackLabel);
        }
        int frameNr = 1;
        foreach (const Frame &frame, stack.frames()) {
            QString frameName = makeFrameName(frame, relativeTo);
            QTC_ASSERT(!frameName.isEmpty(), /**/);

            QLabel *frameLabel = new QLabel(widget);
            frameLabel->setAutoFillBackground(true);
            if (frameNr % 2 == 0) {
                // alternating rows
                QPalette p = frameLabel->palette();
                p.setBrush(QPalette::Base, p.alternateBase());
                frameLabel->setPalette(p);
            }

            QFont fixedPitchFont = font;
            fixedPitchFont.setFixedPitch(true);
            frameLabel->setFont(fixedPitchFont);
            connect(frameLabel, &QLabel::linkActivated,
                    this, &MemcheckErrorDelegate::openLinkInEditor);
            // pad frameNr to 2 chars since only 50 frames max are supported by valgrind
            const QString displayText = displayTextTemplate
                                            .arg(frameNr++, 2).arg(frameName);
            frameLabel->setText(displayText);

            frameLabel->setToolTip(toolTipForFrame(frame));
            frameLabel->setWordWrap(true);
            frameLabel->setContentsMargins(0, 0, 0, 0);
            frameLabel->setMargin(0);
            frameLabel->setIndent(10);
            layout->addWidget(frameLabel);
        }
    }

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    widget->setLayout(layout);
    return widget;
}

MemcheckErrorDelegate::MemcheckErrorDelegate(QListView *parent)
    : Analyzer::DetailedErrorDelegate(parent)
{
}

Analyzer::DetailedErrorDelegate::SummaryLineInfo MemcheckErrorDelegate::summaryInfo(
        const QModelIndex &index) const
{
    const Error error = index.data(ErrorListModel::ErrorRole).value<Error>();
    SummaryLineInfo info;
    info.errorText = error.what();
    info.errorLocation = errorLocation(index, error);
    return info;
}

void MemcheckErrorDelegate::copy()
{
    QTC_ASSERT(m_detailsIndex.isValid(), return);

    QString content;
    QTextStream stream(&content);
    const Error error = m_detailsIndex.data(ErrorListModel::ErrorRole).value<Error>();

    stream << error.what() << "\n";
    stream << "  " << errorLocation(m_detailsIndex, error) << "\n";

    const QString relativeTo = relativeToPath();

    foreach (const Stack &stack, error.stacks()) {
        if (!stack.auxWhat().isEmpty())
            stream << stack.auxWhat();
        int i = 1;
        foreach (const Frame &frame, stack.frames()) {
            stream << "  " << i++ << ": " << makeFrameName(frame, relativeTo) << "\n";
        }
    }

    stream.flush();
    QApplication::clipboard()->setText(content);
}

MemcheckErrorView::MemcheckErrorView(QWidget *parent)
    : Analyzer::DetailedErrorView(parent),
      m_settings(0)
{
    MemcheckErrorDelegate *delegate = new MemcheckErrorDelegate(this);
    setItemDelegate(delegate);

    m_copyAction = new QAction(this);
    m_copyAction->setText(tr("Copy Selection"));
    m_copyAction->setIcon(QIcon(QLatin1String(Core::Constants::ICON_COPY)));
    m_copyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    m_copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_copyAction, &QAction::triggered, delegate, &MemcheckErrorDelegate::copy);
    addAction(m_copyAction);

    m_suppressAction = new QAction(this);
    m_suppressAction->setText(tr("Suppress Error"));
    m_suppressAction->setIcon(QIcon(QLatin1String(":/qmldesigner/images/eye_crossed.png")));
    m_suppressAction->setShortcut(QKeySequence(Qt::Key_Delete));
    m_suppressAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_suppressAction, &QAction::triggered, this, &MemcheckErrorView::suppressError);
    addAction(m_suppressAction);
}

MemcheckErrorView::~MemcheckErrorView()
{
}

void MemcheckErrorView::setDefaultSuppressionFile(const QString &suppFile)
{
    m_defaultSuppFile = suppFile;
}

QString MemcheckErrorView::defaultSuppressionFile() const
{
    return m_defaultSuppFile;
}

// slot, can (for now) be invoked either when the settings were modified *or* when the active
// settings object has changed.
void MemcheckErrorView::settingsChanged(ValgrindBaseSettings *settings)
{
    QTC_ASSERT(settings, return);
    m_settings = settings;
}

void MemcheckErrorView::contextMenuEvent(QContextMenuEvent *e)
{
    const QModelIndexList indizes = selectionModel()->selectedRows();
    if (indizes.isEmpty())
        return;

    QList<Error> errors;
    foreach (const QModelIndex &index, indizes) {
        Error error = model()->data(index, ErrorListModel::ErrorRole).value<Error>();
        if (!error.suppression().isNull())
            errors << error;
    }

    QMenu menu;
    menu.addAction(m_copyAction);
    menu.addSeparator();
    menu.addAction(m_suppressAction);
    m_suppressAction->setEnabled(!errors.isEmpty());
    menu.exec(e->globalPos());
}

void MemcheckErrorView::suppressError()
{
    SuppressionDialog::maybeShow(this);
}

} // namespace Internal
} // namespace Valgrind

#include "memcheckerrorview.moc"
