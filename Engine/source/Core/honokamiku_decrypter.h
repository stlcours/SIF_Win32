/*!
 * \file honokamiku_decrypter.h
 * Header of HonokaMiku ANSI C implementation
 */

#ifndef __DEP_HONOKAMIKU_H
#define __DEP_HONOKAMIKU_H

#include <stdlib.h>

/****************
** Error codes **
****************/
#define HONOKAMIKU_ERR_OK					0	/*!< No error (success) */
#define HONOKAMIKU_ERR_DECRYPT_NOMETHOD		1	/*!< No method found to decrypt this file */
#define HONOKAMIKU_ERR_DECRYPTIONUNKNOWN	1	/*!< No method found to decrypt this file */
#define HONOKAMIKU_ERR_BUFFERTOOSMALL		2	/*!< Buffer to store the headers is too small */
#define HONOKAMIKU_ERR_INVALIDMETHOD		3	/*!< Invalid decryption method OR Invalid argument OR Method unimplemented*/

/**************************************
** Pre-defined prefix for game files **
***************************************/
#define HONOKAMIKU_KEY_SIF_EN	"BFd3EnkcKa"
#define HONOKAMIKU_KEY_SIF_WW	"BFd3EnkcKa"
#define HONOKAMIKU_KEY_SIF_JP	"Hello"
#define HONOKAMIKU_KEY_SIF_TW	"M2o2B7i3M6o6N88"
#define HONOKAMIKU_KEY_SIF_KR	"Hello"
#define HONOKAMIKU_KEY_SIF_CN	"iLbs0LpvJrXm3zjdhAr4"

/*!
 * Decryption modes.
 */
typedef enum
{
	honokamiku_decrypt_none,		/*!< Transparent encryption/decryption. */
	honokamiku_decrypt_version1,	/*!< Version 1 encryption/decryption. All games supports this decryption mode. TODO: Implementation/ */
	honokamiku_decrypt_version2,	/*!< Version 2 encryption/decryption. Used in EN, TW, KR, and CN game files. */
	honokamiku_decrypt_version3,	/*!< Version 3 encryption/decryption. Used in JP game files. */
	
	honokamiku_decrypt_jpfile = honokamiku_decrypt_version3,
	honokamiku_decrypt_auto = (-1)	/*!< Automatically determine decryption type from honokamiku_decrypt_init(). */
} honokamiku_decrypt_mode;

/*!
 * Game file IDs for HonokaMiku.
 */
typedef enum
{
	honokamiku_gamefile_unknown,	/*!< Unknown game file.*/
	honokamiku_gamefile_en,			/*!< SIF EN game file.*/
	honokamiku_gamefile_jp,			/*!< SIF JP game file.*/
	honokamiku_gamefile_tw,			/*!< SIF TW game file.*/
	honokamiku_gamefile_kr,			/*!< SIF KR game file.*/
	honokamiku_gamefile_cn,			/*!< SIF CN game file.*/
	
	honokamiku_gamefile_ww = honokamiku_gamefile_en	/*!< SIF EN game file.*/
} honokamiku_gamefile_id;

/*!
 * Decrypter context structure. All honokamiku_* function needs this structure.
 */
typedef struct
{
	honokamiku_decrypt_mode dm;	/*!< Used to determine correct routines */
	unsigned int	init_key;	/*!< Key used at pos 0. Used when the decrypter needs to jump to specific-position */
	unsigned int	update_key;	/*!< Current key at `pos` */
	unsigned int	xor_key;	/*!< Values to use when XOR-ing bytes */
	unsigned int	pos;		/*!< Variable to track current position. Needed to allow jump to specific-position */
} honokamiku_context;

/**************
** Functions **
**************/

/*!
 * \brief Initialize HonokaMiku decrypter context to decrypt a file.
 *        Returns #HONOKAMIKU_ERR_OK on success.
 * \param decrypter_context HonokaMiku decrypter context to be initialized
 * \param decrypt_mode HonokaMiku decryption mode.
 * \param gamefile_id Game file to decrypt. This can be ::honokamiku_gamefile_unknown if \a gamefile_prefix is not NULL
 * \param gamefile_prefix String to be prepended when initializing the \a decrypter_context.
 *                        This can be NULL if \a gamefile_id is not ::honokamiku_gamefile_unknown
 * \param filename File name that want to be decrypted.
 * \param file_header The first 4-bytes (16-bytes if \a decrypt_mode is ::honokamiku_decrypt_version3) contents of the file
 * \warning You can't set \a gamefile_id to values other than ::honokamiku_gamefile_unknown if \a gamefile_prefix is not NULL.  
 *          You also can't set \a gamefile_prefix to NULL if \a gamefile_id is ::honokamiku_gamefile_unknown.  
 *          In short: Only one of them can be zero/NULL.
 * \todo Version 1 decrypter initialization
 * \sa honokamiku_context
 * \sa honokamiku_decrypt_mode
 * \sa honokamiku_gamefile_id
 */ 
int honokamiku_decrypt_init(
	honokamiku_context		*decrypter_context,
	honokamiku_decrypt_mode	decrypt_mode,
	honokamiku_gamefile_id	gamefile_id,
	const char				*gamefile_prefix,
	const char				*filename,
	const void				*file_header
);

/*!
 * \brief Initialize HonokaMiku decrypter context with all possible known game ID.
 *        Returns ::honokamiku_gamefile_unknown if no suitable decryption method is found.
 * \param decrypter_context HonokaMiku decrypter context to be initialized
 * \param filename File name that want to be decrypted.
 * \param file_header The first 16-bytes contents of the file
 * \sa honokamiku_decrypt_init()
 * \sa honokamiku_context
 * \sa honokamiku_gamefile_id
 */
