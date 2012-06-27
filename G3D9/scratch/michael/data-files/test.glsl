    ivec2 ssC = ivec2(gl_FragCoord.xy);
    //gl_FragColor.rgb = vec3(0.5);
    gl_FragColor.rgb = (((ssC.x + ssC.y) & 1) > 0) ? vec3(1.0) : vec3(0.0);