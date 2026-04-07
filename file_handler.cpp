//**********************************************************************************
//  Copyright (c) 2026 Daniel D. Miller                       
//  This file does the actual work on each file
//**********************************************************************************

#include <windows.h>
#include <string>
#include <memory>
#include <vector>
// #include <wchar.h>
#ifdef USE_64BIT
#include <fileapi.h>
#endif

#include "common.h"
#ifndef _lint
#include "conio_min.h"
#endif
#include "franklin.h"

//lint -esym(601, Franklin_data::wstring) No explicit type for symbol

static uint max_filename_len = 0 ;

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
struct Franklin_data {
    std::wstring date_str {};
    float kWh_home {};
    float kWh_solar {};
    float kWh_battery_charge {};
    float kWh_battery_discharge {};
    float kWh_grid_import {};
    float kWh_grid_export {};
    float kWh_generator {};
    float kWh_v2l {};
} ;


std::vector<Franklin_data> fdlist ;

//************************************************************************
//lint -esym(759, get_data_list_size)  header declaration could be moved from header to module
uint get_data_list_size(void)
{
   // console->dputsf(L"list size: %u\n", fdlist.size());
   return (uint) fdlist.size();
}

//************************************************************************
//lint -esym(759, parse_data_files) header declaration for symbol could be moved from header to module
void calc_max_filename_len(ffdata& ftemp)
{
   // parse_data_files(file);
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

static uint parse_month(wchar_t const * const hd)
{
   uint mnum ;
   // console->dputsf(L"%s, seek month\n", hd);
   for (mnum=0; month_str[mnum] != 0; mnum++) {
      if (wcsncmp(hd, month_str[mnum], 3) == 0) {
         return mnum + 1;
      }
   }
   return 0 ;
}

//************************************************************************
//"Mar 23, 2026",16.3,20.6,0.0,0.0,9.2,13.4,0.0,0.0
//  return YYYYMMDD
//  return NULL on error
static wchar_t *process_date_string(wchar_t *hd)
{
   static wchar_t date_str[9] = L"" ;
   uint month = parse_month(hd);
   if (month == 0) {
      return NULL ;
   }
   // console->dputsf(L"date parse (month): %s, %u\n", hd, month);
   
   hd = next_field(hd);
   uint day = (uint) _wtoi(hd);
   // console->dputsf(L"date parse (day): %s, %u\n", hd, day);
   
   hd = next_field(hd);
   uint year = (uint) _wtoi(hd);
   // console->dputsf(L"date parse (year): %s, %u\n", hd, year);
   swprintf(date_str, L"%04u%02u%02u", year, month, day);
   
   return date_str ;
}

//************************************************************************
int parse_data_files(ffdata& ftemp)
{
   ffdata *fptr = &ftemp ;

   //  display directory entry
   if (fptr->dirflag) {
      // console->dputsf(L"[%s]\n", fptr->filename.c_str());
      return 1 ;
   }
   std::wstring filepath = base_path + fptr->filename;
   
   // console->dputsf(L"%s\n", filepath.c_str());
   
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
         wchar_t *date_str = NULL ;
         
         fdlist.emplace_back();
         uint idx = fdlist.size() - 1 ;
         Franklin_data *fdtemp = &fdlist[idx] ;
         // filecount++;
         
         bool done = false ;
         while (!done) {
            switch (field_idx) {
            case 0: // 0 Date, convert to YYYYMMDD format
               tl = wcschr(hd, L',');  //  first comma is inside date string; ignore this
               if (tl == NULL)  goto error_exit;
               tl++ ;   //  skip first comma
               tl = wcschr(tl, L',');  //  second comma is desired terminator
               if (tl == NULL)  goto error_exit;
               
               // clip the date string and process it
               // *tl++ = 0 ;
               hd++ ;   //  skip the opening quote
               
               //  process date string
               date_str = process_date_string(hd);
               if (date_str == NULL) {
                  done = true ;
                  break ;
               }
               fdtemp->date_str = date_str ;
               // console->dputsf(L"date parse: %s\n", fdtemp->date_str.c_str());
               // console->dputsf(L"tail: %s", tl);
               
               hd = tl + 1;  //  point head to next data element
               break ;
               
            case 1: // 1 Home (kWh),
               fdtemp->kWh_home = (float) wcstod(hd, NULL);
               // console->dputsf(L"daily consumption: %.1f\n", fdtemp->kWh_home);
               tl = wcschr(hd, L',');  //  second comma is desired terminator
               if (tl == NULL)  goto error_exit;
               hd = tl + 1;  //  point head to next data element
               break ;

            case 2: // 2 Solar (kWh),
               fdtemp->kWh_solar = (float) wcstod(hd, NULL);
               // console->dputsf(L"Solar production: %.1f\n\n", fdtemp->kWh_solar);
               tl = wcschr(hd, L',');  //  second comma is desired terminator
               if (tl == NULL)  goto error_exit;
               hd = tl + 1;  //  point head to next data element
               break ;

            case 3: // 3 aPower(s) Charge (kWh),
               fdtemp->kWh_battery_charge = (float) wcstod(hd, NULL);
               // console->dputsf(L"Solar production: %.1f\n\n", fdtemp->kWh_solar);
               tl = wcschr(hd, L',');  //  second comma is desired terminator
               if (tl == NULL)  goto error_exit;
               hd = tl + 1;  //  point head to next data element
               break ;

            case 4: // 4 aPower(s) Discharge (kWh),
               fdtemp->kWh_battery_discharge = (float) wcstod(hd, NULL);
               // console->dputsf(L"Solar production: %.1f\n\n", fdtemp->kWh_solar);
               tl = wcschr(hd, L',');  //  second comma is desired terminator
               if (tl == NULL)  goto error_exit;
               hd = tl + 1;  //  point head to next data element
               break ;

            case 5: // 5 Grid Import (kWh),
               fdtemp->kWh_grid_import = (float) wcstod(hd, NULL);
               // console->dputsf(L"Solar production: %.1f\n\n", fdtemp->kWh_solar);
               tl = wcschr(hd, L',');  //  second comma is desired terminator
               if (tl == NULL)  goto error_exit;
               hd = tl + 1;  //  point head to next data element
               break ;

            case 6: // 6 Grid Export (kWh),
               fdtemp->kWh_grid_export = (float) wcstod(hd, NULL);
               // console->dputsf(L"Solar production: %.1f\n\n", fdtemp->kWh_solar);
               tl = wcschr(hd, L',');  //  second comma is desired terminator
               if (tl == NULL)  goto error_exit;
               hd = tl + 1;  //  point head to next data element
               break ;

            case 7: // 7 Generator (kWh),
               fdtemp->kWh_generator = (float) wcstod(hd, NULL);
               // console->dputsf(L"Solar production: %.1f\n\n", fdtemp->kWh_solar);
               tl = wcschr(hd, L',');  //  second comma is desired terminator
               if (tl == NULL)  goto error_exit;
               hd = tl + 1;  //  point head to next data element
               break ;

            case 8: // 8 V2L (kWh)
               fdtemp->kWh_v2l = (float) wcstod(hd, NULL);
               // console->dputsf(L"Solar production: %.1f\n\n", fdtemp->kWh_solar);
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

//************************************************************************
void list_data_elements(void)
{
   console->dputsf(L"list size: %u\n\n", get_data_list_size());
   
   for(auto &fdtemp : fdlist)
   {
      console->dputsf(L"%s: H%4.1f S%4.1f BC%4.1f BD%4.1f GI%4.1f GE%4.1f Gen%4.1f v2l%4.1f\n", 
         fdtemp.date_str.c_str(),
         fdtemp.kWh_home, fdtemp.kWh_solar, fdtemp.kWh_battery_charge,
         fdtemp.kWh_battery_discharge, fdtemp.kWh_grid_import, 
         fdtemp.kWh_grid_export, fdtemp.kWh_generator, fdtemp.kWh_v2l);
   }
}

