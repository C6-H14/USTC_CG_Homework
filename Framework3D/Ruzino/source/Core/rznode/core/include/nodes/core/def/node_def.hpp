#pragma once

#include <nodes/core/node_exec.hpp>
#include <string>

#ifdef _WIN32
#define RUZINO_EXPORT __declspec(dllexport)
#else
#define RUZINO_EXPORT
#endif

// Add a new macro that supports auto - adding student names as a prefix.

#ifndef CGHW_STUDENT_NAME
#define NODE_DEF_OPEN_SCOPE \
    extern "C" {            \
    namespace Ruzino {

#define NODE_DEF_CLOSE_SCOPE \
    }                        \
    }

#define NODE_DECLARATION_FUNCTION(name) \
    RUZINO_EXPORT void node_declare_##name(Ruzino::NodeDeclarationBuilder& b)

#define NODE_EXECUTION_FUNCTION(name) \
    RUZINO_EXPORT bool node_execution_##name(ExeParams params)

#define NODE_DECLARATION_UI(name) \
    RUZINO_EXPORT const char* node_ui_name_##name()

#define CONVERSION_DECLARATION_FUNCTION(from, to)     \
    RUZINO_EXPORT void node_declare_##from##_to_##to( \
        Ruzino::NodeDeclarationBuilder& b)

#define CONVERSION_EXECUTION_FUNCTION(from, to) \
    RUZINO_EXPORT bool node_execution_##from##_to_##to(ExeParams params)

#define CONVERSION_FUNC_NAME(from, to)                                    \
    RUZINO_EXPORT std::string node_id_name_##from##_to_##to()             \
    {                                                                     \
        return "conv_" + std::string(type_name<from>().data()) + "_to_" + \
               std::string(type_name<to>().data());                       \
    }

#define NODE_DECLARATION_REQUIRED(name)       \
    RUZINO_EXPORT bool node_required_##name() \
    {                                         \
        return true;                          \
    }

#define NODE_DECLARATION_ALWAYS_DIRTY(name)       \
    RUZINO_EXPORT bool node_always_dirty_##name() \
    {                                             \
        return true;                              \
    }
#else  // CGHW_STUDENT_NAME is defined!

#define PASTE_HELPER(a, b) a##b
#define PASTE(a, b)        PASTE_HELPER(a, b)

#define NODE_DEF_OPEN_SCOPE \
    extern "C" {            \
    namespace Ruzino {

#define NODE_DEF_CLOSE_SCOPE \
    }                        \
    }

#define NODE_DECLARATION_FUNCTION(name)                                  \
    RUZINO_EXPORT void PASTE(node_declare_##name##_, CGHW_STUDENT_NAME)( \
        Ruzino::NodeDeclarationBuilder & b)

#define NODE_EXECUTION_FUNCTION(name) \
    RUZINO_EXPORT bool PASTE(         \
        node_execution_##name##_, CGHW_STUDENT_NAME)(ExeParams params)

#define NODE_DECLARATION_UI(name) \
    RUZINO_EXPORT const char* PASTE(node_ui_name_##name##_, CGHW_STUDENT_NAME)()

#define CONVERSION_DECLARATION_FUNCTION(from, to)     \
    RUZINO_EXPORT void node_declare_##from##_to_##to( \
        Ruzino::NodeDeclarationBuilder& b)

#define CONVERSION_EXECUTION_FUNCTION(from, to) \
    RUZINO_EXPORT bool node_execution_##from##_to_##to(ExeParams params)

#define CONVERSION_FUNC_NAME(from, to)                                    \
    RUZINO_EXPORT std::string node_id_name_##from##_to_##to()             \
    {                                                                     \
        return "conv_" + std::string(type_name<from>().data()) + "_to_" + \
               std::string(type_name<to>().data());                       \
    }

#define NODE_DECLARATION_REQUIRED(name)                                    \
    RUZINO_EXPORT bool PASTE(node_required_##name##_, CGHW_STUDENT_NAME)() \
    {                                                                      \
        return true;                                                       \
    }

#define NODE_DECLARATION_ALWAYS_DIRTY(name)                                    \
    RUZINO_EXPORT bool PASTE(node_always_dirty_##name##_, CGHW_STUDENT_NAME)() \
    {                                                                          \
        return true;                                                           \
    }

#endif