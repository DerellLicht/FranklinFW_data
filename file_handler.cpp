//**********************************************************************************
//  Copyright (c) 2026 Daniel D. Miller                       
//  This file does the actual work on each data file
//**********************************************************************************

#include <windows.h>
#include <string>
#include <memory>
#include <vector>
#include <array>
// #include <wchar.h>
#ifdef USE_64BIT
#include <fileapi.h>
#endif

#include "common.h"
#ifndef _lint
#include "conio_min.h"
#endif
#include "franklin.h"

//lint -esym(601, Franklin_data::wstring, yearly_totals_s::uint)  No explicit type for symbol 
//lint -esym(752, month_str)  local declarator not referenced

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
}  //lint !e550 !e1764 !e715

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
struct ymd_s {
   u16 year ;
   u8 month ;
   u8 day ;
} ;

//  ymd_str is in format YYYYMMDD, NULL-term
static int convert_date(ymd_s &ymd_record, std::wstring &date_str)
{
   std::wstring 
   temp = date_str.substr(0, 4);
   ymd_record.year   = (u16) wcstoul((wchar_t *) temp.c_str(), NULL, 10);
   temp = date_str.substr(4, 2);
   ymd_record.month  = (u16) wcstoul((wchar_t *) temp.c_str(), NULL, 10);
   temp = date_str.substr(6, 2);
   ymd_record.day    = (u16) wcstoul((wchar_t *) temp.c_str(), NULL, 10);
   return 0 ;   
}

//************************************************************************
// franklin data\*week.csv -l
//************************************************************************
// deduced-args array declaration requires -std=c++17, 
// but that standard invalidates -wtoi() and swprintf(), which I am using.
// so don't worry about that for now; I know what size my array is.
// std::array month_max_days = {

