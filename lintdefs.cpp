//lint -esym(40, errno)

//lint -esym(18, sprintf, FILETIME)  Symbol redeclared (ellipsis) conflicts with ...
//lint -e534  Ignoring return value of function
//lint -e713  Loss of precision (arg. no. 2) (unsigned int to long)
//lint -e737  Loss of sign in promotion from int to unsigned long
//lint -e754  local struct member not referenced
//lint -e56   Bad type

//  deal with ambiguities in MediaInfoDll.cpp
//lint -esym(526, get_mi_info)  //  symbol not defined

//lint -esym(526, _wstat, _wopen, atoi, exit)
//lint -esym(628, _wstat, _wopen, atoi, exit)
//lint -esym(746, _wstat, _wopen, atoi, exit)
//lint -esym(1055, _wstat, _wopen)

//  more common_funcs warnings
//lint -e740   Unusual pointer cast (incompatible indirect types)
//lint -e1786  Implicit conversion to Boolean (arg. no. 1) (int to bool)

//lint -e734   Loss of precision (assignment) (31 bits to 15 bits)
//lint -e732   Loss of sign (arg. no. 3) (int to unsigned long)

//  later C++ alterations
//lint -e745   function has no explicit type or class, int assumed (lint doesn't know uint now)
//lint -esym(728, uint)  Symbol 'uint' not explicitly initialized
//lint -esym(551, uint)  Symbol 'uint' not accessed
//lint -esym(843, uint)  Variable 'uint' (line 37, file franklin.cpp) could be declared as const
//lint -e1075  Ambiguous reference to symbol 'uint' and symbol 'uint'
