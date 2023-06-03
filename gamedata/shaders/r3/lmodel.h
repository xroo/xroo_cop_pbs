#ifndef	LMODEL_H
#define LMODEL_H

#include "common.h"
#include "pbs.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Lighting formulas			// 
float4 plight_infinity( float4 diff, float4 pnt, float4 normal, float3 light_direction )
{
	float 	metalness 	=  pnt.w * pnt.w;
	float 	gloss 		=  diff.w;
	float 	roughness	=  1 - gloss;
	float4 	light 		=  direct_light	(roughness, pnt.xyz, normal.xyz, light_direction).xxxy;
			diff 		*= diff;
			light.rgb 	=  lerp(light.rgb * diff.rgb + getF0(pnt.xyz, normal.xyz, roughness, dielectric) * light.a, light.a * diff.rgb * 2, metalness);
			return 			   light;
}

float4 plight_local( float4 diff, float4 pnt, float4 normal, float3 light_position, float light_range_rsq, out float rsqr )
{
	float 	metalness 	=  pnt.w * pnt.w;
	float 	gloss 		=  diff.w;
	float 	roughness	=  1 - gloss;
			light_range_rsq		*=  0.92; 									// Fixing the light range for some reason

	float3	L2P 		=  pnt.xyz - light_position;						// light2point 
			rsqr		=  dot 			(L2P,L2P);							// distance 2 light (squared)
	float  	att 		=  saturate		(1 - rsqr * light_range_rsq);		// q-linear attenuate
	float4 	light		=  direct_light	(roughness, pnt.xyz, normal.xyz, normalize(L2P)).xxxy;
			light		=  light * att * att;
			diff 		*= diff;
			light.rgb 	=  lerp(light.rgb * diff.rgb + getF0(pnt.xyz, normal.xyz, roughness, dielectric) * light.a, light.a * diff.rgb * 2, metalness);

			return 			   light;
}

//	TODO: DX10: Remove path without blending
float4 blendp( float4 value, float4 tcp)
{
//	#ifndef FP16_BLEND  
//		value 	+= (float4)tex2Dproj 	(s_accumulator, tcp); 	// emulate blend
//	#endif
	return 	value;
}

float4 blend( float4 value, float2 tc)
{
//	#ifndef FP16_BLEND  
//		value 	+= (float4)tex2D 	(s_accumulator, tc); 	// emulate blend
//	#endif
	return 	value;
}

#endif