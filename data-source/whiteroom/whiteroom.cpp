    static Shader::Ref shader;
    if (shader.isNull()) {
        shader = Shader::fromStrings(
            STR(
            varying vec3 pos;
            void main() {
                pos = -gl_Vertex.xyz;
                gl_Position = gl_ModelViewProjectionMatrix * vec4(-gl_Vertex.xyz, 1);
            }
            ), 
            STR(
            varying vec3 pos;
            void main() {
                float a = atan(pos.x, pos.z);
                if (pos.y < 0) {
                    // Ground
                    gl_FragColor.rgb = vec3(0.8);
                /*} else if ((pos.y < 0.05) && (cos(a * 4) > 0.995)) {
                    // Square horizon rotation markers every 90 degrees
                    gl_FragColor.rgb = vec3(0.8);
                } else if ((pos.y < 0.04) && (cos(a * 16) > 0.97 + pos.y * 2.0f)) {
                    // Triangular horizon rotation markers every 22.5 degrees
                    gl_FragColor.rgb = vec3(0.8);
                */} else {
                    // Sky
                    gl_FragColor.rgb = vec3(max(0.8, 1.0 - pos.y * 0.45));
                }
            }));
    }
    rd->setShader(shader);
    Draw::sphere(Sphere(Point3::zero(), 400), rd, Color3::white(), Color4::clear());
    return;