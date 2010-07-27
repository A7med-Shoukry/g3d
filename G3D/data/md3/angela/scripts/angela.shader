models/players/angela/default_1
{
	{
		map models/players/angela/default_enviro1.jpg
		tcgen environment
		tcMod rotate 35
		rgbGen lightingDiffuse
	}
	{
		map models/players/angela/default_1.tga
		rgbGen lightingDiffuse
		blendfunc GL_ONE GL_SRC_ALPHA
	}
	{
		map models/players/angela/default_self.jpg
		blendfunc add
		rgbGen wave triangle 0 1 0 .2
	}
}
models/players/angela/default_2
{
		cull none
	{
		map models/players/angela/default_2.tga
		alphaFunc GT0
		depthWrite
		alphagen identity
	}
	{
		map models/players/angela/default_enviro3.jpg
		tcgen environment
		tcMod rotate 10
		depthFunc equal
		rgbGen lightingDiffuse
	}
	{
		map models/players/angela/default_2.tga
		depthFunc equal
		rgbGen lightingDiffuse
		blendfunc GL_ONE GL_SRC_ALPHA
	}
}
models/players/angela/default_3
{
		deformVertexes wave 1 sin 0 .2 0 .5
	{
		map models/players/angela/default_enviro2.jpg
		tcgen environment
		rgbGen lightingDiffuse
	}
	{
		map models/players/angela/default_3.tga
		depthWrite
		rgbGen lightingDiffuse
		blendfunc GL_ONE GL_SRC_ALPHA
	}
}
models/players/angela/blue_1
{
	{
		map models/players/angela/blue_e.jpg
		tcgen environment
		tcMod scroll 0.2 0.5
		rgbGen identity
	}
	{
		map models/players/angela/blue_1.tga
		rgbGen lightingDiffuse
		blendfunc GL_ONE GL_SRC_ALPHA
	}
	{
		map models/players/angela/blue_s.jpg
		blendfunc add
		rgbGen wave triangle 0 1 0 .2
	}
}
models/players/angela/blue_2
{
		cull none
	{
		map models/players/angela/blue_2.tga
		alphaFunc GT0
		depthWrite
		alphagen identity
	}
	{
		map models/players/angela/default_enviro3.jpg
		tcgen environment
		tcMod rotate 10
		depthFunc equal
		rgbGen lightingDiffuse
	}
	{
		map models/players/angela/blue_2.tga
		depthFunc equal
		rgbGen lightingDiffuse
		blendfunc GL_ONE GL_SRC_ALPHA
	}
}
models/players/angela/blue_3
{
		deformVertexes wave 1 sin 0 .2 0 .5
	{
		map models/players/angela/default_enviro2.jpg
		tcgen environment
		rgbGen lightingDiffuse
	}
	{
		map models/players/angela/blue_3.tga
		depthWrite
		rgbGen lightingDiffuse
		blendfunc GL_ONE GL_SRC_ALPHA
	}
}
models/players/angela/red_1
{
	{
		map models/players/angela/red_f.jpg
		tcgen environment
		tcMod rotate 40
		rgbGen wave sin 0.5 1.5 0 0.1
	}
	{
		map models/players/angela/red_1.tga
		rgbGen lightingDiffuse
		blendfunc GL_ONE GL_SRC_ALPHA
	}
	{
		map models/players/angela/red_s.jpg
		blendfunc add
		rgbGen wave triangle 0 1 0 .2
	}
}
models/players/angela/red_2
{
		cull none
	{
		map models/players/angela/red_2.tga
		alphaFunc GT0
		depthWrite
		alphagen identity
	}
	{
		map models/players/angela/default_enviro3.jpg
		tcgen environment
		tcMod rotate 10
		depthFunc equal
		rgbGen lightingDiffuse
	}
	{
		map models/players/angela/red_2.tga
		depthFunc equal
		rgbGen lightingDiffuse
		blendfunc GL_ONE GL_SRC_ALPHA
	}
}
models/players/angela/red_3
{
		deformVertexes wave 1 sin 0 .2 0 .5
	{
		map models/players/angela/default_enviro2.jpg
		tcgen environment
		rgbGen lightingDiffuse
	}
	{
		map models/players/angela/red_3.tga
		depthWrite
		rgbGen lightingDiffuse
		blendfunc GL_ONE GL_SRC_ALPHA
	}
}
