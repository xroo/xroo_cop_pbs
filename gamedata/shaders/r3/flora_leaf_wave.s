function normal		(shader, t_base, t_second, t_detail, t_bump, t_bumpX)
	shader:begin	("deffer_leaf_bump","deffer_base_aref_bump")
			-- : fog		(false)
			-- : emissive 	(true)
--	shader:sampler	("s_base")      :texture	(t_base)
	shader:dx10texture	("s_base",	t_base)
	shader:dx10texture	("s_bump",	t_base .. "_bump")
	shader:dx10texture	("s_bumpX",	t_base .. "_bump#")

	shader:dx10texture	("s_detail",		t_detail)
	shader:dx10texture	("s_detailBump",	t_detail .. "_bump")
	shader:dx10texture	("s_detailBumpX",	t_detail .. "_bump#")

	shader:dx10sampler	("smp_base")
	shader:dx10stencil	( 	true, cmp_func.always, 
							255 , 127, 
							stencil_op.keep, stencil_op.replace, stencil_op.keep)
	shader:dx10stencil_ref	(1)
end

function l_point  (shader, t_base, t_second, t_detail)
	shader:begin	("deffer_tree_flat","deffer_base_aref_flat")
			: fog		(false)
			-- : aref 		(true,128)
	shader:dx10texture	("s_base", t_base)
	shader:dx10sampler	("smp_base")
	-- shader:dx10stencil	( 	true, cmp_func.always, 
	-- 						255 , 127, 
	-- 						stencil_op.keep, stencil_op.replace, stencil_op.keep)
	-- shader:dx10stencil_ref	(1)
end