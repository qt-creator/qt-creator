VARYING vec3 pos;
VARYING float worldPos;

void MAIN()
{
    if (orthoMode) {
        // No fadeout in orthographic mode
        FRAGCOLOR = vec4(color.xyz, 1);
    } else {
        vec3 camDir = CAMERA_POSITION - worldPos;
        vec3 camLevel = vec3(camDir.x, 0, camDir.z);
        float depth;
        depth = length(camDir);
        float cosAngle = dot(normalize(camDir), normalize(camLevel));
        float angleDepth = density * pow(cosAngle, 8);
        float alpha = generalAlpha * clamp((1.0 - ((angleDepth * depth - alphaStartDepth) / (alphaEndDepth - alphaStartDepth))), 0, 1);
        if (alpha > 0.01)
            FRAGCOLOR = vec4(color.x * alpha, color.y * alpha, color.z * alpha, alpha);
        else
            discard;
    }
}
