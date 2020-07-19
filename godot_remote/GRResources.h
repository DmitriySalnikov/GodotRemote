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

extern const unsigned int Bin_NoSignalVerticalPNG_size;
extern const unsigned char Bin_NoSignalVerticalPNG[];

// NOTIFICATION ICONS

extern const unsigned int Bin_CloseIconPNG_size;
extern const unsigned char Bin_CloseIconPNG[];

extern const unsigned int Bin_ConnectedIconPNG_size;
extern const unsigned char Bin_ConnectedIconPNG[];

extern const unsigned int Bin_DisconnectedIconPNG_size;
extern const unsigned char Bin_DisconnectedIconPNG[];

extern const unsigned int Bin_ErrorIconPNG_size;
extern const unsigned char Bin_ErrorIconPNG[];

extern const unsigned int Bin_WarningIconPNG_size;
extern const unsigned char Bin_WarningIconPNG[];

} // namespace GRResources

#endif // !GRRESOURCES_H

#endif
