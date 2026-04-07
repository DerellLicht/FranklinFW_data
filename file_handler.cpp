//**********************************************************************************
//  Copyright (c) 1998-2026 Daniel D. Miller                       
//  This file does the actual work on each file
//**********************************************************************************

#include <windows.h>
#include <string>
#include <memory>
#include <wchar.h>
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
static wchar_t const month_str[13][4] = {
   L"Jan",
   L"Feb",
   L"Mar",
   L"Apr",
   L"May",
   L"Jun",
   L"Jul",
   L"Aug",
   L"Sep",
   L"Oct",
   L"Nov",
   L"Dec",
L"" } ;

static uint parse_month(wchar_t *hd)
{
   uint mnum ;
   // console->dputsf(L"%s, seek month\n", hd);
   for (mnum=0; month_str[mnum] != 0; mnum++) {
      if (wcscmp(hd, month_str[mnum]) == 0) {
         return mnum + 1;
      }
   }
   return 0 ;
}

//************************************************************************
//"Mar 23, 2026",16.3,20.6,0.0,0.0,9.2,13.4,0.0,0.0
//  return YYYYMMDD
//  return 0 on error
static uint process_date_string(wchar_t *hd)
{
   uint pds_data = 0;
   // if (*hd != L'"') {
   //    console->dputsf(L"%s, fail test 1\n", hd);
   //    return 0 ;
   // }
   // hd++ ;
   wchar_t *tl = wcschr(hd, L' ');
   if (*tl == NULL) {
      console->dputsf(L"%s, fail test 2\n", hd);
      return 0 ;
   }
   *tl++ = 0 ;  //  NULL-terminate month
   uint month = parse_month(hd);
   if (*tl == NULL) {
      console->dputsf(L"%s, fail test 3 (parse month)\n", hd);
      return 0 ;
   }
   pds_data = month ;
   return pds_data ;
}

//************************************************************************
//lint -esym(759, analyze_franklin_data) header declaration for symbol could be moved from header to module
int analyze_franklin_data(ffdata& ftemp)
{
   ffdata *fptr = &ftemp ;

   //  display directory entry
   if (fptr->dirflag) {
      // console->dputsf(L"[%s]\n", fptr->filename.c_str());
      return 1 ;
   }
   //  display file entry
   
   // // console->dputsf(L"%-*s ", max_filename_len, fptr->filename.c_str());
   // // console->dputsf(L"|%14s\n", convert_to_commas(fptr->fsize, NULL));
   // size_t ext_dot = fptr->filename.find_last_of(L".");
   // // if (ext_dot == std::wstring::npos) {
   // //    console->dputsf(L"%d: %s: %s\n", ext_dot, fptr->filename.c_str(), get_system_message());
   // // }
   // // else 
   // //  if ext_dot == npos (i.e. -1), then no extension is present
   // //  if ext_dot == 0, then dot is at start of string, treat as no extension
   // if (ext_dot == 0  ||  ext_dot == std::wstring::npos) {
   //    fptr->name = fptr->filename;
   //    fptr->ext = {};
   // }
   // else {
   //    fptr->name = fptr->filename.substr(0, ext_dot);
   //    fptr->ext  = fptr->filename.substr(ext_dot);
   // }
   // console->dputsf(L"%d: %s: [%s][%s], %u\n", 
   //    ext_dot, fptr->filename.c_str(), fptr->name.c_str(), fptr->ext.c_str(), fptr->fsize);
   std::wstring filepath = base_path + fptr->filename;
   
   // console->dputsf(L"%s\n", fptr->filename.c_str());
   console->dputsf(L"%s\n", filepath.c_str());
   
   FILE *infd = _wfopen(filepath.c_str(), L"rt");
   if (infd == NULL) {
      console->dputsf(L"%s: %s\n", filepath.c_str(), get_system_message());
      return 1 ;
   }
   
   wchar_t inpstr[MAX_LINE_LEN] ;
   uint lcount = 0 ;
   while (fgetws(inpstr, MAX_LINE_LEN, infd) != NULL) {
      //  read label line
      // 0 Date,
      // 1 Home (kWh),
      // 2 Solar (kWh),
      // 3 aPower(s) Charge (kWh),
      // 4 aPower(s) Discharge (kWh),
      // 5 Grid Import (kWh),
      // 6 Grid Export (kWh),
      // 7 Generator (kWh),
      // 8 V2L (kWh)
      if (lcount == 0) {
         //  don't do anything with label line
      }
      //  read data lines
      //"Mar 23, 2026",16.3,20.6,0.0,0.0,9.2,13.4,0.0,0.0
      else {
         uint field_idx = 0 ;
         wchar_t *hd = inpstr ;
         wchar_t *tl ;
         bool done = false ;
         while (!done) {
            switch (field_idx) {
            case 0: // 0 Date, convert to YYYYMMDD format
               tl = wcschr(hd, L',');  //  first comma is inside date string; ignore this
               if (tl == NULL)  goto error_exit;
               tl = wcschr(tl, L',');  //  second comma is desired terminator
               if (tl == NULL)  goto error_exit;
               
               // clip the date string and process it
               *tl = 0 ;
               hd++ ;   //  skip the opening quote
               
               //  process date string
               {
               uint udate = process_date_string(hd);
               console->dputsf(L"date parse: %s, %u\n", hd, udate);
               if (udate == 0) {
                  done = true ;
               }
               }
               
               hd = tl + 1 ;  //  point head to next data element
               break ;
            case 1: // 1 Home (kWh),
               break ;
            case 2: // 2 Solar (kWh),
               break ;
            case 3: // 3 aPower(s) Charge (kWh),
               break ;
            case 4: // 4 aPower(s) Discharge (kWh),
               break ;
            case 5: // 5 Grid Import (kWh),
               break ;
            case 6: // 6 Grid Export (kWh),
               break ;
            case 7: // 7 Generator (kWh),
               break ;
            case 8: // 8 V2L (kWh)
               break ;
            default:
               done = true ;
               break ;            
            }  //  end switch()
            field_idx++ ;
         }  //  end while()
      }
      lcount++ ;
   }
   fclose(infd);
   return 0 ;  //lint !e438
   
error_exit:
   console->dputsf(L"%s: line %u  Something went wrong\n", fptr->filename.c_str(), lcount);
   return 1 ;   
}  //lint !e550
