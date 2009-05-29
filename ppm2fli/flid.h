/* File: flid.h		-*- C -*- 					     */
/* Created by: Alex Knowles (alex@ed.ac.uk) Tue Feb 13 12:11:21 1996	     */
/* Last Modified: Time-stamp: <13 Feb 96 1256 Alex Knowles> 		     */
/* RCS $Id$ */
#ifndef _FLID_H
#define _FLID_H

#ifdef HAVE_SDSC_IM
/* include the sandiego image tool kit library */
#include "im.h"
#else
/* include my cut down version of im.h only if im.h has not been included yet*/
#ifndef __IMH__
#include "flid_im.h"
#endif
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* this is the library routines for the fli decoding library */

#ifdef __cplusplus
extern "C" {
#endif
  
  extern int FLId_open( FILE * );
  extern ImVfb *FLId_image( void );
  extern int FLId_close( void );
  extern ImVfb *FLId_ImVfbAlloc(int width, int height, 
				int fields, int for_image );
  
#ifdef __cplusplus
}
#endif

#endif /* _FLID_H */
