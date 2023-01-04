# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import pandas as pd
import plotly.graph_objects as go
import plotly.subplots as sp
import plotly.express as px

def readWriteAppend(data, name, rowA, rowB, group, colorIdx):
    if rowA.empty or rowB.empty:
        return
    data['processName'].append(name)
    data['threadName'].append(rowA['threadName'])
    data['name'].append("ReadWrite")
    data['begin'].append(rowA['begin'])
    data['duration'].append(rowB['begin']-rowA['begin'])
    data['fromName'].append(rowA['command'])
    data['toName'].append(rowB['command'])
    data['fromCounter'].append(rowA['counter'])
    data['toCounter'].append(rowB['counter'])
    data['group'].append(group)
    data['color'].append(px.colors.qualitative.Plotly[colorIdx])


def recursiveAdd(data, dFromRowD, dfd, dfp, writeCmd, readCmd, trackName, counter):
    dFromRow = dFromRowD.squeeze()

    dToMask = (dfd['name']=="writeCommand") & (dfd['command']==writeCmd) & (dfd['begin']>dFromRow['begin'])
    dToRowD = dfd.loc[ dToMask ].head(1)
    dToRow = dToRowD.squeeze()
    if dToRowD.empty: return

    dfd.drop([dFromRowD.index[0], dToRowD.index[0]], errors='ignore', inplace=True)

    pFromMask = (dfp['name']=="readCommand") & (dfp['command']==writeCmd) & (dfp['counter']==dToRow['counter'])
    pFromRowD = dfp.loc[ pFromMask ].head(1)
    pFromRow = pFromRowD.squeeze()
    if pFromRowD.empty: return

    pToMask = (dfp['name']=="writeCommand") & (dfp['command']==readCmd) & (dfp['begin']>pFromRow['begin'])
    pToRowD = dfp.loc[ pToMask ].head(1)
    pToRow = pToRowD.squeeze()
    if pToRowD.empty: return

    dfp.drop([pFromRowD.index[0], pToRowD.index[0]], errors='ignore', inplace=True)

    name = trackName + " (" + str(counter) + ")"
    readWriteAppend(data, name, dFromRow, dToRow, counter, 0)
    readWriteAppend(data, name, pFromRow, pToRow, counter, counter+1)

    dFromMask = (dfd['name']=="readCommand") & (dfd['command']==readCmd) & (dfd['counter']==pToRow['counter'])
    dFromRowD = dfd.loc[ dFromMask ].head(1)
    if not dFromRowD.empty:
        recursiveAdd(data, dFromRowD, dfd, dfp, writeCmd, readCmd, trackName, counter)

def createDsToPuppetDataframe(dfdo, dfpo, writeCmd, readCmd, trackName):
    if dfdo.empty or dfpo.empty:
        return pd.DataFrame()

    cmdMask = lambda df: (df['type'] == "I") & (df['category']=="Update") & ((df['command'] == writeCmd) | (df['command'] == readCmd))
    dfd = dfdo.loc[cmdMask(dfdo) | (dfdo['category'] == "Initialize")].copy()
    dfp = dfpo.loc[cmdMask(dfpo)].copy()

    data = {
        "processName": [],
        "threadName": [],
        "name": [],
        "begin": [],
        "duration": [],
        "fromName": [],
        "toName": [],
        "fromCounter": [],
        "toCounter": [],
        "group": [],
        "color": []
    }

    dFromMask = (dfd['category']=="Initialize") & (dfd['name']=="Initialize") & (dfd['command']=="Initialize")
    dFromRowD = dfd.loc[ dFromMask ].head(1)

    group = 0
    nothingChanged = False
    while not nothingChanged:
        size = len(data['processName'])
        recursiveAdd(data, dFromRowD, dfd, dfp, writeCmd, readCmd, trackName, group)
        nothingChanged = size == len(data['processName'])
        group = group + 1

    df = pd.DataFrame(data)
    return df

