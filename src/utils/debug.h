#ifndef MUHORS_DEBUG_H
#define MUHORS_DEBUG_H

#define DEBUG_INF 0
#define DEBUG_ERR 1
#define DEBUG_WARNING 2

/// Debug function
/// \param message Message to be printed as the debug
/// \param debug_level Level of the debug
void debug(unsigned char message[], int debug_level);


#endif