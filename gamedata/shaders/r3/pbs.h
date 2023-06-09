#ifndef	PBS_H
#define	PBS_H


#define	env_mip_size 		float(9.0h)
#define dielectric 			float(sqrt(0.04h))
#define PI 					float(3.141592653589f)

#define lsun_intensity 		float(1.0h) 				// 1.25h is beautiful, 1.0h is default	
#define llocal_intensity 	float(1.0h)					// 1.0h is beautiful,  1.0h is default
#define env_intensity		float(1.0h)					// 2.0h is beautiful,  1.0h is default
#define amb_intensity		float(1.0h)

#define PBR_USE_SMALLSKY
// #define PBR_USE_FAKE_GROUND_REFLECTION
#define PBR_IRRADIENCE_IN_SSLR

#define SSR_QUALITY	3

// #include "ssr.h"
#include "anonim_reflections.h"

TextureCube		env_s0;
TextureCube		env_s1;
TextureCube		sky_s0;
TextureCube		sky_s1;

uniform float4		env_color;        // color.w  = lerp factor

float3 desaturate( float3 color, float amount )
{
	// float 	grayscale 	= (color.r + color.g + color.b) / 3;
	float 	grayscale 	= (0.2989 * color.r + 0.5870 * color.g + 0.1140 * color.b);
	float3 	result 		= lerp( grayscale, color, amount);
	return 	result;
}

half3 getF0( float3 position, float3 normal, half roughness, half3 specular )
{
	float3	N			=  normalize(normal);						// normal, It has to be normalized, it fixes flickering!
	float3	V			=  -normalize(position);					// vector2eye
	float	NdotV		=  saturate(max(dot(N, V), 1e-5));			// fresnel

	float	roughness2	=  pow(max(roughness,0.04),2.0);
	float 	fresnel 	=  pow(1 - NdotV, 5.0) * (1 - roughness2);
	half3	F0 			=  lerp(specular, 1, fresnel);
	return	F0;
}
half getF0( float3 position, float3 normal, half roughness, half specular )
{
	return	getF0( position, normal, roughness, specular.rrr ).x;
}

float distributionGGX(float NdotH, float roughness)
{
	float 	alpha 		= roughness * roughness;

	float 	denom 		= (NdotH * alpha - NdotH) * NdotH + 1;
			denom 		= PI * denom * denom;

			return 		  alpha / denom;
}

float lambdaSmith(float NdotX, float alpha)
{    
	float 	alpha_sqr 	= alpha * alpha;
	float 	NdotX_sqr 	= NdotX * NdotX;
			return 		  (-1.0 + sqrt(alpha_sqr * (1.0 - NdotX_sqr) / NdotX_sqr + 1.0)) * 0.5;
}

float g2Smith(half NdotV, half NdotL, half alpha)
{

	half 	lambdaV 	=  lambdaSmith(NdotV, alpha);
	half 	lambdaL 	=  lambdaSmith(NdotL, alpha);
			return 		1 / (1 + lambdaV + lambdaL);

}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0,float roughness)
{
	return F0 + (max((1-roughness).xxx,F0)-F0)*pow(1-cosTheta,5);
}

// fED skybox reflection remapping
float3 cubify_normals( float3 normals )
{
	float3	vabs			=  abs(normals);
			normals			/= max(vabs.x, max(vabs.y, vabs.z));
	if (normals.y < 0.999)
			normals.y		=  normals.y * 2 - 1;
	return	normals;
}

// TOO MUCH SAMPLERS for making transition between two mip levels of cubemap
half3 get_skybox_reflection( half sphericalRoughness, float3 vreflect, half lerp_factor )
{
	float	mip_num 	=  sphericalRoughness * env_mip_size;
	float3	e0s0		=  sky_s0.SampleLevel( smp_rtlinear, vreflect, floor (mip_num) ).rgb;
	float3	e0s1		=  sky_s0.SampleLevel( smp_rtlinear, vreflect, ceil  (mip_num) ).rgb;
	float3 	e0s 		=  lerp(e0s0, e0s1, frac(mip_num));
	float3	e1s0		=  sky_s1.SampleLevel( smp_rtlinear, vreflect, floor (mip_num) ).rgb;
	float3	e1s1		=  sky_s1.SampleLevel( smp_rtlinear, vreflect, ceil  (mip_num) ).rgb;
	float3	e1s			=  lerp(e1s0, e1s1, frac(mip_num));
			return		lerp( e0s, e1s, lerp_factor );
}

half3 env_ground( half3 cubemap, float3 normal, half roughness )
{
		 	// roughness 	=  0;
	return				lerp ( cubemap,
							   env_color.rgb * 1/16,
							   saturate( -normal.y / ((1 - (1 - roughness) * 0.75) * 2) + 0.1 ) );
}

