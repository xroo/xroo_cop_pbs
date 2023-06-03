#ifndef reflections_h_2134124_inc
#define reflections_h_2134124_inc

// Чтоб отражения выключались на обычной динамике
#if defined(USE_DOF) || defined(USE_SOFT_PARTICLES) || defined(SSAO_QUALITY)
	#define USE_REFLECTIONS
#endif

	// #define SSR_SAMPLES int(40)
	#define SSR_SAMPLES int(8)

static const float2 resolution = pos_decompression_params.xy;
static const float2 inv_resolution = pos_decompression_params.zw;

float4 proj_to_screen(float4 proj)
{
	float4 screen = proj;
	screen.x = (proj.x + proj.w);
	screen.y = (proj.w - proj.y);
	screen.xy *= 0.5;
	return screen;
}

float	get_depth_fast			(float2 tc)
{
#ifndef USE_MSAA
	return s_position.SampleLevel( smp_nofilter, tc, 0).z;
#else
	return s_position.Load( int3( tc * pos_decompression_params2.xy ,0), 0 ).z;
#endif
}

float3 gbuf_unpack_position(float2 uv)
{
	float	depth		=  get_depth_fast(uv); uv = uv * 2.f - 1.f;
			return		float3(uv * pos_decompression_params.xy, 1.f) * (depth > 0.01 ? depth : 1000.0);
}

float2 gbuf_unpack_uv(float3 position)
{
			position.xy	/= pos_decompression_params.xy * position.z;
			return		saturate(position.xy * 0.5 + 0.5);
}

float GetBorderAtten(float2 tc, float att)
{
	return	smoothstep(0.0, att, min(1.f-tc.y, tc.y) * min(1.f-tc.x, tc.x));
}

float4 ScreenSpaceLocalReflections (float3 Point, float3 Reflect)
{
	float2	ReflUV	= 0.0;
	float3	HitPos, TestPos;
	float	L		= 1.0 / float(SSR_SAMPLES);
	
	int		i		= 0;
	while(i++ <= SSR_SAMPLES) {
		TestPos		= Point + Reflect * L;
		ReflUV		= gbuf_unpack_uv(TestPos);
		HitPos		= gbuf_unpack_position(ReflUV);
		L			= length(Point - HitPos);
	}
	
	float3	Normal		= normalize(Reflect - normalize(Point));	
	float	Fade		= smoothstep(0.0, 0.15, length(HitPos) - length(Point));
			Fade		*= step(0.0, dot(normalize(HitPos - Point), Normal));
			Fade		*= GetBorderAtten(ReflUV, 0.025);
	
	float	Fog			= saturate (length(HitPos) * fog_params.w + fog_params.x);
	float	Skyblend	= 1.f - saturate (Fog*Fog);

			return		float4(ReflUV, Skyblend, Fade);
}
#endif