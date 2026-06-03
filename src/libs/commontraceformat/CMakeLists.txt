add_qtc_library(CommonTraceFormat
  SOURCES
    commontraceformat_global.h

    util/leb128.h
    util/uuid.h

    schema/fieldlocation.h
    schema/fieldclass.h
    schema/scalarfieldclasses.h
    schema/stringfieldclasses.h
    schema/blobfieldclasses.h
    schema/compoundfieldclasses.h
    schema/clockclass.h
    schema/traceclass.h
    schema/datastreamclass.h
    schema/eventrecordclass.h
    schema/schema.h                 schema/schema.cpp

    metadata/metadatawriter.h       metadata/metadatawriter.cpp
    metadata/metadatareader.h       metadata/metadatareader.cpp
    metadata/ctf1packets.h          metadata/ctf1packets.cpp
    metadata/tsdlparser.h           metadata/tsdlparser.cpp

    binary/bitbuffer.h              binary/bitbuffer.cpp
    binary/fieldvalue.h
    binary/fieldwriter.h            binary/fieldwriter.cpp
    binary/fieldreader.h            binary/fieldreader.cpp
    binary/packetwriter.h           binary/packetwriter.cpp
    binary/packetreader.h           binary/packetreader.cpp

    stream/tracewriter.h            stream/tracewriter.cpp
    stream/datastreamwriter.h       stream/datastreamwriter.cpp
    stream/tracereader.h            stream/tracereader.cpp
    stream/datastreamreader.h       stream/datastreamreader.cpp
    stream/tracedirectory.h         stream/tracedirectory.cpp

  PUBLIC_DEPENDS Qt::Core Utils
)