half2 direct_light ( float	roughness, float3 position, float3 normal, float3 light_direction )
{
 	float3	N			=  normalize(normal);						// normal, It has to be normalized, it fixes flickering!
 	float3	V			=  -normalize(position);					// vector2eye
 	float3	L			=  -normalize(light_direction);				// vector2light
 	float3	H			=  normalize(L+V);							// float-angle-vector
		 	// roughness 	=  1;

	float	roughness2	=  pow(max(roughness,0.04),2.0);

	float	NdotV		=  saturate(max(dot(N, V), 1e-5));			// fresnel
	float	NdotL		=  saturate(max(dot(N, L), 1e-5));			// lightDir
	float	NdotH		=  saturate(dot(N, H));						// reflected direction to the lightsource
	float	VdotH		=  saturate(dot(V, H));

	float	G2			=  g2Smith(NdotV, NdotL, roughness2);
	float	D			=  distributionGGX(NdotH, roughness2);
	float	F 			=  1 - pow(1.0 - VdotH, 5.0);

	float	num			=  D * G2 * F;
	float	denom		=  4.0 * NdotV;
			denom		=  max(denom, 1e-5);

	// return s_material.Sample( smp_material, float3( dot(L,N), dot(H,N), m ) ).xxxy;		// sample material
	half2	result;
			result.x 	=  max(0.0000, NdotL);
			result.y	=  num / denom;
			return		result;
}

float2 EnvBRDFApprox(float roughness, float NdotV)
{
	float4 c0 = float4(-1,	-0.0275,	-0.572,	0.022);
	float4 c1 = float4( 1,	 0.0425,	  1.04,	-0.04);
	float4 r = roughness*c0 + c1;
	float a004 = min(r.x * r.x,exp2(-9.28*NdotV))*r.x + r.y;
	float2 AB = float2(-1.04,1.04)*a004 + r.zw;
	return AB;
}

// calculating the diffuse sky lighting
half3 ambient_diff
(
	float3 			nw,
	float 			hscale
)
{

			hscale			=  1 - pow(1 - saturate( pow(hscale, 0.6) ), 0.6); 	// some hemi lighting beautification

#ifndef PBR_USE_SMALLSKY
	// Correct diffuse light
	float3	venv		=  cubify_normals(nw);
	float3	e0d			=  sky_s0.SampleLevel( smp_rtlinear, venv, env_mip_size - 0 ).rgb;
	float3	e1d			=  sky_s1.SampleLevel( smp_rtlinear, venv, env_mip_size - 0 ).rgb;
	float3	env_d		=  lerp( e0d, e1d, env_color.w );
			env_d		=  sky_adjust(end_d);
			// env_d 		=  env_ground( env_d, nw );					// fake ground in reflection
			env_d		*= env_color.xyz * env_intensity * 0.5;

			// ambient color beautification: desaturating the env_color in darkest places
			// env_d		*= lerp( desaturate( env_color.xyz, 0.0 ), env_color.xyz, 1 - pow(clamp(1.010 - hscale, 0, 1), 72 ) );
			// env_d 		*= 0.5; // because of bright env_color
			// env_d		*= env_d;

	half3	hdiffuse	=  env_d * hscale + L_ambient.rgb * amb_intensity * 0.5;

#else
	float3	venv		=  nw;
	float3	e0d			=  env_s0.SampleLevel( smp_rtlinear, venv, 0 ).rgb;
	float3	e1d			=  env_s1.SampleLevel( smp_rtlinear, venv, 0 ).rgb;
	float3	env_d		=  lerp( e0d, e1d, env_color.w );
			env_d		=  sky_adjust(env_d);
			env_d		*= env_color.xyz * env_intensity * 0.5;
	half3	hdiffuse	=  env_d * hscale + L_ambient.rgb * amb_intensity;

#endif

			return		hdiffuse;
}

// calculating the specular sky lighting
half3 ambient_skybox
(
	float3		vreflect,
	float 		NdotV,
	half 		roughness
)
{
	half 	gloss		=  1 - roughness;
	half	skyRough	=  sqrt(1 - gloss * gloss);

	float3	env_s 		=  get_skybox_reflection( skyRough, cubify_normals(vreflect), env_color.w );
#ifdef PBR_USE_FAKE_GROUND_REFLECTION
			env_s 		=  env_ground( env_s, vreflect, roughness );
#endif
			env_s		=  sky_adjust(env_s);

	half3	hspecular 	=  env_s;
//	Tryings to compensate mip color distortions
//	half3	tonemapped	=  ((hspecular * half3(113, 104, 92) + half3(132, 133, 140)) / 255 - 0.5 ) * 2;
		//	return 		lerp(hspecular, tonemapped, roughness);
			return		hspecular;
}

