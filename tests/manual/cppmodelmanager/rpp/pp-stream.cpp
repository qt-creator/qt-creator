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
/*
  Copyright 2006 Hamish Rodda <rodda@kde.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pp-stream.h"

#include <QtCore/QDebug>

class DevNullDevice : public QIODevice
{
protected:
  virtual qint64 readData ( char *, qint64 ) { return 0; }
  virtual qint64 writeData ( const char *, qint64 maxSize ) { return maxSize; }
};

Stream::Stream()
  : QTextStream(new DevNullDevice)
  , m_atEnd(false)
  , m_isNull(true)
  , m_pos(0)
  , m_inputLine(0)
  , m_outputLine(0)
{
}

Stream::Stream( const QByteArray & array, QIODevice::OpenMode openMode )
  : QTextStream(array, openMode)
  , m_atEnd(!array.count())
  , m_isNull(false)
  , m_pos(0)
  , m_inputLine(0)
  , m_outputLine(0)
{
  operator>>(c);
  //kDebug() << "'" << c << "' " << c.cell() << endl;
}

Stream::Stream( QByteArray * array, QIODevice::OpenMode openMode )
  : QTextStream(array, openMode)
  , m_atEnd(!array->count())
  , m_isNull(false)
  , m_pos(0)
  , m_inputLine(0)
  , m_outputLine(0)
{
  operator>>(c);
  //kDebug() << "'" << c << "' " << c.cell() << endl;
}

Stream::Stream( QString * string, QIODevice::OpenMode openMode )
  : QTextStream(string, openMode)
  , m_atEnd(!string->count())
  , m_isNull(false)
  , m_pos(0)
  , m_inputLine(0)
  , m_outputLine(0)
{
  operator>>(c);
  //if (!string->isEmpty())
    //kDebug() << "'" << c << "' " << c.cell() << endl;
}

Stream::Stream( FILE * fileHandle, QIODevice::OpenMode openMode )
  : QTextStream(fileHandle, openMode)
  , m_atEnd(false)
  , m_isNull(false)
  , m_pos(0)
  , m_inputLine(0)
  , m_outputLine(0)
{
  operator>>(c);
  //kDebug() << "'" << c << "' " << c.cell() << endl;
}

Stream::Stream( QIODevice * device )
  : QTextStream(device)
  , m_atEnd(false)
  , m_isNull(false)
  , m_pos(0)
  , m_inputLine(0)
  , m_outputLine(0)
{
  operator>>(c);
  //kDebug() << "'" << c << "' " << c.cell() << endl;
}

Stream::~Stream()
{
  if (isNull())
    delete device();
}

Stream & Stream::operator ++( )
{
  if (m_atEnd)
    return *this;

  if (c == '\n')
     ++m_inputLine;

  if (QTextStream::atEnd()) {
    m_atEnd = true;
    ++m_pos;
    c = QChar();

  } else {
    operator>>(c);
    //kDebug() << "'" << c << "' " << c.cell() << endl;
    ++m_pos;
  }
  return *this;
}

Stream& Stream::operator--()
{
  seek(pos() - 2);
  operator>>(c);
  --m_pos;
  return *this;
}

void Stream::rewind(qint64 offset)
{
  seek(pos() - offset);
}

bool Stream::atEnd() const
{
  return m_atEnd;
}

QChar Stream::peek() const
{
  Stream* s = const_cast<Stream*>(this);
  int inputLine = m_inputLine;
  ++(*s);
  QChar ret = s->current();
  s->rewind();
  inputLine = m_inputLine;
  return ret;
}

qint64 Stream::pos( ) const
{
  return m_pos;
}

void Stream::seek(qint64 offset)
{
  if (QTextStream::seek(offset)) {
    m_pos = offset;
    if (QTextStream::atEnd()) {
      m_atEnd = true;
    } else {
      operator>>(c);
      m_atEnd = false;
    }
  }
}

Stream& Stream::operator<< ( QChar c )
{
  if (!isNull()) {
    if (c == '\n') {
      ++m_outputLine;
      //output.clear();
    } else {
      //output += c;
    }
    QTextStream::operator<<(c);
  }
  return *this;
}

Stream& Stream::operator<< ( const QString & string )
{
  if (!isNull()) {
    m_outputLine += string.count('\n');
    //output += c;
    QTextStream::operator<<(string);
  }
  return *this;
}

int Stream::outputLineNumber() const
{
  return m_outputLine;
}

bool Stream::isNull() const
{
  return m_isNull;
}

int Stream::inputLineNumber() const
{
  return m_inputLine;
}

void Stream::setOutputLineNumber(int line)
{
  m_outputLine = line;
}

void Stream::mark(const QString& filename, int inputLineNumber)
{
  QTextStream::operator<<(QString("# %1 \"%2\"\n").arg(inputLineNumber).arg(filename.isEmpty() ? QString("<internal>") : filename));
  setOutputLineNumber(inputLineNumber);
}

void Stream::reset( )
{
  QTextStream::seek(0);
  m_inputLine = m_outputLine = m_pos = 0;
  //output.clear();
  m_atEnd = false;
  operator>>(c);
}
