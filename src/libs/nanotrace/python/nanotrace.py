############################################################################
#
# Copyright (C) 2022 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################
import reader as rd
import figures as fgs

import os
import base64
import dash
import dash_core_components as dcc
import dash_html_components as html
import pandas as pd

uploadStyle = {
    'width': '100%',
    'height': '60px',
    'lineHeight': '60px',
    'borderWidth': '1px',
    'borderStyle': 'dashed',
    'borderRadius': '5px',
    'textAlign': 'center'
}

external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']
app = dash.Dash(__name__, external_stylesheets=external_stylesheets)
app.layout = html.Div(className="row", children= [
    dcc.Upload(id='upload-data', children=['Drag and Drop or ', html.A('Select a File')], style=uploadStyle, multiple=True),
    html.Div(id='output-data-upload'),
])

def createLayout(fig):
    return html.Div([
        html.Div(className="six columns",  style={"width": "100%", "height": "500px"}, children = [ dcc.Graph(id='timeline-graph', figure=fig)]),
    ] )

def decodeContent(content):
     content_type, content_string = content.split(',')
     return base64.b64decode(content_string)

@app.callback(dash.dependencies.Output('output-data-upload', 'children'),
              dash.dependencies.Input('upload-data', 'contents'),
              dash.dependencies.State('upload-data', 'filename'),
              dash.dependencies.State('upload-data', 'last_modified'))
def update_output(list_of_contents, list_of_names, list_of_dates):
    if list_of_names is None:
        return []

    if len(list_of_names) == 1:
        name, ext = os.path.splitext(list_of_names[0])
        if rd.hasExtension(list_of_names[0], ".zip"):
            decoded = decodeContent(list_of_contents[0])
            dfs = rd.openZipArchive(decoded)
            return createLayout(fgs.create(dfs))

    return []

if __name__ == '__main__':
    app.run_server(debug=True)



