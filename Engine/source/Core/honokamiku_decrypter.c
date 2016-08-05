/*!
 * \file honokamiku_decrypter.c
 * Routines implementation of HonokaMiku in ANSI C
 */

#include <stdlib.h>
#include <string.h>

#include "honokamiku_decrypter.h"
#include "md5.h"

/*!
 * Version 3 key update macro.
 */
#define honokamiku_update_v3(dctx) (dctx)->xor_key = ((dctx)->update_key = (dctx)->update_key * 214013 + 2531011) >> 24;
/*!
 * Version 2 key update macro.
 */
#define honokamiku_update_v2(dctx) \
	{ \
		unsigned int a, b, c, d; \
		a = (dctx)->update_key >> 16; \
		b = ((a * 1101463552) & 2147483647) + ((dctx)->update_key & 65535) * 16807; \
		c = (a * 16807) >> 15; \
		d = c + b - 2147483647; \
		b = b > 2147483646 ? d : b + c; \
		(dctx)->update_key = b; \
		(dctx)->xor_key = ((b >> 23) & 255) |((b >> 7) & 65280); \
	}

/*!
 * This array of values is used for Version 3 decryption routines.
 */
static const unsigned int version_3_key_tables[64]={
	1210253353u	,1736710334u,1030507233u,1924017366u,
	1603299666u	,1844516425u,1102797553u,32188137u	,
	782633907u	,356258523u	,957120135u	,10030910u	,
	811467044u	,1226589197u,1303858438u,1423840583u,
	756169139u	,1304954701u,1723556931u,648430219u	,
	1560506399u	,1987934810u,305677577u	,505363237u	,
	450129501u	,1811702731u,2146795414u,842747461u	,
	638394899u	,51014537u	,198914076u	,120739502u	,
	1973027104u	,586031952u	,1484278592u,1560111926u,
	441007634u	,1006001970u,2038250142u,232546121u	,
	827280557u	,1307729428u,775964996u	,483398502u	,
	1724135019u	,2125939248u,742088754u	,1411519905u,
	136462070u	,1084053905u,2039157473u,1943671327u,
	650795184u	,151139993u	,1467120569u,1883837341u,
	1249929516u	,382015614u	,1020618905u,1082135529u,
	870997426u	,1221338057u,1623152467u,1020681319u
};

/*!
 * Initialize decrypter context. Used internally
 */
