#include "mainwindow.h"
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>

MainWindow::MainWindow()
:   QMainWindow(0, 0)
{
    setupUi(this);

    connect(&m_debugger, SIGNAL(debugOutput(const QString&)), SLOT(appendOutput(const QString&)));
    connect(&m_debugger, SIGNAL(debuggeePaused()), SLOT(onDebuggeePaused()));
}

void MainWindow::setDebuggee(const QString& filename)
{
    m_debugger.openProcess(filename);
}

void MainWindow::on_actionOpen_triggered()
{
    QString exeName;
    exeName = QFileDialog::getOpenFileName(this, "Open Executable", ".", "*.exe");
    if (!exeName.isNull())
        m_debugger.openProcess(exeName);
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionBreak_triggered()
{
    m_debugger.breakAtCurrentPosition();
}

void MainWindow::on_actionRun_triggered()
{
    m_debugger.continueProcess();
}

void MainWindow::on_lstStack_itemClicked(QListWidgetItem* item)
{
    Debugger::StackFrame sf = m_stackTrace[ lstStack->row(item) ];
    QFile f(sf.filename);
    if (!f.exists())
        return;

    f.open(QFile::ReadOnly);
    QTextStream ts(&f);
    int cursorPos = 0;
    int currentLine = 0;
    QString fullText;
    do {
        QString strLine = ts.readLine();
        currentLine++;
        if (currentLine < sf.line)
            cursorPos += strLine.length();
        fullText.append(strLine + "\n");
    } while (!ts.atEnd());
    codeWindow->setPlainText(fullText);

    //QList<QTextEdit::ExtraSelection> extraSelections;
    //extraSelections.append(QTextEdit::ExtraSelection());

    //QTextEdit::ExtraSelection& exsel = extraSelections.first();
    //exsel.cursor.setPosition(cursorPos, QTextCursor::MoveAnchor);
    //exsel.cursor.select(QTextCursor::LineUnderCursor);
    //exsel.format.setBackground(Qt::red);
    //exsel.format.setFontUnderline(true);
    //codeWindow->setExtraSelections(extraSelections);
}

void MainWindow::appendOutput(const QString& str)
{
    teOutput->setPlainText(teOutput->toPlainText() + str);
}

void MainWindow::onDebuggeePaused()
{
    lstStack->clear();
    m_stackTrace = m_debugger.stackTrace();
    foreach (Debugger::StackFrame sf, m_stackTrace) {
        QString str = sf.symbol;
        if (!sf.filename.isEmpty())
            str.append(" at " + sf.filename + ":" + QString::number(sf.line));
        lstStack->addItem(str);
    }
}
