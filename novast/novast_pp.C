/* ======== IGeoS ===== Distribition full ===== IGeoS =========
**                                                                         
** novast_pp.C  : Created by 'AMSS-developer' on Wed Sep 30 19:29:02 2015
**                                                                         
** Product      : IGeoS - Integrated Geoscience Software
**                                                                         
** Description  : System for seismic, well log, and potential-field data analysis
**                                                                         
** =================== Limited License: ===================================
** 
** This software is provided free under the following terms and conditions:
** 
** 1. Permission to use, copy, and modify this software 
** for non-commercial purposes without fee is hereby granted, provided  
** that this copyright notice, the warranty disclaimer, and this
** permission notice appear in all copies.
** 
** 2. Distribution of this software or any part of it "bundled" in with
** any product is considered to be a 'commercial purpose'.
** 
** 3. Any new or adapted code developed to operate as a part of this
** software shall be contributed to the authors and distributed under
** the same license.
** 
** ================== Warranty Disclaimer: ================================
** 
** This software is provided "as is", with no support and without
** obligation on the part of the author to assist in its use, correction,
** modification, or enhancement. No guarantees or warranties,
** either express or implied, and regarding the accuracy, safety, or
** fitness for any particular purpose are provided by any contributor
** to this software package.
** 
** ======== IGeoS ===== Distribition full ===== IGeoS ========= */


#include "sia_module.C.h"

extern "C"
{
boolean novast_procp_ ();
}

boolean novast_procp_ ()
{
  return SIA.input()->pass_trace(SIA.output()) != NULL;
}
