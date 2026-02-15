#pragma once

/**
 * UE_LOG wrapper for Cling interpreter environment.
 * 
 * Problem: UE_LOG macro expands to code with static variables in a scope.
 * In function scope, each call has independent static variables.
 * But in Cling's global execution context, static variables are shared globally,
 * causing all UE_LOG calls to output the same content.
 * 
 * Solution: Wrap the UE_LOG call in a lambda to create an independent function scope.
 */
#undef UE_LOG
#define UE_LOG(CategoryName, Verbosity, Format, ...) \
[&](){UE_PRIVATE_LOG(PREPROCESSOR_NOTHING, constexpr, CategoryName, Verbosity, Format, ##__VA_ARGS__)}();

