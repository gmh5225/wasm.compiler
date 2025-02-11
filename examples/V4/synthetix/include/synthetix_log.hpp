
#pragma once

#include <cstdint>
#include <string>

using namespace std;

#define WASM_FUNCTION_PRINT_LENGTH 50

#define WASM_LOG_PRINT( debug,  ... ) {           \
if( debug ) {                                 \
   std::string str = std::string(__FILE__); \
   str += std::string(":");                 \
   str += std::to_string(__LINE__);         \
   str += std::string(":[");                \
   str += std::string(__FUNCTION__);        \
   str += std::string("]");                 \
   while(str.size() <= WASM_FUNCTION_PRINT_LENGTH) str += std::string(" ");\
   print(str);            \
   print( __VA_ARGS__ ); }}

