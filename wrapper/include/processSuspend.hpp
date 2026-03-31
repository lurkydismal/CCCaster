#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <tlhelp32.h>

#include <vector>

using processSuspendGuard_t = struct processSuspendGuard {
    processSuspendGuard();
    ~processSuspendGuard();

    processSuspendGuard( const processSuspendGuard& ) = default;
    processSuspendGuard( processSuspendGuard&& ) = delete;
    auto operator=( const processSuspendGuard& )
        -> processSuspendGuard& = default;
    auto operator=( processSuspendGuard&& ) -> processSuspendGuard& = delete;

private:
    std::vector< HANDLE > _suspendedThreadHandles;
};
