
// Built-in constants.
const int gl_MaxLights = 8;
const int gl_MaxClipPlanes = 6;
const int gl_MaxTextureUnits = 2;
const int gl_MaxTextureCoords = 2;
const int gl_MaxVertexAttribs = 16;
const int gl_MaxVertexUniformComponents = 512;
const int gl_MaxVaryingFloats = 32;
const int gl_MaxVertexTextureImageUnits = 0;
const int gl_MaxCombinedTextureImageUnits = 2;
const int gl_MaxTextureImageUnits = 2;
const int gl_MaxFragmentUniformComponents = 64;
const int gl_MaxDrawBuffers = 1;

// Built-in uniform state.
uniform mat4 gl_ModelViewMatrix;
uniform mat4 gl_ProjectionMatrix;
uniform mat4 gl_ModelViewProjectionMatrix;
uniform mat4 gl_TextureMatrix[gl_MaxTextureCoords];
uniform mat3 gl_NormalMatrix;

uniform mat4 gl_ModelViewMatrixInverse;
uniform mat4 gl_ProjectionMatrixInverse;
uniform mat4 gl_ModelViewProjectionMatrixInverse;
uniform mat4 gl_TextureMatrixInverse[gl_MaxTextureCoords];

uniform mat4 gl_ModelViewMatrixTranspose;
uniform mat4 gl_ProjectionMatrixTranspose;
uniform mat4 gl_ModelViewProjectionMatrixTranspose;
uniform mat4 gl_TextureMatrixTranspose[gl_MaxTextureCoords];

uniform mat4 gl_ModelViewMatrixInverseTranspose;
uniform mat4 gl_ProjectionMatrixInverseTranspose;
uniform mat4 gl_ModelViewProjectionMatrixInverseTranspose;
uniform mat4 gl_TextureMatrixInverseTranspose[gl_MaxTextureCoords];

uniform float gl_NormalScale;

struct gl_DepthRangeParameters {
    float near;
    float far;
    float diff;
};
uniform gl_DepthRangeParameters gl_DepthRange;

uniform vec4 gl_ClipPlane[gl_MaxClipPlanes];

struct gl_PointParameters {
    float size;
    float sizeMin;
    float sizeMax;
    float fadeThresholdSize;
    float distanceConstantAttenuation;
    float distanceLinearAttenuation;
    float distanceQuadraticAttenuation;
};
uniform gl_PointParameters gl_Point;

struct gl_MaterialParameters {
    vec4  emission;
    vec4  ambient;
    vec4  diffuse;
    vec4  specular;
    float shininess;
};
uniform gl_MaterialParameters gl_FrontMaterial;
uniform gl_MaterialParameters gl_BackMaterial;

struct gl_LightSourceParameters {
    vec4  ambient;
    vec4  diffuse;
    vec4  specular;
    vec4  position;
    vec4  halfVector;
    vec3  spotDirection;
    float spotExponent;
    float spotCutoff;
    float spotCosCutoff;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};
uniform gl_LightSourceParameters gl_LightSource[gl_MaxLights];

struct gl_LightModelParameters {
    vec4  ambient;
};
uniform gl_LightModelParameters gl_LightModel;

struct gl_LightModelProducts {
    vec4  sceneColor;
};
uniform gl_LightModelProducts gl_FrontLightModelProduct;
uniform gl_LightModelProducts gl_BackLightModelProduct;

struct gl_LightProducts {
    vec4  ambient;
    vec4  diffuse;
    vec4  specular;
};
uniform gl_LightProducts gl_FrontLightProduct[gl_MaxLights];
uniform gl_LightProducts gl_BackLightProduct[gl_MaxLights];

uniform vec4 gl_TextureEnvColor[gl_MaxTextureUnits];
uniform vec4 gl_EyePlaneS[gl_MaxTextureUnits];
uniform vec4 gl_EyePlaneT[gl_MaxTextureUnits];
uniform vec4 gl_EyePlaneR[gl_MaxTextureUnits];
uniform vec4 gl_EyePlaneQ[gl_MaxTextureUnits];
uniform vec4 gl_ObjectPlaneS[gl_MaxTextureUnits];
uniform vec4 gl_ObjectPlaneT[gl_MaxTextureUnits];
uniform vec4 gl_ObjectPlaneR[gl_MaxTextureUnits];
uniform vec4 gl_ObjectPlaneQ[gl_MaxTextureUnits];

struct gl_FogParameters {
    vec4  color;
    float density;
    float start;
    float end;
    float scale;
};
uniform gl_FogParameters gl_Fog;

// Angle and trigonometry functions.
genType radians(genType degress);
genType degrees(genType radians);
genType sin(genType angle);
genType cos(genType angle);
genType tan(genType angle);
genType asin(genType x);
genType acos(genType x);
genType atan(genType y, genType x);
genType atan(genType y_over_x);

// Exponential functions.
genType pow(genType x, genType y);
genType exp(genType x);
genType log(genType x);
genType exp2(genType x);
genType log2(genType x);
genType sqrt(genType x);
genType inversesqrt(genType x);