int honokamiku_dinit(
	honokamiku_context		*decrypter_context,
	honokamiku_decrypt_mode	decrypt_mode,
	const char				*prefix,
	const char				*filename,
	const void				*file_header
)
{
	MD5_CTX mctx;	/* The MD5 context */
	const char* header; /* Actually points to file_header. It just used so we can read byte-per-byte from it. */
	const char* original_filename;	/* The original filename to prevent infinite loop when searching for basename */
	size_t filename_size; /* Will contain the length of filename. */

	/* Zero memory */
	memset(decrypter_context, 0, sizeof(honokamiku_context));
	
	header = (const char*)file_header;
	filename_size = strlen(filename);
	decrypter_context->dm = decrypt_mode;
	original_filename = filename;
	
	/* Get basename */
	filename += filename_size;
	for (;*filename != '/' && *filename != '\\' && filename >= original_filename; filename--) {}
	filename++;
	filename_size = strlen(filename);
	
	MD5Init(&mctx);
	MD5Update(&mctx, (unsigned char*)prefix, strlen(prefix));
	MD5Update(&mctx, (unsigned char*)filename, filename_size);
	MD5Final(&mctx);
	
	if (decrypt_mode == honokamiku_decrypt_none) return HONOKAMIKU_ERR_OK; /* Do nothing */
	if (decrypt_mode == honokamiku_decrypt_version1) return HONOKAMIKU_ERR_INVALIDMETHOD; /* TODO */
	if (decrypt_mode == honokamiku_decrypt_version2)
	{
		/* Check if we can decrypt this */
		if (memcmp(mctx.digest+4, file_header, 4) == 0)
		{
			/* Initialize decrypter context */
			decrypter_context->init_key = ( (mctx.digest[0] & 127) << 24) |
										  (mctx.digest[1] << 16) |
										  (mctx.digest[2] << 8) |
										  mctx.digest[3];
			decrypter_context->xor_key = ((decrypter_context->init_key >> 23) & 255) |
										 ((decrypter_context->init_key >> 7) & 65280);
			decrypter_context->update_key = decrypter_context->init_key;
			decrypter_context->pos = 0;
			
			return HONOKAMIKU_ERR_OK;
		}
		else
			/* Cannot decrypt */
			return HONOKAMIKU_ERR_INVALIDMETHOD;
	}
	if (decrypt_mode == honokamiku_decrypt_version3)
	{
		unsigned int i;
		char actual_file_header[3];
		unsigned short expected_sum; /* Used to calculate the sum of the filename and to be compared with the header */
		
		expected_sum = ( (unsigned short)header[10] << 8 ) | (unsigned char)header[11];
		
		/* Flip file header bytes */
		actual_file_header[0] = ~mctx.digest[4];
		actual_file_header[1] = ~mctx.digest[5];
		actual_file_header[2] = ~mctx.digest[6];
		
		/* Subtract filename char code. expected_sum should be 500 after it's done. */
		for (i = 0; i < filename_size; i++)
			expected_sum -= (unsigned char)filename[i];
		
		if (memcmp(actual_file_header, file_header, 3) == 0 && expected_sum == 500)
		{
			decrypter_context->xor_key = (decrypter_context->update_key = decrypter_context->init_key = version_3_key_tables[header[11] & 63]) >> 24;
			decrypter_context->pos = 0;
			
			return HONOKAMIKU_ERR_OK;
		}
		else
			return HONOKAMIKU_ERR_INVALIDMETHOD;
	}
	
	return HONOKAMIKU_ERR_DECRYPT_NOMETHOD;
}

/*!
 * Initialize decrypter context for encryption. Used internally
 */
int honokamiku_einit(
	honokamiku_context		*decrypter_context,
	honokamiku_decrypt_mode	decrypt_mode,
	const char				*prefix,
	const char				*filename,
	void					*header_out,
	size_t					header_size
)
{
	MD5_CTX mctx;
	char* header;
	const char* original_filename;
	size_t filename_size;
	
	/* Zero memory decrypter context */
	memset(decrypter_context, 0, sizeof(honokamiku_context));
	
	header = (char*)header_out;
	filename_size = strlen(filename);
	decrypter_context->dm = decrypt_mode;
	original_filename = filename;
	
	/* Get basename */
	filename+=filename_size;
	for (; *filename != '/' && *filename != '\\' && filename >= original_filename; filename--) {}
	filename++;
	filename_size = strlen(filename);
	
	MD5Init(&mctx);
	MD5Update(&mctx, (unsigned char*)prefix, strlen(prefix));
	MD5Update(&mctx, (unsigned char*)filename, filename_size);
	MD5Final(&mctx);
	
	if (decrypt_mode == honokamiku_decrypt_none) return HONOKAMIKU_ERR_OK; /* Do nothing */
	if (decrypt_mode == honokamiku_decrypt_version1) return HONOKAMIKU_ERR_INVALIDMETHOD; /* TODO */
	if (decrypt_mode == honokamiku_decrypt_version2)
	{
		if (header_size < 4) return HONOKAMIKU_ERR_BUFFERTOOSMALL;
		
		/* Initialize decrypter context */
		decrypter_context->init_key = ( (mctx.digest[0] & 127) << 24) |
									  (mctx.digest[1] << 16) |
									  (mctx.digest[2] << 8) |
									  mctx.digest[3];
		decrypter_context->xor_key = ((decrypter_context->init_key >> 23) & 255) |
									 ((decrypter_context->init_key >> 7) & 65280);
		decrypter_context->update_key = decrypter_context->init_key;
		decrypter_context->pos = 0;
		
		/* Copy header */
		memcpy(header_out, mctx.digest+4, 4);
		
		return HONOKAMIKU_ERR_OK;
	}
	if (decrypt_mode == honokamiku_decrypt_version3)
	{
		unsigned short filename_sum = 500; /* The sum of the filename binary char */
		
		if (header_size < 16) return HONOKAMIKU_ERR_BUFFERTOOSMALL;
		
		/* Calculate filename binary char sum */
		for(; *filename != 0; filename++)
			filename_sum += (unsigned char)*filename;
		
		/* Initialize decrypter context */
		decrypter_context->xor_key = (decrypter_context->update_key = decrypter_context->init_key = version_3_key_tables[filename_sum & 63]) >> 24;
		decrypter_context->pos = 0;
		
		/* Write header */
		memset(header, 0, 16);
		header[0] = ~mctx.digest[4];
		header[1] = ~mctx.digest[5];
		header[2] = ~mctx.digest[6];
		header[3] = 12;
		header[10] = filename_sum >> 8;
		header[11] = filename_sum & 255;
		
		return HONOKAMIKU_ERR_OK;
	}
	
	return HONOKAMIKU_ERR_DECRYPT_NOMETHOD;
}

