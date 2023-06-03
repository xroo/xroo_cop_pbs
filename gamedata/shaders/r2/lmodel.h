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
/*
#ifndef	LMODEL_H
#define LMODEL_H

#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Lighting formulas			// 
half4 	plight_infinity		(half m, half3 point, half3 normal, half3 light_direction)       				{
  half3 N		= normal;							// normal 
  half3 V 		= -normalize	(point);					// vector2eye
  half3 L 		= -light_direction;						// vector2light
  half3 H		= normalize	(L+V);						// half-angle-vector 
  return tex3D 		(s_material,	half3( dot(L,N), dot(H,N), m ) );		// sample material
}
half4 	plight_infinity2	(half m, half3 point, half3 normal, half3 light_direction)       				{
  	half3 N		= normal;							// normal 
  	half3 V 	= -normalize		(point);		// vector2eye
  	half3 L 	= -light_direction;					// vector2light
 	half3 H		= normalize			(L+V);			// half-angle-vector 
	half3 R     = reflect         	(-V,N);
	half 	s	= saturate(dot(L,R));
			s	= saturate(dot(H,N));
	half 	f 	= saturate(dot(-V,R));
			s  *= f;
	half4	r	= tex3D 			(s_material,	half3( dot(L,N), s, m ) );	// sample material
			r.w	= pow(saturate(s),4);
  	return	r	;
}
half4 	plight_local		(half m, half3 point, half3 normal, half3 light_position, half light_range_rsq, out float rsqr)  {
  half3 N		= normal;							// normal 
  half3 L2P 	= point-light_position;                         		// light2point 
  half3 V 		= -normalize	(point);					// vector2eye
  half3 L 		= -normalize	((half3)L2P);					// vector2light
  half3 H		= normalize	(L+V);						// half-angle-vector
		rsqr	= dot		(L2P,L2P);					// distance 2 light (squared)
  half  att 	= saturate	(1 - rsqr*light_range_rsq);			// q-linear attenuate
  half4 light	= tex3D		(s_material, half3( dot(L,N), dot(H,N), m ) ); 	// sample material
  return att*light;
}

half4	blendp	(half4	value, float4 	tcp)    		{
	#ifndef FP16_BLEND  
		value 	+= (half4)tex2Dproj 	(s_accumulator, tcp); 	// emulate blend
	#endif
	return 	value	;
}
half4	blend	(half4	value, float2 	tc)			{
	#ifndef FP16_BLEND  
		value 	+= (half4)tex2D 	(s_accumulator, tc); 	// emulate blend
	#endif
	return 	value	;
}

#endif
*/