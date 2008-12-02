/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "attachexternaldialog.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QStandardItemModel>
#include <QHeaderView>

using namespace Debugger::Internal;

AttachExternalDialog::AttachExternalDialog(QWidget *parent, const QString &pid)
  : QDialog(parent)
{
    setupUi(this);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_defaultPID = pid;
    m_model = new QStandardItemModel(this);

    procView->setSortingEnabled(true);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(procView, SIGNAL(activated(const QModelIndex &)),
        this, SLOT(procSelected(const QModelIndex &)));


    pidLineEdit->setText(m_defaultPID);
    rebuildProcessList();
}

static bool isProcessName(const QString &procname)
{
    for (int i = 0; i != procname.size(); ++i)
        if (!procname.at(i).isDigit())
            return false;
    return true;
}

struct ProcData {
    QString ppid;
    QString name;
    QString state;
};

static void insertItem(QStandardItem *root, const QString &pid,
    const QMap<QString, ProcData> &procs, QMap<QString, QStandardItem *> &known)
{
    //qDebug() << "HANDLING " << pid;
    QStandardItem *parent = 0;
    const ProcData &proc = procs[pid];
    if (1 || pid == "0") {
        parent = root;
    } else {
        if (!known.contains(proc.ppid))
            insertItem(root, proc.ppid, procs, known);
        parent = known[proc.ppid];
    }
    QList<QStandardItem *> row;
    row.append(new QStandardItem(pid));
    row.append(new QStandardItem(proc.name));
    //row.append(new QStandardItem(proc.ppid));
    row.append(new QStandardItem(proc.state));
    parent->appendRow(row);
    known[pid] = row[0];
}

void AttachExternalDialog::rebuildProcessList()
{
    QStringList procnames = QDir("/proc/").entryList();
    if (procnames.isEmpty()) {
        procView->hide();
        return;
    }
    
    typedef QMap<QString, ProcData> Procs;
    Procs procs;

    foreach (const QString &procname, procnames) {
        if (!isProcessName(procname))
            continue;
        QString filename = "/proc/" + procname + "/stat";
        QFile file(filename);
        file.open(QIODevice::ReadOnly);
        QStringList data = QString::fromLocal8Bit(file.readAll()).split(' ');
        //qDebug() << filename << data;
        ProcData proc;
        proc.name = data.at(1);
        if (proc.name.startsWith('(') && proc.name.endsWith(')'))
            proc.name = proc.name.mid(1, proc.name.size() - 2);
        proc.state = data.at(2);
        proc.ppid = data.at(3);
        procs[procname] = proc;
    }

    m_model->clear();
    QMap<QString, QStandardItem *> known;
    for (Procs::const_iterator it = procs.begin(); it != procs.end(); ++it)
        insertItem(m_model->invisibleRootItem(), it.key(), procs, known);
    m_model->setHeaderData(0, Qt::Horizontal, "Process ID", Qt::DisplayRole);
    m_model->setHeaderData(1, Qt::Horizontal, "Name", Qt::DisplayRole);
    //model->setHeaderData(2, Qt::Horizontal, "Parent", Qt::DisplayRole);
    m_model->setHeaderData(2, Qt::Horizontal, "State", Qt::DisplayRole);

    procView->setModel(m_model);
    procView->expandAll();
    procView->resizeColumnToContents(0);
    procView->resizeColumnToContents(1);
}

#ifdef Q_OS_WINDOWS

#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>

//  Forward declarations:
BOOL GetProcessList( );
BOOL ListProcessModules( DWORD dwPID );
BOOL ListProcessThreads( DWORD dwOwnerPID );
void printError( TCHAR* msg );

BOOL GetProcessList( )
{
  HANDLE hProcessSnap;
  HANDLE hProcess;
  PROCESSENTRY32 pe32;
  DWORD dwPriorityClass;

  // Take a snapshot of all processes in the system.
  hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
    printError( TEXT("CreateToolhelp32Snapshot (of processes)") );
    return( FALSE );
  }

  // Set the size of the structure before using it.
  pe32.dwSize = sizeof( PROCESSENTRY32 );

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if( !Process32First( hProcessSnap, &pe32 ) )
  {
    printError( TEXT("Process32First") ); // show cause of failure
    CloseHandle( hProcessSnap );          // clean the snapshot object
    return( FALSE );
  }

  // Now walk the snapshot of processes, and
  // display information about each process in turn
  do
  {
    printf( "\n\n=====================================================" );
    _tprintf( TEXT("\nPROCESS NAME:  %s"), pe32.szExeFile );
    printf( "\n-----------------------------------------------------" );

    // Retrieve the priority class.
    dwPriorityClass = 0;
    hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID );
    if( hProcess == NULL )
      printError( TEXT("OpenProcess") );
    else
    {
      dwPriorityClass = GetPriorityClass( hProcess );
      if( !dwPriorityClass )
        printError( TEXT("GetPriorityClass") );
      CloseHandle( hProcess );
    }

    printf( "\n  Process ID        = 0x%08X", pe32.th32ProcessID );
    printf( "\n  Thread count      = %d",   pe32.cntThreads );
    printf( "\n  Parent process ID = 0x%08X", pe32.th32ParentProcessID );
    printf( "\n  Priority base     = %d", pe32.pcPriClassBase );
    if( dwPriorityClass )
      printf( "\n  Priority class    = %d", dwPriorityClass );

    // List the modules and threads associated with this process
    ListProcessModules( pe32.th32ProcessID );
    ListProcessThreads( pe32.th32ProcessID );

  } while( Process32Next( hProcessSnap, &pe32 ) );

  CloseHandle( hProcessSnap );
  return( TRUE );
}