void honokamiku_decrypt_block(
	honokamiku_context			*decrypter_context,
	void						*buffer,
	size_t						buffer_size
)
{
	honokamiku_decrypt_mode decrypt_mode;
	char* file_buffer;
	
	decrypt_mode = decrypter_context->dm;
	file_buffer = (char*)buffer;
	
	if (buffer_size == 0 || decrypt_mode == honokamiku_decrypt_none) return; /* Do nothing */
	if (decrypt_mode == honokamiku_decrypt_version1) {} /* TODO */
	if (decrypt_mode == honokamiku_decrypt_version2)
	{
		size_t decrypt_size;
		
		/* Check if the last decrypt position is odd */
		if (decrypter_context->pos % 2 == 1)
		{
			/* Then we'll decrypt it and update the key */
			file_buffer[0] ^= decrypter_context->xor_key>>8;
			file_buffer++;
			decrypter_context->pos++;
			buffer_size--;
			
			honokamiku_update_v2(decrypter_context);
		}
		
		/* Because we'll decrypt 2 bytes in every loop, divide by 2 */
		decrypt_size = buffer_size / 2;
		
		for (; decrypt_size!=0; decrypt_size--, file_buffer+=2)
		{
			file_buffer[0] ^= decrypter_context->xor_key;
			file_buffer[1] ^= decrypter_context->xor_key>>8;
			
			honokamiku_update_v2(decrypter_context);
		}
		
		/* If it's not equal, there should be 1 character need to decrypted. 
		   In this case, we decrypt the last byte but don't update the key */
		if ((buffer_size / 2) * 2 != buffer_size)
			file_buffer[0] ^= decrypter_context->xor_key;
	}
	if (decrypt_mode == honokamiku_decrypt_version3)
	{
		size_t decrypt_size = buffer_size;
		
		for(; decrypt_size!=0; file_buffer++, decrypt_size--)
		{
			*file_buffer ^= decrypter_context->xor_key;
			
			honokamiku_update_v3(decrypter_context);
		}
	}
	
	decrypter_context->pos += buffer_size;
}

void honokamiku_jump_offset(
	honokamiku_context			*decrypter_context,
	unsigned int				offset
)
{
	int reset_dctx;
	unsigned int loop_times;
	honokamiku_decrypt_mode decrypt_mode;
	
	reset_dctx = 0;
	decrypt_mode = decrypter_context->dm;
	
	/* Check if we're seeking forward */
	if (offset > decrypter_context->pos)
		loop_times = offset - decrypter_context->pos;
	else if (offset == decrypter_context->pos) return; /* Do nothing if the offset = pos*/
	else
	{
		/* Seeking backward */
		loop_times = offset;
		reset_dctx = 1;
	}
	if (decrypt_mode == honokamiku_decrypt_none) return; /* Do nothing (again) */
	if (decrypt_mode == honokamiku_decrypt_version1) {} /* TODO */
	if (decrypt_mode == honokamiku_decrypt_version2)
	{
		if (reset_dctx)
		{
			decrypter_context->update_key = decrypter_context->init_key;
			decrypter_context->xor_key = ((decrypter_context->init_key >> 23) & 255) |
										 ((decrypter_context->init_key >> 7) & 65280);
		}
		
		if (decrypter_context->pos % 2 == 1 && reset_dctx == 0)
		{
			loop_times--;
			honokamiku_update_v2(decrypter_context);
		}
		
		loop_times /= 2;
		
		for(; loop_times != 0; loop_times--)
			honokamiku_update_v2(decrypter_context);
	}
	if (decrypt_mode == honokamiku_decrypt_version3)
	{
		if (reset_dctx)
			decrypter_context->xor_key = (decrypter_context->update_key = decrypter_context->init_key) >> 24;
		
		for(; loop_times != 0; loop_times--)
			honokamiku_update_v3(decrypter_context);
	}
	
	decrypter_context->pos = offset;
}

