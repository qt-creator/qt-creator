VARYING vec3 pos;
VARYING vec3 worldPos;

void MAIN()
{
    pos = VERTEX;
    vec4 pos4 = vec4(pos, 1.0);
    POSITION = MODELVIEWPROJECTION_MATRIX * pos4;
    worldPos = (MODEL_MATRIX * pos4).xyz;
}