std::array<int, 13> month_max_days = {
   0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

void list_data_elements(void)
{
   console->dputsf(L"number of data elements: %u\n\n", get_data_list_size());
   
   //  show all data entries
   for(auto &fdtemp : fdlist) {
      console->dputsf(L"%s: H%4.1f S%4.1f BC%4.1f BD%4.1f GI%4.1f GE%4.1f [%5.1f] Gen%4.1f v2l%4.1f\n", 
         fdtemp.date_str.c_str(),
         fdtemp.kWh_home, fdtemp.kWh_solar, fdtemp.kWh_battery_charge, fdtemp.kWh_battery_discharge, 
         fdtemp.kWh_grid_import, fdtemp.kWh_grid_export, 
         fdtemp.kWh_grid_import - fdtemp.kWh_grid_export, 
         fdtemp.kWh_generator, fdtemp.kWh_v2l);
   }

   //  scan again, looking for gaps in date entries
   //  Note:  all of this code is assuming that all of the recovered daily records
   //         are sorted by Y,M,D ... if that ends up not being true, we will
   //         need to run a sort on the list.
   ymd_s prev_date{};
   ymd_s curr_date{};
   // char prev_date[9] {} ;
   for(auto &fdtemp : fdlist) {
      //  this is first pass through loop
      if (prev_date.year == 0) {
         convert_date(prev_date, fdtemp.date_str);
         console->dputsf(L"First date is Y%04u, M%02u, D%02u\n\n", 
            prev_date.year, prev_date.month, prev_date.day);
      }
      else {
         convert_date(curr_date, fdtemp.date_str);
         
         //  first, check for change of year
         if (curr_date.year != prev_date.year) {
            if (prev_date.month != 12  ||  prev_date.day != 31) {
               console->dputsf(L"year %04u ended on month%02u, day%02u\n\n", 
                  prev_date.year, prev_date.month, prev_date.day);
            }
         }
         //  second, check for change of month
         else if (curr_date.month != prev_date.month) {
            uint max_days = month_max_days[prev_date.month] ;
            // tweak max_days for February
            if (prev_date.month == 2) {
               //  this won't be valid for (year % 100) == 0
               if ((prev_date.year % 4) != 0) {
                  max_days = 28 ;
               }
               console->dputsf(L"year %04u: February max days: %02u\n", prev_date.year, max_days);
            }
            
            //  then, just check day against max_days
            if (prev_date.day != max_days) {
               console->dputsf(L"year %04u: month %u ended at day: %02u\n", 
                  prev_date.year, prev_date.month, prev_date.day);
            }
         }
         //  check for consecutive days within month
         else {
            if (curr_date.day != prev_date.day+1) {
               console->dputsf(L"year %04u: month %u: gap detected between days %u and %u\n", 
                  curr_date.year, curr_date.month, prev_date.day, curr_date.day) ;
            }
         }
//  we don't detect July 1 and 2 missing...         
// "Jun 29, 2026",23.7,24.9,0.0,0.0,13.9,15.1,0.0,0.0
// "Jul 3, 2026",18.5,25.0,0.0,0.0,9.9,16.4,0.0,0.0
// "Jul 4, 2026",18.7,23.9,0.0,0.0,10.7,15.9,0.0,0.0
// "Jul 5, 2026",27.3,24.9,0.1,0.0,16.6,14.1,0.0,0.0
         
         prev_date = curr_date ;
      }
   }
   
}

//************************************************************************
// franklin data\*week.csv -g
//************************************************************************
//lint -esym(728, yearly_totals)  Symbol not explicitly initialized
//lint -esym(843, year_idx)  Variable could be declared as const
//lint -esym(551, MAX_YEARS)  Symbol not accessed

//  data output before converting yearly_totals array to <vector>
// D:\SourceCode\Git\FranklinFW_data Yes, Master?? > franklin data\*week.csv -g
// FranklinFW data analyzer, Version 1.00, 32-bit
// show grid import/export by month
// number of data elements: 161
// 
// month: 12/2025       116.1
// month:  1/2026       241.7
// month:  2/2026       137.3
// month:  3/2026       -40.6
// month:  4/2026      -102.5
// month:  5/2026      -139.0
// 
// ==============     =======
// total (2025):        116.1
// total (2026):         96.9
// total [overall]:     213.0

struct yearly_totals_s {
   uint year {};
   double yearly_total {};
   yearly_totals_s (uint iyear, double iyearly_total) ;
} ;

//  constructor with data values specified  
yearly_totals_s::yearly_totals_s (
   uint iyear,
   double iyearly_total
) :
year(iyear),
yearly_total(iyearly_total)
{}

static std::vector<yearly_totals_s> yearly_totals;

void list_grid_IO_by_month(void)
{
   yearly_totals_s *this_year {};
   console->dputsf(L"show grid import/export by month\n");
   console->dputsf(L"number of data elements: %u\n\n", get_data_list_size());
   
   uint curr_year = 0 ;
   uint curr_month = 0 ;
   double grid_import_total = 0.0 ;
   
   for(auto const &fdtemp : fdlist) {
      uint year_idx ;
      uint ymd   = _wtoi(fdtemp.date_str.c_str());
      // uint day   = ymd % 100 ;
      uint ym    = ymd / 100 ;
      uint month = ym % 100 ;
      uint year  = ym / 100 ;
      //  handle very first element
      if (curr_year == 0) {
         curr_year  = year ;
         curr_month = month ;
         grid_import_total = 0.0 ;
         
         yearly_totals.emplace_back(curr_year, 0.0);
         year_idx = yearly_totals.size() - 1 ;
         this_year = &yearly_totals[year_idx] ;
         // console->dputsf(L"starting ym: %2u %4u\n\n", month, year);
      }
      //  if year changed, update annual totals
      //  We can assume that if year changes, month changes as well
      else if (curr_year != year) {
         console->dputsf(L"month: %2u/%4u     %7.1f\n", curr_month, curr_year, grid_import_total);
         this_year->yearly_total += grid_import_total ;
         
         curr_year  = year ;
         curr_month = month ;
         grid_import_total = 0.0 ;
         
         yearly_totals.emplace_back(curr_year, 0.0);
         year_idx  = yearly_totals.size() - 1 ;
         this_year = &yearly_totals[year_idx] ;
      }
      else if (curr_month != month) {
         console->dputsf(L"month: %2u/%4u     %7.1f\n", curr_month, curr_year, grid_import_total);
         this_year->yearly_total += grid_import_total ;
         
         curr_year  = year ;
         curr_month = month ;
         grid_import_total = 0.0 ;
      }
      
      //  update the import/export total
      // grid_import_total += (fdtemp.kWh_grid_import - fdtemp.kWh_grid_export) ;
      grid_import_total += fdtemp.kWh_grid_import ;
      grid_import_total -= fdtemp.kWh_grid_export ;
      // console->dputsf(L"git: %.1f, ginp: %.1f, gexp: %.1f\n", 
      //    grid_import_total, fdtemp.kWh_grid_import, fdtemp.kWh_grid_export) ;
   }  //  for each daily log entry
   
   //  pick up last record
   console->dputsf(L"month: %2u/%4u     %7.1f\n\n", curr_month, curr_year, grid_import_total);
   if (this_year != NULL) {
      this_year->yearly_total += grid_import_total ;
   }
         
   console->dputsf(L"==============     =======\n");
   //  compute and display yearly totals
   double overall_total = 0.0 ;
   for(auto &year_total : yearly_totals) {
      console->dputsf(L"total (%04u):      %7.1f\n", year_total.year, year_total.yearly_total);
      overall_total += year_total.yearly_total ;
   }
   
   console->dputsf(L"total [overall]:   %7.1f\n\n", overall_total);
}
