# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import os
import io
import json

import pandas as pd
import zipfile as zf

def nsToMs(ns):
    return ns / 1000000

def appendEvent(data, event):
    data['processId'].append(event['pid'])
    data['threadId'].append(event['tid'])
    data['type'].append(event['ph'])
    data['category'].append(event['cat'])
    data['name'].append(event['name'])
    data['begin'].append(nsToMs(event['ts']))

    duration = 1.0
    command = ""
    counter = int(-1)

    if event['cat'] == "Initialize":
        command = "Initialize"

    if "dur" in event['args']:
        duration = nsToMs(event['args']['dur'])

    if "name" in event['args']:
        command = event["args"]["name"].split("::")[-1]

    if "counter" in event['args']:
        counter = event['args']['counter']

    data['duration'].append(duration)
    data['command'].append(command)
    data['counter'].append(counter)

def readTraceFile(file):
    pdata = {
        "processId": [],
        "processName": []
    }
    tdata = {
        "threadId": [],
        "threadName": []
    }
    data = {
        "processId": [],
        "threadId": [],
        "type": [],
        "category": [],
        "name": [],
        "begin": [],
        "duration": [],
        "command": [],
        "counter": []
    }

    for event in json.loads(file)["traceEvents"]:
        if event["ph"] == "M" and event["name"] == "process_name":
            pdata["processId"].append(event["pid"])
            pdata["processName"].append(event["args"]["name"])
        elif event["ph"] == "M" and event["name"] == "thread_name":
            tdata["threadId"].append(event["tid"])
            tdata["threadName"].append(event["args"]["name"])
        else:
            appendEvent(data, event)

    pdf = pd.DataFrame(pdata)
    tdf = pd.DataFrame(tdata)
    ddf = pd.DataFrame(data)

    # Replace pid/tid with processName/threadName and sort.
    df = ddf.merge(right=pdf).merge(right=tdf)
    df.drop(inplace=True, columns=["processId", "threadId"])
    df.sort_values("begin").reset_index(inplace=True, drop=True)

    # Merge Begin/End rows
    for idx, row in df.iterrows():
        if row['type'] == 'B':
            nextEndMask = (df['type']=='E') & (df.index>idx)
            relatedMask = (df['category']==row['category']) & (df['name']==row['name'])
            nextRelatedRow = df.loc[nextEndMask & relatedMask].head(1).squeeze()

            if nextRelatedRow.empty:
                df.loc[idx, 'duration'] = 1.0
            else:
                df.loc[idx, 'type'] = 'BE'
                df.loc[idx, 'duration'] = nextRelatedRow['begin'] - row['begin']

    # Make columns categorial
    categories = ['type', 'category', 'name', 'command', 'processName', 'threadName']
    df[categories] = df[categories].astype('category')

    df.drop(df[ df['type']=='E'].index, inplace=True)
    df.reset_index(inplace=True, drop=True)

    return df

def computePuppetOffset(dfd, dfp):
    puppetSyncWriteMask = (dfp['category'] == "Sync") & (dfp['name']=="writeCommand") & (dfp['type']=="I")
    puppetSyncWriteDf = dfp.loc[ puppetSyncWriteMask ].head(1)
    puppetSyncWriteRow = puppetSyncWriteDf.squeeze()
    if puppetSyncWriteDf.empty: return 0.0

    cmdName = puppetSyncWriteRow['command']

    puppetSyncReadMask = (dfp['category'] == "Sync") & (dfp['name']=="readCommand") & (dfp['command']==cmdName)
    puppetSyncReadDf = dfp.loc[ puppetSyncReadMask ].head(1)
    puppetSyncReadRow = puppetSyncReadDf.squeeze()
    if puppetSyncReadDf.empty: return 0.0

    designerSyncReadMask = (dfd['category'] == "Sync") & (dfd['name']=="readCommand") & (dfd['command']==cmdName)
    designerSyncWriteDf = dfd.loc[ designerSyncReadMask ].head(1)
    designerSyncWriteRow = designerSyncWriteDf.squeeze()

    if designerSyncWriteDf.empty: return 0.0

    puppetCenter = puppetSyncReadRow['begin'] - puppetSyncWriteRow['begin']
    return designerSyncWriteRow['begin'] - puppetCenter

def SynchronizeDataframes(dfs):
    dfd = dfs['QmlDesigner']
    dfe = dfs['Puppet']['Editor']
    dfp = dfs['Puppet']['Preview']
    dfr = dfs['Puppet']['Render']
    dfe['begin'] = dfe['begin'] + computePuppetOffset(dfd, dfe)
    dfp['begin'] = dfp['begin'] + computePuppetOffset(dfd, dfp)
    dfr['begin'] = dfr['begin'] + computePuppetOffset(dfd, dfr)
    return dfs

def collectDataframes(dfs):
    out = { "QmlDesigner": {}, "Puppet": { "Editor": {}, "Preview": {}, "Render": {} } }
    for df in dfs:
        if not df.empty:
            df = df.sort_values(by="threadName", ascending=True)
            if df['processName'][0] == 'QmlDesigner':
                out['QmlDesigner'] = df.sort_index()
            elif df['processName'][0] == 'EditorModePuppet':
                out['Puppet']['Editor'] = df.sort_index()
            elif df['processName'][0] == 'PreviewModePuppet':
                out['Puppet']['Preview'] = df.sort_index()
            elif df['processName'][0] == 'RenderModePuppet':
                out['Puppet']['Render'] = df.sort_index()

    return SynchronizeDataframes(out)

def hasExtension(filename, ext):
    name, e = os.path.splitext(filename)
    return True if e == ext else False

def extractZipArchive(zip_file):
    dfs = []
    for filename in zip_file.namelist():
        invalid = filename.startswith('__MACOSX')
        if not invalid and hasExtension(filename, ".json"):
            with zip_file.open(filename) as file_object:
                data = file_object.read().strip()
                dfs.append(readTraceFile(data))
    return collectDataframes(dfs)

def openZipArchive(data):
    zip_str = io.BytesIO(data)
    try:
        with zf.ZipFile(zip_str, 'r') as zip_file:
            return extractZipArchive(zip_file)
    except IOError:
        print("Failed to open zip file: \"" + path + "\"")
        sys.exit(2)

def readZipArchive(path):
    try:
        with zf.ZipFile(path) as zip_file:
            return extractZipArchive(zip_file)
    except IOError:
        print("Failed to read zip file: \"" + path + "\"")
        sys.exit(2)

def openTraceFile(path):
    try:
        with open(path, "r") as file_object:
            return readTraceFile(file_object.read())
    except IOError:
        print("Failed to open file: \"" + path + "\"")
        sys.exit(2)