honokamiku_gamefile_id honokamiku_decrypt_init_auto(
	honokamiku_context	*decrypter_context,
	const char			*filename,
	const void			*file_header
);

/*!
 * \brief Initialize HonokaMiku decrypter context to encrypt a file.
 *        Returns #HONOKAMIKU_ERR_OK on success.
 * \param decrypter_context HonokaMiku decrypter context to be initialized
 * \param decrypt_mode HonokaMiku encryption mode
 * \param gamefile_id Game file to encrypt
 * \param gamefile_prefix String to be prepended when initializing the \a decrypter_context
 * \param filename File name that want to be encrypted
 * \param header_out Pointer to store the file headers
 * \param header_size Size of \a header_out
 * \todo Version 1 encryption initialization
 * \sa honokamiku_decrypt_init()
 * \sa honokamiku_context
 * \sa honokamiku_decrypt_mode
 */
int honokamiku_encrypt_init(
	honokamiku_context		*decrypter_context,
	honokamiku_decrypt_mode	decrypt_mode,
	honokamiku_gamefile_id	gamefile_id,
	const char				*gamefile_prefix,
	const char				*filename,
	void					*header_out,
	size_t					header_size
);

/*!
 * \brief XOR block of memory with specificed decrypt mode and decrypter context.
 * \param decrypter_context HonokaMiku decrypter context that already initialized with honokamiku_decrypt_init()
 * \param buffer Buffer to be decrypted
 * \param buffer_size Size of `buffer`
 * \todo Version 1 decryption routines
 * \sa honokamiku_decrypt_init()
 * \sa honokamiku_context
 * \sa honokamiku_decrypt_mode
 */
void honokamiku_decrypt_block(
	honokamiku_context			*decrypter_context,
	void						*buffer,
	size_t						buffer_size
);

/*!
 * \brief Recalculate decrypter context to decrypt at specific position.
 * \param decrypter_context HonokaMiku decrypter context which wants to set their position
 * \param offset Absolute position (starts at 0)
 * \todo Version 1 decryption jump
 * \sa honokamiku_context
 * \sa honokamiku_decrypt_init()
 * \sa honokamiku_decrypt_mode
 */
void honokamiku_jump_offset(
	honokamiku_context			*decrypter_context,
	unsigned int				offset
);

/******************
** Useful macros **
******************/
/*!
 * \brief Initialize decrypter context to decrypt SIF EN game file.
 * \param decrypter_context HonokaMiku decrypter context to be initialized
 * \param filename File name that want to be decrypted.
 * \param file_header The first 4-bytes contents of the file
 * \sa honokamiku_decrypt_init
 */
#define honokamiku_decrypt_init_sif_en(decrypter_context,filename,file_header) \
	honokamiku_decrypt_init( \
		decrypter_context, \
		honokamiku_decrypt_auto, \
		honokamiku_gamefile_unknown, \
		HONOKAMIKU_KEY_SIF_EN, \
		filename, \
		file_header)

/*!
 * \brief Initialize decrypter context to decrypt SIF JP game file.
 * \param decrypter_context HonokaMiku decrypter context to be initialized
 * \param filename File name that want to be decrypted.
 * \param file_header The first 16-bytes contents of the file
 * \sa honokamiku_decrypt_init
 */
#define honokamiku_decrypt_init_sif_jp(decrypter_context,filename,file_header) \
	honokamiku_decrypt_init( \
		decrypter_context, \
		honokamiku_decrypt_auto, \
		honokamiku_gamefile_unknown, \
		HONOKAMIKU_KEY_SIF_JP, \
		filename, \
		file_header)

/*!
 * \brief Initialize decrypter context to decrypt SIF TW game file.
 * \param decrypter_context HonokaMiku decrypter context to be initialized
 * \param filename File name that want to be decrypted.
 * \param file_header The first 4-bytes contents of the file
 * \sa honokamiku_decrypt_init
 */
#define honokamiku_decrypt_init_sif_tw(decrypter_context,filename,file_header) \
	honokamiku_decrypt_init( \
		decrypter_context, \
		honokamiku_decrypt_auto, \
		honokamiku_gamefile_unknown, \
		HONOKAMIKU_KEY_SIF_TW, \
		filename, \
		file_header)

/*!
 * \brief Initialize decrypter context to decrypt SIF KR game file.
 * \param decrypter_context HonokaMiku decrypter context to be initialized
 * \param filename File name that want to be decrypted.
 * \param file_header The first 4-bytes contents of the file
 * \sa honokamiku_decrypt_init
 */
#define honokamiku_decrypt_init_sif_kr(decrypter_context,filename,file_header) \
	honokamiku_decrypt_init( \
		decrypter_context, \
		honokamiku_decrypt_auto, \
		honokamiku_gamefile_unknown, \
		HONOKAMIKU_KEY_SIF_KR, \
		filename, \
		file_header)

/*!
 * \brief Initialize decrypter context to decrypt SIF CN game file.
 * \param decrypter_context HonokaMiku decrypter context to be initialized
 * \param filename File name that want to be decrypted.
 * \param file_header The first 4-bytes contents of the file
 * \sa honokamiku_decrypt_init
 */
#define honokamiku_decrypt_init_sif_cn(decrypter_context,filename,file_header) \
	honokamiku_decrypt_init( \
		decrypter_context, \
		honokamiku_decrypt_auto, \
		honokamiku_gamefile_unknown, \
		HONOKAMIKU_KEY_SIF_CN, \
		filename, \
		file_header)

#endif
