$input v_texcoord0

/*
 * Copyright 2011-2016 Andrew Mac. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

SAMPLER2D(s_lightmap, 0);

void main()
{
    vec4 lightmapColor = texture2D(s_lightmap, v_texcoord0);
	gl_FragColor = vec4(lightmapColor.rgb, 1.0);
}
