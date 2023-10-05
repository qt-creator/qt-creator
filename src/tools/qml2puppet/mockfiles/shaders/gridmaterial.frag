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

        // Force additional alpha when approaching the far clip of edit camera
        if (depth > 90000.0)
            alpha *= clamp((100000.0 - depth) / 10000.0, 0, 1);

        if (alpha > 0.01)
            FRAGCOLOR = vec4(color.xyz * alpha, alpha);
        else
            discard;
    }
}