#define honokamiku_verify_arguments \
	if ((gamefile_id != honokamiku_gamefile_unknown && gamefile_prefix != NULL) || \
		(gamefile_id == honokamiku_gamefile_unknown && gamefile_prefix == NULL)) \
		return HONOKAMIKU_ERR_INVALIDMETHOD; /* Only one of them can be zero/NULL but both can't be not zero/NULL and zero/NULL */ \
	if (gamefile_prefix == NULL) \
	{ \
		switch (gamefile_id) \
		{ \
			case honokamiku_gamefile_en: \
				gamefile_prefix = HONOKAMIKU_KEY_SIF_EN; \
				break; \
			case honokamiku_gamefile_jp: \
				gamefile_prefix = HONOKAMIKU_KEY_SIF_JP; \
				break; \
			case honokamiku_gamefile_tw: \
				gamefile_prefix = HONOKAMIKU_KEY_SIF_TW; \
				break; \
			case honokamiku_gamefile_kr: \
				gamefile_prefix = HONOKAMIKU_KEY_SIF_KR; \
				break; \
			case honokamiku_gamefile_cn: \
				gamefile_prefix = HONOKAMIKU_KEY_SIF_CN; \
				break; \
			default: \
				return HONOKAMIKU_ERR_INVALIDMETHOD; \
		} \
		if (decrypt_mode == honokamiku_decrypt_auto) \
			switch (gamefile_id) \
			{ \
				case honokamiku_gamefile_jp: \
					decrypt_mode = honokamiku_decrypt_version3; \
					break; \
				default: \
					decrypt_mode = honokamiku_decrypt_version2; \
					break; \
			} \
	}

int honokamiku_decrypt_init(
	honokamiku_context		*decrypter_context,
	honokamiku_decrypt_mode	decrypt_mode,
	honokamiku_gamefile_id	gamefile_id,
	const char				*gamefile_prefix,
	const char				*filename,
	const void				*file_header
)
{
	honokamiku_verify_arguments;
	
	return honokamiku_dinit(decrypter_context, decrypt_mode, gamefile_prefix, filename, file_header);
}

int honokamiku_encrypt_init(
	honokamiku_context		*decrypter_context,
	honokamiku_decrypt_mode	decrypt_mode,
	honokamiku_gamefile_id	gamefile_id,
	const char				*gamefile_prefix,
	const char				*filename,
	void					*header_out,
	size_t					header_size
)
{
	honokamiku_verify_arguments;
	
	return honokamiku_einit(decrypter_context, decrypt_mode, gamefile_prefix, filename, header_out, header_size);
}

honokamiku_gamefile_id honokamiku_decrypt_init_auto(
	honokamiku_context	*decrypter_context,
	const char			*filename,
	const void			*file_header
)
{
	honokamiku_gamefile_id gid;
	
	/* Loop through all known game IDs*/
	for (gid = honokamiku_gamefile_en; gid <= honokamiku_gamefile_cn; gid++)
	{
		if (honokamiku_decrypt_init(decrypter_context, honokamiku_decrypt_auto, gid, NULL, filename, file_header) == HONOKAMIKU_ERR_OK)
			return gid;
	}
	return honokamiku_gamefile_unknown;
}
