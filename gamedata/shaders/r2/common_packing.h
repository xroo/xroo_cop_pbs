#ifndef	common_packing_h_included
#define	common_packing_h_included

struct	gbuffer_data
{
	float3	P; 			// position
	half	metalness; 	// metalness
	half3	N; 			// normal
	half	hemi; 		// AO
	half3	C;
	half	gloss;
	half3	C_pac;		// packed into pos.xy
	half	gloss_pac;	// packed into pos.xy
};


half2	gbuf_pack_color( half4 input )
{
	half 	mul 		=  64.0;
			input		=  min( input, 1 - 1e-5 );				// fixing artifacts on white pixels - fixing white color
			input		=  max( input, 1e-5 );					// fixing artifacts on white pixels - fixing white color
    		input 		=  floor 	( input * mul ) / mul;		// making the normalized float more like int
    float4	mul4		=  float4	( mul, 1.0, mul, 1.0 );
    		input 		=  input * mul4;
    		input 		/= mul;
    return	float2 ( input.x + input.y, input.z + input.w );
}
float4	gbuf_unpack_color( float2 input )
{
	half 	mul 		=  64.0;
	float4  mask 		=  float4 	(1.0, mul, 1.0, mul);
	float4	color 		=  input.xxyy * mask;
			color 		=  frac 	(color);
	return	color;
}
float3	gbuf_unpack_position( float z, float2 pos2d )
{
	return	float3( z * ( pos2d * pos_decompression_params.zw - pos_decompression_params.xy ), z );
}

f_deffer pack_gbuffer( float4 norm, float4 pos, float4 col )
{

	f_deffer res;

	res.position	= float4( gbuf_pack_color( col ),	pos.zw );	//	[col, gloss => xy], p.z,	metalness
	res.Ne			= norm;											//	norm,						hemi
	res.C			= col;											//	col,						gloss

	return res;
}

gbuffer_data gbuffer_load_data( float2 tc : TEXCOORD, float2 pos2d )
{
	gbuffer_data gbd;

	float4	P				= tex2D( s_position,	tc );
	float4	N				= tex2D( s_normal,		tc );
	float4	C				= tex2D( s_diffuse,		tc );

			gbd.P 	 		= gbuf_unpack_position(P.z, pos2d);
			gbd.metalness	= P.w;
			gbd.N			= N.xyz;
			gbd.hemi		= N.w;
			gbd.C			= C.xyz;
			gbd.gloss		= C.w;

	float4	P_xy			= gbuf_unpack_color ( P.xy );
			gbd.C_pac		= P_xy.xyz;
			gbd.gloss_pac	= P_xy.w;

	return gbd;

}

#endif