def createDsToPuppetTrace(df):
    cd = list(zip(
        df.fromName.tolist(),
        df.toName.tolist(),
        df.duration.tolist(),
        df.fromCounter.tolist(),
        df.toCounter.tolist(),
        df.name.tolist()))

    return go.Bar(
        base = df.begin,
        x = df.duration,
        y = df.processName,
        marker = dict(color=df.color),
        insidetextanchor = 'middle',
        orientation = 'h',
        name = df.processName.iloc[0],
        customdata = cd,
        hovertemplate =
            '<b>%{customdata[5]}</b><br>' +
            '<b>Name From</b>: %{customdata[0]} (%{customdata[3]})<br>' +
            '<b>Name To</b>: %{customdata[1]} (%{customdata[4]})<br>' +
            '<b>Time From</b>: %{base} <b>To</b>: %{x}<br>' +
            '<b>Duration</b>: %{customdata[2]}',
    )

def addDsToPuppetTrace(figure, pos, dfd, dfp, writeCmd, readCmd, trackName):
    df = createDsToPuppetDataframe(dfd, dfp, writeCmd, readCmd, trackName)
    if not df.empty:
        figure.add_trace(createDsToPuppetTrace(df), row=pos, col=1)

def createTimeTrace(df):
    cd = list(zip(df.command.tolist(), df.duration.tolist()))
    return go.Bar(
        base = df.begin,
        x = df.duration,
        y = df.processName,
        text = df.name,
        insidetextanchor = 'middle',
        orientation = 'h',
        name = df.processName[0],
        customdata = cd,
        hovertemplate =
            '<b>%{text}</b><br>' +
            '<b>Command</b>: %{customdata[0]}<br>' +
            '<b>From</b>: %{base: .1f} <b>To</b>: %{x: .1f}<br>' +
            '<b>Duration</b>: %{customdata[1]: .1f}',
    )

def addTimeTrace(figure, pos, df):
    if not df.empty:
        figure.add_trace(createTimeTrace(df), row=pos, col=1)


colors = {
    'background': '#111111',
    'text': '#7FDBFF',
}

style = dict(
    plot_bgcolor=colors['background'],
    paper_bgcolor=colors['background'],
    font_color=colors['text']
)

axis = dict(
    titlefont_size=16,
    tickfont_size=14,
    showgrid=False,
)

def create(dfs):
    dfd = dfs['QmlDesigner']
    dfe = dfs['Puppet']['Editor']
    dfp = dfs['Puppet']['Preview']
    dfr = dfs['Puppet']['Render']

    figureCount = 2
    titles = ["DS To Puppet", "Duration events"]
    figure = sp.make_subplots(rows=figureCount, cols=1, subplot_titles=titles, vertical_spacing=0.3, shared_xaxes=True)

    addDsToPuppetTrace(figure, 1, dfd, dfr, "ChangeAuxiliaryCommand", "PixmapChangedCommand", "ChangeAux to PixChanged")
    addDsToPuppetTrace(figure, 1, dfd, dfp, "ChangeAuxiliaryCommand", "StatePreviewImageChangedCommand", "ChangeAux to PrevImgChanged")

    addTimeTrace(figure, 2, dfe.loc[ dfe['type'] != 'I' ].reset_index())
    addTimeTrace(figure, 2, dfp.loc[ dfp['type'] != 'I' ].reset_index())
    addTimeTrace(figure, 2, dfr.loc[ dfr['type'] != 'I' ].reset_index())
    addTimeTrace(figure, 2, dfd.loc[ dfd['type'] != 'I' ].reset_index())

    figure.update_layout(barmode='stack', height=500, **style)
    figure.update_xaxes(dict(**axis, title='Time (ms)', tickmode = 'auto', showticklabels=True, linecolor='green'))
    figure.update_yaxes(dict(**axis, fixedrange=True, type='category'))

    return figure