// Common functions.
genType abs(genType x);
genType sign(genType x);
genType floor(genType x);
genType ceil(genType x);
genType fract(genType x);
genType mod(genType x, float y);
genType mod(genType x, genType y);
genType min(genType x, float y);
genType min(genType x, genType y);
genType max(genType x, float y);
genType max(genType x, genType y);
genType clamp(genType x, genType minVal, genType maxVal);
genType clamp(genType x, float minVal, float maxVal);
genType mix(genType x, genType y, genType a);
genType mix(genType x, genType y, float a);
genType step(genType edge, genType x);
genType step(float edge, genType x);
genType smoothstep(genType edge0, genType edge1, genType x);
genType smoothstep(float edge0, float edge1, genType x);
float length(genType x);
float distance(genType p0, genType p1);
float dot(genType x, genType y);
vec3 cross(vec3 x, vec3 y);
genType normalize(genType x);
genType faceforward(genType N, genType I, genType Nref);
genType reflect(genType I, genType N);
genType refract(genType I, genType N, float eta);

// Matrix functions.
mat2 matrixCompMult(mat2 x, mat2 y);
mat3 matrixCompMult(mat3 x, mat3 y);
mat4 matrixCompMult(mat4 x, mat4 y);
mat2x4 matrixCompMult(mat2x4 x, mat2x4 y);
mat4x2 matrixCompMult(mat4x2 x, mat4x2 y);
mat2x3 matrixCompMult(mat2x3 x, mat2x3 y);
mat3x2 matrixCompMult(mat3x2 x, mat3x2 y);
mat3x4 matrixCompMult(mat3x4 x, mat3x4 y);
mat4x3 matrixCompMult(mat4x3 x, mat4x3 y);
mat2 outerProduct(vec2 c, vec2 r);
mat3 outerProduct(vec3 c, vec3 r);
mat4 outerProduct(vec4 c, vec4 r);
mat2x3 outerProduct(vec3 c, vec2 r);
mat3x2 outerProduct(vec2 c, vec3 r);
mat2x4 outerProduct(vec4 c, vec2 r);
mat4x2 outerProduct(vec2 c, vec4 r);
mat3x4 outerProduct(vec4 c, vec3 r);
mat4x3 outerProduct(vec3 c, vec4 r);
mat2 transpose(mat2 m);
mat3 transpose(mat3 m);
mat4 transpose(mat4 m);
mat2x3 transpose(mat3x2 m);
mat3x2 transpose(mat2x3 m);
mat2x4 transpose(mat4x2 m);
mat4x2 transpose(mat2x4 m);
mat3x4 transpose(mat4x3 m);
mat4x3 transpose(mat3x4 m);

// Vector relational functions.
bvec2 lessThan(vec2 x, vec2 y);
bvec2 lessThan(ivec2 x, ivec2 y);
bvec3 lessThan(vec3 x, vec3 y);
bvec3 lessThan(ivec3 x, ivec3 y);
bvec4 lessThan(vec4 x, vec4 y);
bvec4 lessThan(ivec4 x, ivec4 y);
bvec2 lessThanEqual(vec2 x, vec2 y);
bvec2 lessThanEqual(ivec2 x, ivec2 y);
bvec3 lessThanEqual(vec3 x, vec3 y);
bvec3 lessThanEqual(ivec3 x, ivec3 y);
bvec4 lessThanEqual(vec4 x, vec4 y);
bvec4 lessThanEqual(ivec4 x, ivec4 y);
bvec2 greaterThan(vec2 x, vec2 y);
bvec2 greaterThan(ivec2 x, ivec2 y);
bvec3 greaterThan(vec3 x, vec3 y);
bvec3 greaterThan(ivec3 x, ivec3 y);
bvec4 greaterThan(vec4 x, vec4 y);
bvec4 greaterThan(ivec4 x, ivec4 y);
bvec2 greaterThanEqual(vec2 x, vec2 y);
bvec2 greaterThanEqual(ivec2 x, ivec2 y);
bvec3 greaterThanEqual(vec3 x, vec3 y);
bvec3 greaterThanEqual(ivec3 x, ivec3 y);
bvec4 greaterThanEqual(vec4 x, vec4 y);
bvec4 greaterThanEqual(ivec4 x, ivec4 y);
bvec2 equal(vec2 x, vec2 y);
bvec2 equal(ivec2 x, ivec2 y);
bvec2 equal(bvec2 x, bvec2 y);
bvec3 equal(vec3 x, vec3 y);
bvec3 equal(ivec3 x, ivec3 y);
bvec3 equal(bvec3 x, bvec3 y);
bvec4 equal(vec4 x, vec4 y);
bvec4 equal(ivec4 x, ivec4 y);
bvec4 equal(bvec4 x, bvec4 y);
bvec2 notEqual(vec2 x, vec2 y);
bvec2 notEqual(ivec2 x, ivec2 y);
bvec2 notEqual(bvec2 x, bvec2 y);
bvec3 notEqual(vec3 x, vec3 y);
bvec3 notEqual(ivec3 x, ivec3 y);
bvec3 notEqual(bvec3 x, bvec3 y);
bvec4 notEqual(vec4 x, vec4 y);
bvec4 notEqual(ivec4 x, ivec4 y);
bvec4 notEqual(bvec4 x, bvec4 y);
bool any(bvec2 x);
bool any(bvec3 x);
bool any(bvec4 x);
bool all(bvec2 x);
bool all(bvec3 x);
bool all(bvec4 x);
bvec2 not(bvec2 x);
bvec3 not(bvec3 x);
bvec4 not(bvec4 x);
