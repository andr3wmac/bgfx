$input v_texcoord0

#include "../common/common.sh"

SAMPLER2D(lm_hemispheres, 0);
uniform vec4 lm_textureSize; // [Hemi W, Hemi H]

void main()
{
	vec2 h_uv 		= vec2((gl_FragCoord.xy - vec2(0.25, 0.25)) * 2.0) / lm_textureSize.xy;
	vec2 texelSize 	= vec2(1.0 / lm_textureSize.x, 1.0 / lm_textureSize.y);

	vec4 lb = texture2DLod(lm_hemispheres, h_uv + vec2(0.0, 		0.0), 0.0);
	vec4 rb = texture2DLod(lm_hemispheres, h_uv + vec2(texelSize.x, 0.0), 0.0);
	vec4 lt = texture2DLod(lm_hemispheres, h_uv + vec2(0.0, 		texelSize.y), 0.0);
	vec4 rt = texture2DLod(lm_hemispheres, h_uv + vec2(texelSize.x, texelSize.y), 0.0);

	gl_FragColor = lb + rb + lt + rt;
}