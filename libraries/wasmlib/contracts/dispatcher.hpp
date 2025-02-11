#pragma once
#include "inline_transaction.hpp"

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/std_tuple.hpp>

#include <boost/mp11/tuple.hpp>

namespace wasm {

   /**
    * Unpack the received action and execute the correponding action handler
    *
    * @ingroup dispatcher
    * @tparam T - The contract class that has the correponding action handler, this contract should be derived from wasm::contract
    * @tparam Q - The namespace of the action handler function
    * @tparam Args - The arguments that the action handler accepts, i.e. members of the action
    * @param obj - The contract object that has the correponding action handler
    * @param func - The action handler
    * @return true
    */
   template<typename T, typename... Args>
   bool execute_action( regid self, regid code, void (T::*func)(Args...)  ) {
      size_t size = action_data_size();

      //using malloc/free here potentially is not exception-safe, although WASM doesn't support exceptions
      constexpr size_t max_stack_buffer_size = 512;
      void* buffer = nullptr;
      if( size > 0 ) {
         buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);
         read_action_data( buffer, size );
      }

      std::tuple<std::decay_t<Args>...> args;
      datastream<const char*> ds((char*)buffer, size);
      ds >> args;

      T inst(self, code, ds);
      inst.pre_action();
      auto f2 = [&]( auto... a ){
         ((&inst)->*func)( a... );
      };
      inst.post_action();

      boost::mp11::tuple_apply( f2, args );
      if ( max_stack_buffer_size < size ) {
         free(buffer);
      }
      return true;
   }


   /**
    * Unpack the received action and execute the correponding action handler
    *
    * @ingroup dispatcher
    * @tparam T - The contract class that has the correponding action handler, this contract should be derived from wasm::contract
    * @tparam Q - The namespace of the action handler function
    * @tparam Args - The arguments that the action handler accepts, i.e. members of the action
    * @param obj - The contract object that has the correponding action handler
    * @param func - The action handler
    * @return int64_t
    */
   template<typename T, typename... Args>
   int64_t execute_action_with_return( regid self, regid code, int64_t (T::*func)(Args...)  ) {
      size_t size = action_data_size();

      int64_t execute_action_return = -1;

      //using malloc/free here potentially is not exception-safe, although WASM doesn't support exceptions
      constexpr size_t max_stack_buffer_size = 512;
      void* buffer = nullptr;
      if( size > 0 ) {
         buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);
         read_action_data( buffer, size );
      }

      std::tuple<std::decay_t<Args>...> args;
      datastream<const char*> ds((char*)buffer, size);
      ds >> args;

      T inst(self, code, ds);

      auto f2 = [&]( auto... a ){
         execute_action_return = ((&inst)->*func)( a... );
      };

      boost::mp11::tuple_apply( f2, args );
      if ( max_stack_buffer_size < size ) {
         free(buffer);
      }
      return execute_action_return;
   }

  /// @cond INTERNAL

 // Helper macro for WASM_DISPATCH_INTERNAL
 #define WASM_DISPATCH_INTERNAL( r, OP, elem ) \
    case wasm::name( BOOST_PP_STRINGIZE(elem) ).value: \
       wasm::execute_action( wasm::regid(receiver), wasm::regid(code), &OP::elem ); \
       break;

 // Helper macro for WASM_DISPATCH
 #define WASM_DISPATCH_HELPER( TYPE,  MEMBERS ) \
    BOOST_PP_SEQ_FOR_EACH( WASM_DISPATCH_INTERNAL, TYPE, MEMBERS )

/// @endcond

/**
 * Convenient macro to create contract apply handler
 *
 * @ingroup dispatcher
 * @note To be able to use this macro, the contract needs to be derived from wasm::contract
 * @param TYPE - The class name of the contract
 * @param MEMBERS - The sequence of available actions supported by this contract
 *
 * Example:
 * @code
 * WASM_DISPATCH( wasm::bios, (setpriv)(setalimits)(setglimits)(setprods)(reqauth) )
 * @endcode
 */
#define WASM_DISPATCH( TYPE, MEMBERS ) \
extern "C" { \
   [[wasm::wasm_entry]] \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      if( code == receiver ) { \
         switch( action ) { \
            WASM_DISPATCH_HELPER( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: wasm_exit(0); */ \
      } \
   } \
} \

#define WASM_DISPATCH_ANY( TYPE, MEMBERS ) \
extern "C" { \
   [[wasm::wasm_entry]] \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
         switch( action ) { \
            WASM_DISPATCH_HELPER( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: wasm_exit(0); */ \
   } \
} \

}
