#include "qmlstreamwriter.h"

#include <QtCore/QBuffer>
#include <QtCore/QStringList>

QmlStreamWriter::QmlStreamWriter(QByteArray *array)
    : m_indentDepth(0)
    , m_pendingLineLength(0)
    , m_maybeOneline(false)
    , m_stream(new QBuffer(array))
{
    m_stream->open(QIODevice::WriteOnly);
}

void QmlStreamWriter::writeStartDocument()
{
}

void QmlStreamWriter::writeEndDocument()
{
}

void QmlStreamWriter::writeLibraryImport(const QString &uri, int majorVersion, int minorVersion, const QString &as)
{
    m_stream->write(QString("import %1 %2.%3").arg(uri, QString::number(majorVersion), QString::number(minorVersion)).toUtf8());
    if (!as.isEmpty())
        m_stream->write(QString(" as %1").arg(as).toUtf8());
    m_stream->write("\n");
}

void QmlStreamWriter::writeStartObject(const QString &component)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(QString("%1 {").arg(component).toUtf8());
    ++m_indentDepth;
    m_maybeOneline = true;
}

void QmlStreamWriter::writeEndObject()
{
    if (m_maybeOneline && !m_pendingLines.isEmpty()) {
        --m_indentDepth;
        for (int i = 0; i < m_pendingLines.size(); ++i) {
            m_stream->write(" ");
            m_stream->write(m_pendingLines.at(i).trimmed());
            if (i != m_pendingLines.size() - 1)
                m_stream->write(";");
        }
        m_stream->write(" }\n");
        m_pendingLines.clear();
        m_pendingLineLength = 0;
        m_maybeOneline = false;
    } else {
        if (m_maybeOneline)
            flushPotentialLinesWithNewlines();
        --m_indentDepth;
        writeIndent();
        m_stream->write("}\n");
    }
}

void QmlStreamWriter::writeScriptBinding(const QString &name, const QString &rhs)
{
    writePotentialLine(QString("%1: %2").arg(name, rhs).toUtf8());
}

void QmlStreamWriter::writeArrayBinding(const QString &name, const QStringList &elements)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(QString("%1: [\n").arg(name).toUtf8());
    ++m_indentDepth;
    for (int i = 0; i < elements.size(); ++i) {
        writeIndent();
        m_stream->write(elements.at(i).toUtf8());
        if (i != elements.size() - 1) {
            m_stream->write(",\n");
        } else {
            m_stream->write("\n");
        }
    }
    --m_indentDepth;
    writeIndent();
    m_stream->write("]\n");
}

void QmlStreamWriter::write(const QString &data)
{
    flushPotentialLinesWithNewlines();
    m_stream->write(data.toUtf8());
}

void QmlStreamWriter::writeScriptObjectLiteralBinding(const QString &name, const QList<QPair<QString, QString> > &keyValue)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(QString("%1: {\n").arg(name).toUtf8());
    ++m_indentDepth;
    for (int i = 0; i < keyValue.size(); ++i) {
        const QString key = keyValue.at(i).first;
        const QString value = keyValue.at(i).second;
        writeIndent();
        m_stream->write(QString("%1: %2").arg(key, value).toUtf8());
        if (i != keyValue.size() - 1) {
            m_stream->write(",\n");
        } else {
            m_stream->write("\n");
        }
    }
    --m_indentDepth;
    writeIndent();
    m_stream->write("}\n");
}

void QmlStreamWriter::writeIndent()
{
    m_stream->write(QByteArray(m_indentDepth * 4, ' '));
}

void QmlStreamWriter::writePotentialLine(const QByteArray &line)
{
    m_pendingLines.append(line);
    m_pendingLineLength += line.size();
    if (m_pendingLineLength >= 80) {
        flushPotentialLinesWithNewlines();
    }
}

void QmlStreamWriter::flushPotentialLinesWithNewlines()
{
    if (m_maybeOneline)
        m_stream->write("\n");
    foreach (const QByteArray &line, m_pendingLines) {
        writeIndent();
        m_stream->write(line);
        m_stream->write("\n");
    }
    m_pendingLines.clear();
    m_pendingLineLength = 0;
    m_maybeOneline = false;
}
