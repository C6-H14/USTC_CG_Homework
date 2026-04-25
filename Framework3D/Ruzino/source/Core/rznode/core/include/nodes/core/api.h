
#pragma once

#ifndef RUZINO_NAMESPACE_OPEN_SCOPE
#define RUZINO_NAMESPACE_OPEN_SCOPE namespace Ruzino{
#endif
#define RUZINO_NAMESPACE_CLOSE_SCOPE }

#if defined(_MSC_VER)
#  define NODES_CORE_EXPORT   __declspec(dllexport)
#  define NODES_CORE_IMPORT   __declspec(dllimport)
#  define NODES_CORE_NOINLINE __declspec(noinline)
#  define NODES_CORE_INLINE   __forceinline
#else
#  define NODES_CORE_EXPORT    __attribute__ ((visibility("default")))
#  define NODES_CORE_IMPORT
#  define NODES_CORE_NOINLINE  __attribute__ ((noinline))
#  define NODES_CORE_INLINE    __attribute__((always_inline)) inline
#endif

#if BUILD_NODES_CORE_MODULE
#  define NODES_CORE_API NODES_CORE_EXPORT
#  define NODES_CORE_EXTERN extern
#else
#  define NODES_CORE_API NODES_CORE_IMPORT
#  if defined(_MSC_VER)
#    define NODES_CORE_EXTERN
#  else
#    define NODES_CORE_EXTERN extern
#  endif
#endif
