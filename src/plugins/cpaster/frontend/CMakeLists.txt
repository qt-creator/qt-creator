add_qtc_executable(cpaster
  DEPENDS Core Utils Qt5::Network
  SOURCES
    argumentscollector.cpp argumentscollector.h
    main.cpp
    ../pastebindotcomprotocol.cpp ../pastebindotcomprotocol.h
    ../pastecodedotxyzprotocol.cpp ../pastecodedotxyzprotocol.h
    ../protocol.cpp ../protocol.h
    ../urlopenprotocol.cpp ../urlopenprotocol.h
)
