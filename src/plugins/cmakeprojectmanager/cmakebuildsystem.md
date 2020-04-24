# `CMakeBuildSystem`

## Big Picture: `BuildSystem`

This is a sequence diagram of how `ProjectExplorer::BuildSystem` interacts with
its implementations:

```mermaid
sequenceDiagram
    User ->> BuildSystemImpl: provide data and ask for parse (impl. defined!)
    BuildSystemImpl ->> BuildSystem: call requestParse() or requestDelayedParse()
    activate BuildSystem
    BuildSystem ->> BuildSys    tem: m_delayedParsingTimer sends timeout()
    BuildSystem ->> BuildSystemImpl: call triggerParsing()
    deactivate BuildSystem
    activate BuildSystemImpl
    BuildSystemImpl ->> BuildSystem: call guardParsingRun()
    activate BuildSystem
    BuildSystem ->> ParseGuard: Construct
    activate ParseGuard
    ParseGuard ->> BuildSystem: call emitParsingStarted
    BuildSystem ->> User: signal parsingStarted()
    BuildSystem ->> BuildSystemImpl: Hand over ParseGuard
    deactivate BuildSystem
    BuildSystemImpl ->> BuildSystemImpl: Do parsing
    opt Report Success
    BuildSystemImpl ->> ParseGuard: markAsSuccess()
    end
    BuildSystemImpl ->> ParseGuard: Destruct
    ParseGuard ->> BuildSystem: emitParsingFinished()
    activate BuildSystem
    BuildSystem ->> User: Signal ParsingFinished(...)
    deactivate BuildSystem
    deactivate ParseGuard
    deactivate BuildSystemImpl
```

## The Details of `CMakeBuildSystem`

### States Overview

```mermaid
graph TD
    parse --> TreeScanner::asyncScanForFiles

    parse --> FileApiReader::parse
    FileApiReader::parse --> handleParsingSucceeded
    handleParsingSucceeded --> combineScanAndParse
    FileApiReader::parse --> handleParsingFailed
    handleParsingFailed --> combineScanAndParse

    TreeScanner::asyncScanForFiles --> handleTreeScanningFinished
    handleTreeScanningFinished --> combineScanAndParse
```

### Full Sequence Diagram

```mermaid
sequenceDiagram
    participant User
    participant ParseGuard
    participant CMakeBuildSystem
    participant FileApiReader

    alt Trigger Parsing
    User ->> CMakeBuildSystem: Any of the Actions defined for CMakeBuildSystem
    else
    User ->> CMakeBuildSystem: Signal from outside the CMakeBuildSystem
    end
    activate CMakeBuildSystem
    CMakeBuildSystem ->> CMakeBuildSystem: call setParametersAndRequestReparse()
    CMakeBuildSystem ->> CMakeBuildSystem: Validate parameters
    CMakeBuildSystem ->> FileApiReader: Construct
    activate FileApiReader
    CMakeBuildSystem ->> FileApiReader: call setParameters
    CMakeBuildSystem ->> CMakeBuildSystem: call request*Reparse()
    deactivate CMakeBuildSystem

    CMakeBuildSystem ->> CMakeBuildSystem: m_delayedParsingTimer sends timeout() triggering triggerParsing()

    activate CMakeBuildSystem

    CMakeBuildSystem ->>+ CMakeBuildSystem: call guardParsingRun()
    CMakeBuildSystem ->> ParseGuard: Construct
    activate ParseGuard
    ParseGuard ->> CMakeBuildSystem: call emitParsingStarted
    CMakeBuildSystem ->> User: signal parsingStarted()
    CMakeBuildSystem ->>- CMakeBuildSystem: Hand over ParseGuard

    CMakeBuildSystem ->>+ TreeScanner: call asyncScanForFiles()

    CMakeBuildSystem ->>+ FileApiReader: call parse(...)
    FileApiReader ->> FileApiReader: startState()
    deactivate CMakeBuildSystem

    opt Parse
    FileApiReader ->> FileApiReader: call startCMakeState(...)
    FileApiReader ->> FileApiReader: call cmakeFinishedState(...)
    end

    FileApiReader ->> FileApiReader: call endState(...)

    alt Return Result from FileApiReader
    FileApiReader ->> CMakeBuildSystem: signal dataAvailable() and trigger handleParsingSucceeded()
    CMakeBuildSystem ->> FileApiReader: call takeBuildTargets()
    CMakeBuildSystem ->>  FileApiReader: call takeParsedConfiguration(....)
    else
    FileApiReader ->> CMakeBuildSystem: signal errorOccurred(...) and trigger handelParsingFailed(...)
    CMakeBuildSystem ->> FileApiReader: call takeParsedConfiguration(....)
    end

    deactivate FileApiReader
    Note right of CMakeBuildSystem: TreeScanner is still missing here
    CMakeBuildSystem ->> CMakeBuildSystem: call combineScanAndParse()

    TreeScanner ->> CMakeBuildSystem: signal finished() triggering handleTreeScanningFinished()
    CMakeBuildSystem ->> TreeScanner: call release() to get files
    deactivate TreeScanner
    Note right of CMakeBuildSystem: All results are in now...
    CMakeBuildSystem ->> CMakeBuildSystem: call combineScanAndParse()

    activate CMakeBuildSystem
    opt: Parsing was a success
    CMakeBuildSystem ->> CMakeBuildSystem: call updateProjectData()
    CMakeBuildSystem ->> FileApiReader: call projectFilesToWatch()
    CMakeBuildSystem ->> FileApiReader: call createRawProjectParts(...)
    CMakeBuildSystem ->> FileApiReader: call resetData()
    CMakeBuildSystem ->> ParseGuard: call markAsSuccess()
    end

    CMakeBuildSystem ->> ParseGuard: Destruct
    deactivate ParseGuard

    CMakeBuildSystem ->> CMakeBuildSystem: call emitBuildSystemUpdated()
    deactivate FileApiReader
    deactivate CMakeBuildSystem
```

# `FileApiReader`

States in the `FileApiReader`.

```mermaid
graph TD
    startState --> startCMakeState
    startState --> endState
    startCMakeState --> cmakeFinishedState
    cmakeFinishedState --> endState
```

