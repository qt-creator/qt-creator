QtcLibrary {
    name: "CommonTraceFormat"
    cpp.defines: [ "COMMONTRACEFORMAT_LIBRARY" ]

    Depends { name: "Utils" }

    files: [ "commontraceformat_global.h" ]

    Group {
        name: "CommonTraceFormat Utils"
        prefix: "util/"
        files: [
            "leb128.h",
            "uuid.h",
        ]
    }

    Group {
        name: "CommonTraceFormat Schema"
        prefix: "schema/"
        files: [
            "blobfieldclasses.h",
            "clockclass.h",
            "compoundfieldclasses.h",
            "datastreamclass.h",
            "eventrecordclass.h",
            "fieldclass.h",
            "fieldlocation.h",
            "scalarfieldclasses.h",
            "schema.h", "schema.cpp",
            "stringfieldclasses.h",
            "traceclass.h",
        ]
    }

    Group {
        name: "CommonTraceFormat MetaData"
        prefix: "metadata/"
        files:[
            "ctf1packets.cpp", "ctf1packets.h",
            "metadatareader.cpp", "metadatareader.h",
            "metadatawriter.cpp", "metadatawriter.h",
            "tsdlparser.cpp", "tsdlparser.h",
        ]
    }

    Group {
        name: "CommonTraceFormat Binary"
        prefix: "binary/"
        files: [
            "bitbuffer.cpp", "bitbuffer.h",
            "fieldreader.cpp", "fieldreader.h",
            "fieldvalue.h",
            "fieldwriter.cpp", "fieldwriter.h",
            "packetreader.cpp", "packetreader.h",
            "packetwriter.cpp", "packetwriter.h",
        ]
    }

    Group {
        name: "CommonTraceFormat Stream"
        prefix: "stream/"
        files: [
            "datastreamreader.cpp", "datastreamreader.h",
            "datastreamwriter.cpp", "datastreamwriter.h",
            "tracedirectory.cpp", "tracedirectory.h",
            "tracereader.cpp", "tracereader.h",
            "tracewriter.cpp", "tracewriter.h",
        ]
    }
}
