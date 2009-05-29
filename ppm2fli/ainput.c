/****************************************************************
 * ainput.c
 ****************************************************************/

/******
  Copyright (C) 1995,1996 by Klaus Ehrenfried. 

  Permission to use, copy, modify, and distribute this software
  is hereby granted, provided that the above copyright notice appears 
  in all copies and that the software is available to all free of charge. 
  The author disclaims all warranties with regard to this software, 
  including all implied warranties of merchant-ability and fitness. 
  The code is simply distributed as it is.
*******/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <memory.h>
#include <stdlib.h>

#include "apro.h"
#define EMPTY_TYPE   -1
#define FBM_TYPE     1000
#define UNKNOWN_TYPE 0
const char fbm_magic[]="bitmap";
static FILE *open_xx_file(char *file_name);

static char *command;
static int max_cmd_len = -1;
static int popen_flag;

/****************************************************************
 * type_detector
 ****************************************************************/

static int detect_type(FILE *fp)
{
  int c, i;

  if(r8_height!=0) return(10); // pure raw r8

  c=fgetc(fp);
  if (c == 'P')
    {
      c=fgetc(fp);
      switch (c)
	{
	case '1': return(1);
	case '2': return(2);
	case '3': return(3);
	case '4': return(4);
	case '5': return(5);
	case '6': return(6);
	default: break;
	}
    }
  if (c == 'R')
    {
      c=fgetc(fp); // A
      if(c=='A'){
	c=fgetc(fp); // W
	if(c=='W'){
	  return(10);
	}
      }
    }
  else if (c == '%')
    {
      for (i=0; i < 7; i++)
	{
	  c = fgetc(fp);
	  if (c != fbm_magic[i]) return(UNKNOWN_TYPE);
	}
      return(FBM_TYPE);
    }
  
  else if (c == EOF)
    {
      fprintf(stderr,"Empty data stream\n");
      return(EMPTY_TYPE);
    }

  //  return(UNKNOWN_TYPE);
  fprintf(stderr,"Assuming UNKNOWN_TYPE is r8 format\n");
  return(10);
}

/****************************************************************
 * check_exist
 ****************************************************************/

int check_exist(char *file_name)
{
  struct stat statbuf;

  if (stat(file_name, &statbuf) == 0)
    {
      return(1);  /* gibt's */
    }

  return(0);  /* ist nicht da */
}

/****************************************************************
 * free_pms
 ****************************************************************/

int free_pms(PMS *image)
{
  if (image->pixels != NULL)
    {
      free(image->pixels);
      image->pixels=NULL;
    }
  return(1);
}

/****************************************************************
 * read_image
 ****************************************************************/

int read_image(PMS *image, char *file_name)
{
  FILE *fpin, *fopen();
  int k, type;
  int length;
  static char old_filter_name[1000];
  static int old_filter_flag;
  static int firsttime=1;

  if(firsttime){
    if(filter_name==NULL){
      filter_name=(char*)malloc(100*sizeof(char));
      if(filter_name==NULL){
	fprintf(stderr,"couldn't get filter_name memory\n");
      }
    }
    strcpy(old_filter_name,filter_name);
    old_filter_flag=filter_flag;
  }

  // auto-detect .gz file type per file
  length=strlen(file_name);
  if(
     (file_name[length-1]=='z')&&
     (file_name[length-2]=='g')&&
     (file_name[length-3]=='.')
     )
    {
      strcpy(filter_name,"gunzip");
      filter_flag=1; // by default
    }
  else{ // then assume NOT gzip file
    filter_flag=0;
  }
  //  fprintf(stderr,"filter: %d %s  %d %s",filter_flag,filter_name,old_filter_flag,old_filter_name); fflush(stderr);

  k=0;
  popen_flag = 0;

  if ((fpin = open_xx_file(file_name)) == NULL)
    { return(0);}


  type = detect_type(fpin);
  //fprintf(stderr,"type: %d ",type); fflush(stderr);

  if (type == EMPTY_TYPE) return(0);
  if (type == UNKNOWN_TYPE)
    {
      fprintf(stderr,"No ppm/pgm/pbm or fbm format\n");
      return(0);
    }
  if (type == FBM_TYPE)
    {
      k=get_fbm_data(image,fpin);
    }
  else if( (type>0)&&(type<=6) )
    {
      k=get_ppm_data(image,fpin,type);
    }
  else if(type==10)
    {
      k=get_r8_data(image,fpin,type);
    }

  if (popen_flag == 0)
    { fclose(fpin);}
  else
    { pclose(fpin);}

  // return variables
  strcpy(filter_name,old_filter_name);
  filter_flag=old_filter_flag;

  firsttime=0;

  return(k);
}

/****************************************************************
 * read_raw
 ****************************************************************/

int read_raw(FILE *fp, char *buffer, int len)
{
  int k;

  if ((k=fread(buffer, 1, len, fp)) != len)
    {
      fprintf(stderr,"DATA INCOMPLETE: Can't read all pixels\n");
      fprintf(stderr,"Expected %d bytes and got only %d\n",len, k);
    }
  return(1);
}

/****************************************************************
 * open_xx_file
 ****************************************************************/

static FILE *open_xx_file(char *file_name)
{
  FILE *fopen(), *fp;
  int cmd_len, type;

  if ((filter_flag == 0) || (test_magic_flag == 1))
    {
      if ((fp = fopen(file_name, "r")) == NULL)
	{
	  fprintf(stderr,"Can't open file '%s'\n",file_name);
	  return(NULL);
	}
      if (filter_flag == 0) return(fp);
      type = detect_type(fp);
      if (type == EMPTY_TYPE) return(NULL);
      if (type == UNKNOWN_TYPE)
	{
	  fclose(fp);
	}
      else
	{
	  rewind(fp);
	  return(fp);
	}
    }

  cmd_len = strlen(filter_name);
  if (cmd_len == 0)
    {
      fprintf(stderr,"No filter specified\n");
      return(NULL);
    }
  cmd_len += strlen(file_name);
  cmd_len += 10; /* for extra chars */

  if (cmd_len >= max_cmd_len)
    {
      cmd_len += 10;
      if (max_cmd_len > 0) free(command);
      if ((command = (char *) malloc(cmd_len)) == NULL)
	{
	  fprintf(stderr,"Can't allocate %d bytes\n",cmd_len);
	  return(NULL);
	}
      max_cmd_len = cmd_len;
    }

  if (filter_flag == 1)
    { sprintf(command,"%s < %s",filter_name,file_name);}
  else
    { sprintf(command,"%s %s",filter_name,file_name);}

  if (verbose_flag > 0)
    fprintf(stdout," .... %s\n",command);

  if ((fp = popen(command, "r")) == NULL)
    {
      fprintf(stderr,"Can't open file via pipe '%s'\n",command);
      return(NULL);
    }
  popen_flag = 1;

  return(fp);
}

/**** -- FIN -- *****/
