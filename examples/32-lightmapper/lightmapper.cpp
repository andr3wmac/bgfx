/*
 * Copyright 2016 Andrew Mac. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "common.h"
#include "camera.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"

#define LIGHTMAPPER_IMPLEMENTATION
#include "lightmapper_bgfx.h"

struct PosUVVertex
{
	float m_x;
	float m_y;
	float m_z;
	float m_u;
	float m_v;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	}

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosUVVertex::ms_decl;

typedef struct {
	float p[3];
	float t[2];
} vertex_t;

static int loadSimpleObjFile(const char *filename, vertex_t **vertices, unsigned int *vertexCount, unsigned short **indices, unsigned int *indexCount)
{
	FILE *file = fopen(filename, "rt");
	if (!file)
		return 0;
	char line[1024];

	// first pass
	unsigned int np = 0, nn = 0, nt = 0, nf = 0;
	while (!feof(file))
	{
		fgets(line, 1024, file);
		if (line[0] == '#') continue;
		if (line[0] == 'v')
		{
			if (line[1] == ' ') { np++; continue; }
			if (line[1] == 'n') { nn++; continue; }
			if (line[1] == 't') { nt++; continue; }
			assert(!"unknown vertex attribute");
		}
		if (line[0] == 'f') { nf++; continue; }
		assert(!"unknown identifier");
	}
	assert(np && np == nn && np == nt && nf); // only supports obj files without separately indexed vertex attributes

											  // allocate memory
	*vertexCount = np;
	*vertices = (vertex_t*)calloc(np, sizeof(vertex_t));
	*indexCount = nf * 3;
	*indices = (unsigned short*)calloc(nf * 3, sizeof(unsigned short));

	// second pass
	fseek(file, 0, SEEK_SET);
	unsigned int cp = 0, ct = 0, cf = 0;
	while (!feof(file))
	{
		fgets(line, 1024, file);
		if (line[0] == '#') continue;
		if (line[0] == 'v')
		{
			if (line[1] == ' ') { float *p = (*vertices)[cp++].p; char *e1, *e2; p[0] = (float)strtod(line + 2, &e1); p[1] = (float)strtod(e1, &e2); p[2] = (float)strtod(e2, 0); continue; }
			if (line[1] == 'n') { /*float *n = (*vertices)[cn++].n; char *e1, *e2; n[0] = (float)strtod(line + 3, &e1); n[1] = (float)strtod(e1, &e2); n[2] = (float)strtod(e2, 0);*/ continue; } // no normals needed
			if (line[1] == 't') { float *t = (*vertices)[ct++].t; char *e1;      t[0] = (float)strtod(line + 3, &e1); t[1] = (float)strtod(e1, 0);                                continue; }
			assert(!"unknown vertex attribute");
		}
		if (line[0] == 'f')
		{
			unsigned short *tri = (*indices) + cf;
			cf += 3;
			char *e1, *e2, *e3 = line + 1;
			for (int i = 0; i < 3; i++)
			{
				unsigned long pi = strtoul(e3 + 1, &e1, 10);
				assert(e1[0] == '/');
				unsigned long ti = strtoul(e1 + 1, &e2, 10);
				assert(e2[0] == '/');
				unsigned long ni = strtoul(e2 + 1, &e3, 10);
				assert(pi == ti && pi == ni);
				tri[i] = (unsigned short)(pi - 1);
			}
			continue;
		}
		assert(!"unknown identifier");
	}

	fclose(file);
	return 1;
}

class ExampleLightmapper : public entry::AppI
{
	void init(int _argc, char** _argv) BX_OVERRIDE
	{
		PosUVVertex::init();

		Args args(_argc, _argv);

		m_width					= 1280;
		m_height				= 720;
		m_debug					= BGFX_DEBUG_TEXT;
		m_reset					= BGFX_RESET_NONE;
		m_bake					= false;
		m_exportLightmap		= false;
		m_lightmapSkyColor[0]	= 1.0f;
		m_lightmapSkyColor[1]	= 1.0f;
		m_lightmapSkyColor[2]	= 1.0f;
		m_showSkyColorWheel		= true;

		bgfx::init(bgfx::RendererType::Direct3D11);
		bgfx::reset(m_width, m_height, m_reset);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
				);

