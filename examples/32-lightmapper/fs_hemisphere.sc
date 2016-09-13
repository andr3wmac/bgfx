$input v_texcoord0

#include "../common/common.sh"

SAMPLER2D(lm_hemispheres, 0);
SAMPLER2D(lm_weights, 1);
uniform vec4 lm_textureSize; // [Hemi W, Hemi H, Weights W, Weights H]

vec4 weightedSample(vec2 h_uv, vec2 w_uv, vec2 quadrant)
{
	vec2 fh_uv 		= h_uv / lm_textureSize.rg;
	vec2 fw_uv 		= w_uv / lm_textureSize.ba;
	vec2 fquadrant 	= quadrant / lm_textureSize.rg;

	vec4 sample = texture2DLod(lm_hemispheres, fh_uv + fquadrant, 0);
	vec2 weight = texture2DLod(lm_weights, fw_uv + fquadrant, 0).rg;

	return vec4(sample.rgb * weight.r, sample.a * weight.g);
}

vec4 threeWeightedSamples(vec2 h_uv, vec2 w_uv, vec2 offset)
{
    // horizontal triple sum
	vec4 sum 	= weightedSample(h_uv, w_uv, offset);
	offset.x 	+= 2.0;
	sum 		+= weightedSample(h_uv, w_uv, offset);
	offset.x 	+= 2.0;
	sum 		+= weightedSample(h_uv, w_uv, offset);

	return sum;
}

void main()
{ 
	// this is a weighted sum downsampling pass (alpha component contains the weighted valid sample count)
	vec2 h_uv 	= gl_FragCoord.xy * vec2(6.0, 2.0) + vec2(0.5, 0.5);
	vec2 w_uv 	= vec2(mod(h_uv, lm_textureSize.ba));

	vec4 lb = threeWeightedSamples(h_uv, w_uv, vec2(0.0, 0.0));
	vec4 rb = threeWeightedSamples(h_uv, w_uv, vec2(1.0, 0.0));
	vec4 lt = threeWeightedSamples(h_uv, w_uv, vec2(0.0, 1.0));
	vec4 rt = threeWeightedSamples(h_uv, w_uv, vec2(1.0, 1.0));

	gl_FragColor = lb + rb + lt + rt;
}
