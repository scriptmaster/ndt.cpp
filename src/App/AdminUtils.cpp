#include "AdminUtils.h"

#ifdef _WIN32
#include <windows.h>
#include <sddl.h>
#else
#include <unistd.h>
#endif

/**
 * AdminUtils implementation
 * Provides admin detection functionality
 */

bool isRunningAsAdmin() {
    #ifdef _WIN32
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                 &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin == TRUE;
    #else
    return geteuid() == 0; // Unix/Linux: root user
    #endif
}
