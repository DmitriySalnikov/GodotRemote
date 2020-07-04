/* GRBinResources.h */
#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES

#ifndef GRRESOURCES_H
#define GRRESOURCES_H

#define GetPoolVectorFromBin(to_var, res)         \
	PoolByteArray to_var;                         \
	to_var.resize(res##_size);                    \
	auto to_var##write = to_var.write();          \
	memcpy(to_var##write.ptr(), res, res##_size); \
	to_var##write.release()

namespace GRResources {

extern const char *Txt_CRT_Shader;

extern const unsigned int Bin_NoSignalPNG_size;
extern const unsigned char Bin_NoSignalPNG[];

} // namespace GRResources

#endif // !GRRESOURCES_H

#endif
