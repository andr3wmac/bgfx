shadercDebug -f fs_downsample.sc -o fs_downsample.h --bin2c lm_fs_downsample_dx9 -i ../../src --platform windows -p ps_3_0 --type f
shadercDebug -f vs_downsample.sc -o vs_downsample.h --bin2c lm_vs_downsample_dx9 -i ../../src --platform windows -p vs_3_0 --type v
shadercDebug -f fs_hemisphere.sc -o fs_hemisphere.h --bin2c lm_fs_hemisphere_dx9 -i ../../src --platform windows -p ps_3_0 --type f
shadercDebug -f vs_hemisphere.sc -o vs_hemisphere.h --bin2c lm_vs_hemisphere_dx9 -i ../../src --platform windows -p vs_3_0 --type v
shadercDebug -f fs_lightmap.sc -o fs_lightmap.bin -i ../../src --platform windows -p ps_3_0 --type f
shadercDebug -f vs_lightmap.sc -o vs_lightmap.bin -i ../../src --platform windows -p vs_3_0 --type v