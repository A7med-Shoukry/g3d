models/players/crusader/chead
{     
        {
		map models/players/crusader/chead.tga
                blendFunc GL_ONE GL_ZERO
		rgbGen lightingDiffuse
	} 
        {
                map models/players/crusader/crus_plasma.tga            
                blendFunc GL_ONE GL_ONE
                tcmod scroll 0.1 1
	}  
        {
		map models/players/crusader/chead.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
       
}

models/players/crusader/cupper
{ 
        {
		map models/players/crusader/cupper.tga
                blendFunc GL_ONE GL_ZERO
		rgbGen lightingDiffuse
	} 
        {
                map models/players/crusader/crus_plasma.tga            
                blendFunc GL_ONE GL_ONE
                tcmod scroll 0.1 1
	}  
        {
		map models/players/crusader/cupper.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
       
}


models/players/crusader/clower
{ 
        {
		map models/players/crusader/clower.tga
                blendFunc GL_ONE GL_ZERO
		rgbGen lightingDiffuse
	} 
        {
                map models/players/crusader/crus_plasma.tga            
                blendFunc GL_ONE GL_ONE
                tcmod scroll 0.1 1
	}  
        {
		map models/players/crusader/clower.tga
                blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingDiffuse
	}
       
}

models/players/crusader/gable_upper
{
	{
		map models/players/crusader/gable_upper.tga
                blendfunc GL_ONE GL_ZERO
                rgbGen lightingDiffuse

        }

	{
		map models/players/crusader/gable_upper_g.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave triangle .5 4 5 .1

	}
}
