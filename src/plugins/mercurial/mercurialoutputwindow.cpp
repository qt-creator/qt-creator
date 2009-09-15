#include "mercurialoutputwindow.h"

#include <QtGui/QListWidget>
#include <QtCore/QDebug>
#include <QtCore/QTextCodec>

using namespace Mercurial::Internal;

MercurialOutputWindow::MercurialOutputWindow()
{
    outputListWidgets = new QListWidget;
    outputListWidgets->setWindowTitle(tr("Mercurial Output"));
    outputListWidgets->setFrameStyle(QFrame::NoFrame);
    outputListWidgets->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

MercurialOutputWindow::~MercurialOutputWindow()
{
    delete outputListWidgets;
    outputListWidgets = 0;
}

QWidget *MercurialOutputWindow::outputWidget(QWidget *parent)
{
    outputListWidgets->setParent(parent);
    return outputListWidgets;
}

QList<QWidget*> MercurialOutputWindow::toolBarWidgets() const
{
    return QList<QWidget *>();
}

QString MercurialOutputWindow::name() const
{
    return tr("Mercurial");
}

int MercurialOutputWindow::priorityInStatusBar() const
{
    return -1;
}

void MercurialOutputWindow::clearContents()
{
    outputListWidgets->clear();
}

void MercurialOutputWindow::visibilityChanged(bool visible)
{
    if (visible)
        outputListWidgets->setFocus();
}

void MercurialOutputWindow::setFocus()
{
}

bool MercurialOutputWindow::hasFocus()
{
    return outputListWidgets->hasFocus();
}

bool MercurialOutputWindow::canFocus()
{
    return false;
}

bool MercurialOutputWindow::canNavigate()
{
    return false;
}

bool MercurialOutputWindow::canNext()
{
    return false;
}

bool MercurialOutputWindow::canPrevious()
{
    return false;
}

void MercurialOutputWindow::goToNext()
{
}

void MercurialOutputWindow::goToPrev()
{
}

void MercurialOutputWindow::append(const QString &text)
{
    outputListWidgets->addItems(text.split(QLatin1Char('\n')));
    outputListWidgets->scrollToBottom();
    popup(true);
}

void MercurialOutputWindow::append(const QByteArray &array)
{
    append(QTextCodec::codecForLocale()->toUnicode(array));
}
