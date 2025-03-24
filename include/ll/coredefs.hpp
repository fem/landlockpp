#pragma once

#ifdef _LLPP_EXPORTS
# define LLPP_EXPORT __attribute__((visibility("default")))
#else
# define LLPP_EXPORT
#endif
