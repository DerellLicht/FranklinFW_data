//**********************************************************************************
//  Copyright (c) 2026 Derell Licht
//  franklin.h - public declarations for program
//**********************************************************************************

//lint -esym(552, base_len, base_path)
//lint -esym(759, base_len, base_path, calc_max_filename_len)
extern TCHAR base_path[MAX_FILE_LEN+1] ;
extern unsigned base_len ;  //  length of base_path

extern std::unique_ptr<conio_min> console ;

//************************************************************
//lint -esym(1401, ffdata::ft)  member not initialized by constructor

//  DDM: the squiggly-braces after the field names, are non-static data member initializers.
// from n1ghtyunso  06/04/25 on reddit:
// There is even a benefit in using non-static data member initializers.
// For types with multiple constructors, the compiler will make sure that any member 
// not set in the constructor still gets the specified default value.
// When you do this in the default constructor instead, another constructor is still 
// able to leave some members uninitialized.
//lint -esym(768, ffdata::attrib)  global struct member not referenced

struct ffdata {
    DWORD attrib {};
    FILETIME ft {};
    ULONGLONG fsize {};
    std::wstring filename {};
    std::wstring name {};
    std::wstring ext {};
    bool dirflag {} ;
    ffdata(DWORD sattrib, FILETIME sft, ULONGLONG sfsize, std::wstring sfilename, bool sdirflag );
} ;

//  file_handler.cpp
void calc_max_filename_len(ffdata& ftemp);
int parse_data_files(ffdata& ftemp);
uint get_data_list_size(void);
void list_data_elements(void);
void list_grid_IO_by_month(void);



