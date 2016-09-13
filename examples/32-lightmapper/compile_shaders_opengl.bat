shadercDebug -f fs_downsample.sc --bin2c lm_fs_downsample_glsl -o fs_downsample.h -i ../../src --platform linux --type f
shadercDebug -f vs_downsample.sc --bin2c lm_vs_downsample_glsl -o vs_downsample.h -i ../../src --platform linux --type v
shadercDebug -f fs_hemisphere.sc --bin2c lm_fs_hemisphere_glsl -o fs_hemisphere.h -i ../../src --platform linux --type f
shadercDebug -f vs_hemisphere.sc --bin2c lm_vs_hemisphere_glsl -o vs_hemisphere.h -i ../../src --platform linux --type v
shadercDebug -f fs_lightmap.sc -o fs_lightmap.bin -i ../../src --platform linux --type f
shadercDebug -f vs_lightmap.sc -o vs_lightmap.bin -i ../../src --platform linux --type v