		// Create program from shaders.
		m_program = loadProgram("vs_lightmap", "fs_lightmap");

		// Create lightmap
		m_lightmap = bgfx::createTexture2D(654, 654, false, 1, bgfx::TextureFormat::RGBA8);
		m_lightmapUniform = bgfx::createUniform("s_lightmap", bgfx::UniformType::Int1);

		if (loadSimpleObjFile("TestBoxSimple.obj", &m_vertices, &m_vertexCount, &m_indices, &m_indexCount))
		{
			const bgfx::Memory* vb_mem = bgfx::makeRef(m_vertices, sizeof(vertex_t) * m_vertexCount);
			m_vbh = bgfx::createVertexBuffer(vb_mem, PosUVVertex::ms_decl);

			const bgfx::Memory* ib_mem = bgfx::makeRef(m_indices, sizeof(unsigned short) * m_indexCount);
			m_ibh = bgfx::createIndexBuffer(ib_mem);
		}

		// Imgui.
		imguiCreate();

		// Camera
		cameraCreate();
		const float initialPos[3] = { 0.0f, 0.0f, -15.0f };
		cameraSetPosition(initialPos);
		cameraSetVerticalAngle(0.0f);
	}

	int shutdown() BX_OVERRIDE
	{
		bgfx::destroyIndexBuffer(m_ibh);
		bgfx::destroyVertexBuffer(m_vbh);

		imguiDestroy();

		// Cleanup.
		bgfx::destroyProgram(m_program);
		bgfx::destroyTexture(m_lightmap);
		bgfx::destroyUniform(m_lightmapUniform);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	void drawScene(uint8_t viewID, float view[16], float proj[16])
	{
		bgfx::setViewTransform(viewID, view, proj);

		float mtx[16];
		bx::mtxIdentity(mtx);

		bgfx::setTransform(mtx);
		bgfx::setState(0
			| BGFX_STATE_RGB_WRITE
			| BGFX_STATE_ALPHA_WRITE
			| BGFX_STATE_DEPTH_WRITE
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_CULL_CCW);
		bgfx::setIndexBuffer(m_ibh);
		bgfx::setVertexBuffer(m_vbh);
		bgfx::setTexture(0, m_lightmapUniform, m_lightmap);

		bgfx::submit(viewID, m_program, 0);
	}

	void drawLightmapScene(uint8_t viewID, float view[16], float proj[16])
	{
		bgfx::setViewTransform(viewID, view, proj);

		float mtx[16];
		bx::mtxIdentity(mtx);

		bgfx::setTransform(mtx);
		bgfx::setState(0
			| BGFX_STATE_RGB_WRITE
			| BGFX_STATE_ALPHA_WRITE
			| BGFX_STATE_DEPTH_WRITE
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_CULL_CW);
		bgfx::setIndexBuffer(m_ibh);
		bgfx::setVertexBuffer(m_vbh);
		bgfx::setTexture(0, m_lightmapUniform, m_lightmap);

		bgfx::submit(viewID, m_program, 0);
	}

	int bake()
	{
		lm_context *ctx = lmCreate(32, 0.001f, 100.0f, m_lightmapSkyColor[0], m_lightmapSkyColor[1], m_lightmapSkyColor[2], 1, 100, false);
		if (!ctx)
		{
			fprintf(stderr, "Error: Could not initialize lightmapper.\n");
			return 0;
		}

		int w = 654, h = 654;
		float *data = (float*)calloc(w * h * 4, sizeof(float));
		lmSetTargetLightmap(ctx, data, w, h, 4);

		lmSetGeometry(ctx, NULL,
			LM_FLOAT, (unsigned char*)m_vertices + offsetof(vertex_t, p), sizeof(vertex_t),
			LM_FLOAT, (unsigned char*)m_vertices + offsetof(vertex_t, t), sizeof(vertex_t),
			m_indexCount, LM_UNSIGNED_SHORT, m_indices);

		int vp[4];
		float viewMtx[16], projectionMtx[16];
		uint8_t viewID;
		while (lmBegin(ctx, vp, viewMtx, projectionMtx, &viewID))
		{
			bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
				);
			float progress = ((float)lmProgress(ctx) / (float)m_indexCount) * 100.0f;
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(0, 1, 0x0f, "Baking: %4.2f%%", progress);
			bgfx::touch(0);

			// render to lightmapper framebuffer
			bgfx::setViewRect(viewID, vp[0], vp[1], vp[2], vp[3]);
			drawLightmapScene(viewID, viewMtx, projectionMtx);

			lmEnd(ctx);
		}
		lmDestroy(ctx);

		// postprocess texture
		float *temp = (float*)calloc(w * h * 4, sizeof(float));
		lmImageSmooth(data, temp, w, h, 4);
		lmImageDilate(temp, data, w, h, 4);
		for (int i = 0; i < 16; i++)
		{
			lmImageDilate(data, temp, w, h, 4);
			lmImageDilate(temp, data, w, h, 4);
		}
		lmImagePower(data, w, h, 4, 1.0f / 2.2f, 0x7); // gamma correct color channels
		free(temp);

		// save result to a file
		if (m_exportLightmap)
		{
			lmImageSaveTGAf("result.tga", data, w, h, 4, 1.0f);
		}

		// upload result
		const bgfx::Memory* mem = bgfx::alloc(w * h * 4);
		for (int i = 0; i < (w * h * 4); ++i)
			mem->data[i] = (uint8_t)(data[i] * 255);

		bgfx::destroyTexture(m_lightmap);
		m_lightmap = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA8, 0, mem);

		free(data);
		return 1;
	}

	bool update() BX_OVERRIDE
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
			if (m_bake)
			{
				bake();
				m_bake = false;
			}

			imguiBeginFrame(m_mouseState.m_mx
				, m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				, m_mouseState.m_mz
				, m_width
				, m_height
				);

			imguiBeginArea("Lightmapper", m_width - 260, 10, 250, 315);
			imguiSeparatorLine();
			imguiColorWheel("Sky Color", m_lightmapSkyColor, m_showSkyColorWheel);

			if (imguiCheck("Export Lightmap", m_exportLightmap))
			{
				m_exportLightmap = !m_exportLightmap;
			}

			if (imguiButton("Bake"))
			{
				m_bake = true;
			}

			imguiEndArea();
			imguiEndFrame();

			// Set view 0 default viewport.
			bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

			int64_t now = bx::getHPCounter();
			static int64_t last = now;
			const int64_t frameTime = now - last;
			last = now;
			const double freq = double(bx::getHPFrequency() );
			const double toMs = 1000.0/freq;
			const float deltaTime = float(frameTime / freq);

			// Use debug font to print information about this example.
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/32-lightmapper");
			bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: Ambient occlusion baked into lightmap.");
			bgfx::dbgTextPrintf(0, 3, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

			// Update camera.
			cameraUpdate(deltaTime, m_mouseState);

			float view[16];
			cameraGetViewMtx(view);

			float proj[16];
			bx::mtxProj(proj, 60.0f, float(m_width)/float(m_height), 0.1f, 100.0f);

			// Set view 0 default viewport.
			bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));

			drawScene(0, view, proj);
			bgfx::frame();

			return true;
		}

		return false;
	}

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;

	entry::MouseState m_mouseState;
	bgfx::TextureHandle m_lightmap;
	bgfx::UniformHandle m_lightmapUniform;
	bgfx::ProgramHandle m_program;
	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh;

	vertex_t *m_vertices;
	unsigned short *m_indices;
	unsigned int m_vertexCount;
	unsigned int m_indexCount;
	bool m_bake;
	bool m_exportLightmap;
	float m_lightmapSkyColor[3];
	bool m_showSkyColorWheel;
};

ENTRY_IMPLEMENT_MAIN(ExampleLightmapper);