float4 calc_sslr ( float3 position, float3 normal, float3 hspecular, float4 pos2d )
{
	float4	sslr 			=  ScreenSpaceLocalReflections( position.xyz, normalize( reflect( normalize(position.xyz), normalize(normal.xyz) ) ) );

#ifndef USE_MSAA
	float3	sslrLight		=  s_accumulator.SampleLevel( smp_rtlinear, sslr.xy, 0).xyz;
#else
	float3	sslrLight		=  s_accumulator.Load( int3(sslr.xy * pos_decompression_params2.xy, 0), 0).xyz; //tex2Dlod(s_image, ReflUV.xyyy);
#endif
			sslrLight		=  sqrt(sslrLight);
#ifdef PBR_IRRADIENCE_IN_SSLR
	gbuffer_data	sslrGBD	=  gbuffer_load_data( GLD_P(sslr.xy, pos2d.xy, 0) );
	float3	sslrNW			=  mul( (float3x3)m_v2w, sslrGBD.N );
	float3	sslrDiffuse		=  ambient_diff(sslrNW,	sslrGBD.hemi);
			sslr.rgb		=  lerp(hspecular, sslrDiffuse * sslrGBD.C_pac + sslrLight, sslr.z);
#else
			sslr.rgb		=  lerp(hspecular, sslrLight, sslr.z);
#endif

			sslr.rgb		=  lerp(hspecular, sslr.rgb, sslr.w);
	// return	float4(normalize( reflect( normalize(position.xyz), normalize(normal.xyz) ) ), 1);
	return	sslr;
}

// final combining of maps and light
float3 combine_image
(
	float4	diffuse,				// diffuse.rgb, metalness
	float4	position,
	float4  normal,
	float4	light,					// diffuse_light.rgb, highlight
	float	occ,
	float4  pos2d
)
{

	half	roughness		=  1 - diffuse.w;
			// roughness		=  0;
	half 	metalness		=  position.w * position.w;
	half	roughness2		=  roughness * roughness;
	float3	nw				=  mul( (float3x3)m_v2w, normal.xyz );		// N - worldNormal
	float	hscale			=  normal.w;						//. *        (.5h + .5h*nw.y);

	// reflection vector
	float3	v2PntL			=  normalize( position.xyz );				// Like Screen Position
	float3	v2Pnt			=  mul( (float3x3)m_v2w, v2PntL );					// -V
	float3	vreflect		=  reflect( v2Pnt, nw );
	
	float 	NdotV			=  dot(nw, (-v2Pnt));
			NdotV			=  saturate(max(NdotV, 0.000001));

	float3	hdiffuse, hspecular;

			hdiffuse 		=  ambient_diff(nw,	hscale) * occ;
			hspecular 		=  ambient_skybox(vreflect, NdotV, roughness);

			// Screen Space Local Reflections. https://habr.com/ru/articles/244367/
	half	sslrGlossCrutch	=  1 - smoothstep(0, 0.5, roughness);
	half4	sslr			=  0;
			if(step(1e-5, sslrGlossCrutch))
				sslr			=  calc_sslr( position.xyz, normal.xyz, hspecular, pos2d );

			hspecular		*= saturate(hscale * 2);
			hspecular 		=  lerp(hspecular, sslr.rgb, sslr.w * sslrGlossCrutch) * occ;
	float2	brdf			= EnvBRDFApprox(roughness2, NdotV);

	// compute all image like dielectric materials. Diffuse part
	float3  dresult;
			dresult 		=  hdiffuse * diffuse.rgb;

			// adding specular part
	float3	dspecuilar		=  hspecular * (fresnelSchlickRoughness(NdotV, dielectric, roughness2) * brdf.x + brdf.y);
			dresult			=  dresult*dresult + dspecuilar*dspecuilar + light.rgb; // All the results have to be added squared
	// compute all image like metalic materials
	float3	mresult 		=  hspecular * (fresnelSchlickRoughness(NdotV, diffuse, roughness2) * brdf.x + brdf.y);
			mresult			=  mresult * mresult + light.rgb;
	// Multiple scattering emulation. Referenced from Unity image results
			mresult			=  lerp(mresult, mresult * 2.6, roughness2);

	float3  result 			=  lerp(dresult, mresult, metalness);

			// result			=  hspecular * (fresnelSchlickRoughness(NdotV, 1, roughness2) * brdf.x + brdf.y);
			// result			=  result * result;
			// result			=  lerp(result, result * 2.6, roughness2);

			result			=  sqrt(result);
			// result			=  hspecular;

			return 			result ;

}

#endif