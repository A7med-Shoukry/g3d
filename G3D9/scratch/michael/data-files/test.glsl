
	
  
    //ivec2 ssC = ivec2(gl_FragCoord.xy);
	vec2 ssNC = gl_FragCoord.xy / screenSize;
    gl_FragColor = texture(noise, ssNC);
    //gl_FragColor.rgb = (((ssC.x + ssC.y) & jump) > 0) ? ODD_COLOR : EVEN_COLOR;

	//I3D 2001 parametrized environment maps