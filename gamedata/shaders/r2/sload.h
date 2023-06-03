#ifndef SLOAD_H
#define SLOAD_H

#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Bumped surface loader                //
//////////////////////////////////////////////////////////////////////////////////////////
struct	surface_bumped
{
	half4	base;
	half3	normal;
	half	gloss;
	half	height;
	half	metalness;
	half	occlusion;

};

float4 tbase( float2 tc )
{
	return	tex2D( s_base, tc);
}

#if defined(ALLOW_STEEPPARALLAX) && defined(USE_STEEPPARALLAX)

static const float fParallaxStartFade = 8.0f;
static const float fParallaxStopFade = 12.0f;

void UpdateTC( inout p_bumped I)
{
	if (I.position.z < fParallaxStopFade)
	{
		const float maxSamples = 25;
		const float minSamples = 5;
		const float fParallaxOffset = -0.013;

		float3	 eye = mul (float3x3(I.M1.x, I.M2.x, I.M3.x,
									 I.M1.y, I.M2.y, I.M3.y,
									 I.M1.z, I.M2.z, I.M3.z), -I.position.xyz);
		
		eye = normalize(eye);
		
		//	Calculate number of steps
		float nNumSteps = lerp( maxSamples, minSamples, eye.z );

		float	fStepSize			= 1.0 / nNumSteps;
		float2	vDelta				= eye.xy * fParallaxOffset*1.2;
		float2	vTexOffsetPerStep	= fStepSize * vDelta;

		//	Prepare start data for cycle
		float2	vTexCurrentOffset	= I.tcdh;
		float	fCurrHeight			= 0.0;
		float	fCurrentBound		= 1.0;
/*
		for( int i=0; i<nNumSteps; ++i )
		{
			if (fCurrHeight < fCurrentBound)
			{	
				vTexCurrentOffset += vTexOffsetPerStep;		
				fCurrHeight = s_bumpX.SampleLevel( smp_base, vTexCurrentOffset.xy, 0 ).a; 
				fCurrentBound -= fStepSize;
			}
		}
*/

		//[unroll(25)]	//	Doesn't work with [loop]
		for( ;fCurrHeight < fCurrentBound; fCurrentBound -= fStepSize )
		{
			vTexCurrentOffset += vTexOffsetPerStep;		
			fCurrHeight = tex2Dlod( s_bumpX, float4(vTexCurrentOffset.xy,0,0) ).a; 
		}

		//	Reconstruct previouse step's data
		vTexCurrentOffset -= vTexOffsetPerStep;
		float fPrevHeight = tex2D( s_bumpX, float3(vTexCurrentOffset.xy,0) ).a;

		//	Smooth tc position between current and previouse step
		float	fDelta2 = ((fCurrentBound + fStepSize) - fPrevHeight);
		float	fDelta1 = (fCurrentBound - fCurrHeight);
		float	fParallaxAmount = (fCurrentBound * fDelta2 - (fCurrentBound + fStepSize) * fDelta1 ) / ( fDelta2 - fDelta1 );
		float	fParallaxFade 	= smoothstep(fParallaxStopFade, fParallaxStartFade, I.position.z);
		float2	vParallaxOffset = vDelta * ((1- fParallaxAmount )*fParallaxFade);
		float2	vTexCoord = I.tcdh + vParallaxOffset;
	
		//	Output the result
		I.tcdh = vTexCoord;

#if defined(USE_TDETAIL) && defined(USE_STEEPPARALLAX)
		I.tcdbump = vTexCoord * dt_params;
#endif
	}

}

#elif	defined(USE_PARALLAX) || defined(USE_STEEPPARALLAX)

void UpdateTC( inout p_bumped I)
{
	float3	 eye = mul (float3x3(I.M1.x, I.M2.x, I.M3.x,
								 I.M1.y, I.M2.y, I.M3.y,
								 I.M1.z, I.M2.z, I.M3.z), -I.position.xyz);
	
	half	height	= tex2D( s_bumpX, I.tcdh).w;	//
			//height  /= 2;
			//height  *= 0.8;
			height	= height*(parallax.x) + (parallax.y);	//
	float2	new_tc  = I.tcdh + height * normalize(eye);	//

	//	Output the result
	I.tcdh.xy = new_tc;
}

#else	//	USE_PARALLAX

void UpdateTC( inout p_bumped I)
{
	;
}

#endif	//	USE_PARALLAX

surface_bumped sload_i( p_bumped I)
{
	surface_bumped	S;

	UpdateTC(I);	//	All kinds of parallax are applied here.

	half4 	Nu			= tex2D( s_bump, I.tcdh);		// IN:	normal.gloss
	half4 	NuE			= tex2D( s_bumpX, I.tcdh);	// IN:	normal_error.height

			S.normal	= (Nu.wzy + NuE.xyz - 1.0h) * 2;	//	(Nu.wzyx - .5h) + (E-.5)
			S.normal.xy	+= half2(-10, -6) / 255; // Texture artifacts compensation. It has impact on SSLR!

			S.base		= tbase(I.tcdh);				//	IN:  rgb.a
			S.metalness	= Nu.x;
			S.gloss 	= 1 - Nu.y; 							// from roughness map to gloss
			S.occlusion	= NuE.z * 2;
			S.height	= NuE.w;

#ifdef        USE_TDETAIL
#ifdef        USE_TDETAIL_BUMP
	half4 NDetail		= tex2D( s_detailBump, I.tcdbump);
	half4 NDetailX		= tex2D( s_detailBumpX, I.tcdbump);
			S.normal	+= (NDetail.wzy + NDetailX.xyz - 1.0h) * 2; //	(Nu.wzyx - .5h) + (E-.5)
			S.normal.xy	+= half2(-10, -6) / 255; // Texture artifacts compensation. It has impact on SSLR!
#endif        //	USE_TDETAIL_BUMP
	half4 detail		= tex2D( s_detail, I.tcdbump);
			S.base.rgb	= S.base.rgb * detail.rgb * 2;
			S.gloss		= S.gloss * detail.a * 2;
#endif
			S.normal.z 	= sqrt(saturate(1.0 - S.normal.x * S.normal.x - S.normal.y * S.normal.y));
			// S.normal	= half3(0, 0, 1);

	return S;
}

surface_bumped sload ( p_bumped I)
{
        surface_bumped      S   = sload_i	(I);

#if defined(ALLOW_STEEPPARALLAX) && defined(USE_STEEPPARALLAX)
//		S.base.yz = float2(0,0);
#endif

        return              S;
}

#endif