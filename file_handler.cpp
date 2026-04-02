//**********************************************************************************
//  Copyright (c) 1998-2026 Daniel D. Miller                       
//  This file does the actual work on each file
//**********************************************************************************

#include <windows.h>
#include <string>
#include <memory>
#include <tchar.h>
#ifdef USE_64BIT
#include <fileapi.h>
#endif

#include "common.h"
#ifndef _lint
#include "conio_min.h"
#endif
#include "franklin.h"

static uint max_filename_len = 0 ;
//************************************************************************
//lint -esym(759, analyze_franklin_data) header declaration for symbol could be moved from header to module
void calc_max_filename_len(ffdata& ftemp)
{
   // analyze_franklin_data(file);
   uint flen = ftemp.filename.length();
   if (max_filename_len < flen) {
      max_filename_len = flen ;
   }
}  //lint !e550 !e1764

//************************************************************************
//lint -esym(759, analyze_franklin_data) header declaration for symbol could be moved from header to module
int analyze_franklin_data(ffdata& ftemp)
{
   ffdata *fptr = &ftemp ;

   //  display directory entry
   if (fptr->dirflag) {
      console->dputsf(L"[%s]\n", fptr->filename.c_str());
   }
   //  display file entry
   else {
      // console->dputsf(L"%-*s ", max_filename_len, fptr->filename.c_str());
      // console->dputsf(L"|%14s\n", convert_to_commas(fptr->fsize, NULL));
      size_t ext_dot = fptr->filename.find_last_of(L".");
      // if (ext_dot == std::wstring::npos) {
      //    console->dputsf(L"%d: %s: %s\n", ext_dot, fptr->filename.c_str(), get_system_message());
      // }
      // else 
      //  if ext_dot == npos (i.e. -1), then no extension is present
      //  if ext_dot == 0, then dot is at start of string, treat as no extension
      if (ext_dot == 0  ||  ext_dot == std::wstring::npos) {
         fptr->name = fptr->filename;
         fptr->ext = {};
      }
      else {
         fptr->name = fptr->filename.substr(0, ext_dot);
         fptr->ext  = fptr->filename.substr(ext_dot);
      }
      console->dputsf(L"%d: %s: [%s][%s], %u\n", 
         ext_dot, fptr->filename.c_str(), fptr->name.c_str(), fptr->ext.c_str(), fptr->fsize);
   }
   return 0 ;  //lint !e438
}  //lint !e550
