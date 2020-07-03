/* GRUtils.cpp */

#include "GRUtils.h"

namespace GRUtils {
int compress_buffer_size_mb = 4;
int current_loglevel =
#ifdef DEBUG_ENABLED
		LogLevel::LL_Normal;
#else
		LogLevel::LL_Warning;
#endif
PoolByteArray internal_PACKET_HEADER = PoolByteArray();
PoolByteArray internal_VERSION = PoolByteArray();
PoolByteArray compress_buffer = PoolByteArray();

} // namespace GRUtils