BOOL ListProcessModules( DWORD dwPID )
{
  HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
  MODULEENTRY32 me32;

  // Take a snapshot of all modules in the specified process.
  hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
  if( hModuleSnap == INVALID_HANDLE_VALUE )
  {
    printError( TEXT("CreateToolhelp32Snapshot (of modules)") );
    return( FALSE );
  }

  // Set the size of the structure before using it.
  me32.dwSize = sizeof( MODULEENTRY32 );

  // Retrieve information about the first module,
  // and exit if unsuccessful
  if( !Module32First( hModuleSnap, &me32 ) )
  {
    printError( TEXT("Module32First") );  // show cause of failure
    CloseHandle( hModuleSnap );           // clean the snapshot object
    return( FALSE );
  }

  // Now walk the module list of the process,
  // and display information about each module
  do
  {
    _tprintf( TEXT("\n\n     MODULE NAME:     %s"),   me32.szModule );
    _tprintf( TEXT("\n     Executable     = %s"),     me32.szExePath );
    printf( "\n     Process ID     = 0x%08X",         me32.th32ProcessID );
    printf( "\n     Ref count (g)  = 0x%04X",     me32.GlblcntUsage );
    printf( "\n     Ref count (p)  = 0x%04X",     me32.ProccntUsage );
    printf( "\n     Base address   = 0x%08X", (DWORD) me32.modBaseAddr );
    printf( "\n     Base size      = %d",             me32.modBaseSize );

  } while( Module32Next( hModuleSnap, &me32 ) );

  CloseHandle( hModuleSnap );
  return( TRUE );
}

BOOL ListProcessThreads( DWORD dwOwnerPID ) 
{ 
  HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
  THREADENTRY32 te32; 
 
  // Take a snapshot of all running threads  
  hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
  if( hThreadSnap == INVALID_HANDLE_VALUE ) 
    return( FALSE ); 
 
  // Fill in the size of the structure before using it. 
  te32.dwSize = sizeof(THREADENTRY32 ); 
 
  // Retrieve information about the first thread,
  // and exit if unsuccessful
  if( !Thread32First( hThreadSnap, &te32 ) ) 
  {
    printError( TEXT("Thread32First") ); // show cause of failure
    CloseHandle( hThreadSnap );          // clean the snapshot object
    return( FALSE );
  }

  // Now walk the thread list of the system,
  // and display information about each thread
  // associated with the specified process
  do 
  { 
    if( te32.th32OwnerProcessID == dwOwnerPID )
    {
      printf( "\n\n     THREAD ID      = 0x%08X", te32.th32ThreadID ); 
      printf( "\n     Base priority  = %d", te32.tpBasePri ); 
      printf( "\n     Delta priority = %d", te32.tpDeltaPri ); 
    }
  } while( Thread32Next(hThreadSnap, &te32 ) ); 

  CloseHandle( hThreadSnap );
  return( TRUE );
}

void printError( TCHAR* msg )
{
  DWORD eNum;
  TCHAR sysMsg[256];
  TCHAR* p;

  eNum = GetLastError( );
  FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
         NULL, eNum,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
         sysMsg, 256, NULL );

  // Trim the end of the line and terminate it with a null
  p = sysMsg;
  while( ( *p > 31 ) || ( *p == 9 ) )
    ++p;
  do { *p-- = 0; } while( ( p >= sysMsg ) &&
                          ( ( *p == '.' ) || ( *p < 33 ) ) );

  // Display the message
  _tprintf( TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg );
}

#endif


void AttachExternalDialog::procSelected(const QModelIndex &index0)
{
    QModelIndex index = index0.sibling(index0.row(), 0);
    QStandardItem *item = m_model->itemFromIndex(index);
    if (!item)
        return;
    pidLineEdit->setText(item->text());
    accept();
}

int AttachExternalDialog::attachPID() const
{
    return pidLineEdit->text().toInt();
}
