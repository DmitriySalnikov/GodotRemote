/* GRUtils.cpp */

#include "GRUtils.h"

namespace GRUtils {

int current_loglevel =
#ifdef DEBUG_ENABLED
		LogLevel::LL_Normal;
#else
		LogLevel::LL_Warning;
#endif
PoolByteArray internal_PACKET_HEADER = PoolByteArray();

} // namespace GRUtils
