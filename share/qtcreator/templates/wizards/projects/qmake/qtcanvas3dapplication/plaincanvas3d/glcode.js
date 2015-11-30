//
// Draws a plain green cube.
//

var gl;
var vertexPositionAttrLoc;
var shaderProgram;
var cubeVertexPositionBuffer;
var cubeVertexIndexBuffer;
var vertexShader;
var fragmentShader;
var pMatrixUniformLoc;
var mvMatrixUniformLoc;

var canvas3d;


function initializeGL(canvas) {
    canvas3d = canvas
    try {
        // Get the context object that represents the 3D API
        gl = canvas.getContext("experimental-webgl", {depth:true});

        // Setup the OpenGL state
        gl.enable(gl.DEPTH_TEST);
        gl.depthMask(true);
        gl.enable(gl.CULL_FACE);
        gl.cullFace(gl.BACK);
        gl.clearColor(0.0, 0.0, 0.0, 1.0);

        // Set viewport
        gl.viewport(0, 0, canvas.width * canvas.devicePixelRatio, canvas.height * canvas.devicePixelRatio);

        // Initialize vertex and element array buffers
        initBuffers();

        // Initialize the shader program
        initShaders();
    } catch(e) {
        console.log("initializeGL FAILURE!");
        console.log(""+e);
        console.log(""+e.message);
    }
}

function resizeGL(canvas) {
    var pixelRatio = canvas.devicePixelRatio;
    canvas.pixelSize = Qt.size(canvas.width * pixelRatio,
                               canvas.height * pixelRatio);
    if (gl) {
        gl.viewport(0, 0,
                    canvas.width * canvas.devicePixelRatio,
                    canvas.height * canvas.devicePixelRatio);
    }
}

function paintGL(canvas) {
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    gl.useProgram(shaderProgram);

    gl.uniformMatrix4fv(pMatrixUniformLoc, false,
                        [1.4082912447176388, 0, 0, 0,
                         0, 2.414213562373095, 0, 0,
                         0, 0, -1.002002002002002, -1,
                         0, 0, -0.20020020020020018, 0]);
    gl.uniformMatrix4fv(mvMatrixUniformLoc, false,
                        [0.4062233546734186, -0.7219893376569733, -0.5601017607788061, 0,
                         0.5868604797442467, 0.6759683901051262, -0.4457146092434445, 0,
                         0.7004122810404072, -0.14764190424242357, 0.6983011561492968, 0,
                         0, 0, -20, 1]);

    gl.bindBuffer(gl.ARRAY_BUFFER, cubeVertexPositionBuffer);
    gl.enableVertexAttribArray(vertexPositionAttrLoc);
    gl.vertexAttribPointer(vertexPositionAttrLoc, 3, gl.FLOAT, false, 0, 0);

    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, cubeVertexIndexBuffer);
    gl.drawElements(gl.TRIANGLES, 36, gl.UNSIGNED_SHORT, 0);
}

function initBuffers() {
    // Create a buffer for cube vertices. Since we are not using textures, we don't need unique
    // vertices for each face. We can define the cube using 8 vertices.
    cubeVertexPositionBuffer = gl.createBuffer();
    cubeVertexPositionBuffer.name = "cubeVertexPositionBuffer";
    gl.bindBuffer(gl.ARRAY_BUFFER, cubeVertexPositionBuffer);
    gl.bufferData(
                gl.ARRAY_BUFFER,
                new Float32Array([ // front
                                  -1.0, -1.0,  1.0,
                                  1.0, -1.0,  1.0,
                                  1.0,  1.0,  1.0,
                                  -1.0,  1.0,  1.0,
                                  // back
                                  -1.0, -1.0, -1.0,
                                  1.0, -1.0, -1.0,
                                  1.0,  1.0, -1.0,
                                  -1.0,  1.0, -1.0
                                 ]),
                gl.STATIC_DRAW);

    // Create buffer for element array indices. We define six sides, each composed of two
    // triangles, using the vertices defined above.
    cubeVertexIndexBuffer = gl.createBuffer();
    cubeVertexIndexBuffer.name = "cubeVertexIndexBuffer";
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, cubeVertexIndexBuffer);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,
                  new Uint16Array([// front
                                   0, 1, 2,
                                   2, 3, 0,
                                   // top
                                   3, 2, 6,
                                   6, 7, 3,
                                   // back
                                   7, 6, 5,
                                   5, 4, 7,
                                   // bottom
                                   4, 5, 1,
                                   1, 0, 4,
                                   // left
                                   4, 0, 3,
                                   3, 7, 4,
                                   // right
                                   1, 5, 6,
                                   6, 2, 1
                                  ]),
                  gl.STATIC_DRAW);
}

function initShaders() {
    vertexShader = getShader(gl, "attribute highp vec3 aVertexPosition; \
                                  uniform highp mat4 uMVMatrix;         \
                                  uniform highp mat4 uPMatrix;          \
                                  void main(void) {                     \
                                      gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0); \
                                  }", gl.VERTEX_SHADER);
    fragmentShader = getShader(gl, "void main(void) {           \
                                        gl_FragColor = vec4(0.5, 0.76, 0.26, 1.0);  \
                                    }", gl.FRAGMENT_SHADER);

    shaderProgram = gl.createProgram();
    shaderProgram.name = "shaderProgram";
    gl.attachShader(shaderProgram, vertexShader);
    gl.attachShader(shaderProgram, fragmentShader);
    gl.linkProgram(shaderProgram);

    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
        console.log("Could not initialize shaders");
        console.log(gl.getProgramInfoLog(shaderProgram));
    }

    gl.useProgram(shaderProgram);

    // look up where the vertex data needs to go.
    vertexPositionAttrLoc = gl.getAttribLocation(shaderProgram, "aVertexPosition");
    gl.enableVertexAttribArray(vertexPositionAttrLoc);

    pMatrixUniformLoc  = gl.getUniformLocation(shaderProgram, "uPMatrix");
    pMatrixUniformLoc.name = "pMatrixUniformLoc";
    mvMatrixUniformLoc = gl.getUniformLocation(shaderProgram, "uMVMatrix");
    mvMatrixUniformLoc.name = "mvMatrixUniformLoc";
}

function getShader(gl, str, type) {
    var shader = gl.createShader(type);
    gl.shaderSource(shader, str);
    gl.compileShader(shader);

    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        console.log("JS:Shader compile failed");
        console.log(gl.getShaderInfoLog(shader));
        return null;
    }

    return shader;
}
