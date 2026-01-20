#ifndef ADMINUTILS_H
#define ADMINUTILS_H

/**
 * AdminUtils - Admin-related utility functions
 * 
 * Single responsibility: Provide admin detection functionality
 * Extracted from display/admin.h for use by AppHost
 */

/**
 * Check if the application is running with administrator privileges
 * @return true if running as admin, false otherwise
 */
bool isRunningAsAdmin();

#endif // ADMINUTILS_H
