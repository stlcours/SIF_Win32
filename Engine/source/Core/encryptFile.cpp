/* 
   Copyright 2013 KLab Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
// ---------------------------------------------------------------
#include <string.h>
#include <openssl/md5.h>
#include "encryptFile.h"

/*!
    @brief  コンストラクタ
 */
CDecryptBaseClass::CDecryptBaseClass()
: m_decrypt	(false)
, m_useNew	(false)
{
	// User context.
	// m_userCtx.members = initValue
}

inline void updateV2(CDecryptBaseClass* dctx)
{
	unsigned int a, b, c, d;
	unsigned int& update_key = dctx->m_userCtx.update_key;
	unsigned int& xor_key = dctx->m_userCtx.xor_key;

	a = update_key >> 16;
	b = ((a * 1101463552) & 2147483647) + (update_key & 65535) * 16807;
	c = (a * 16807) >> 15;
	d = c + b - 2147483647;
	b = b > 2147483646 ? d : b + c;
	update_key = b;
	xor_key = ((b >> 23) & 255) |((b >> 7) & 65280);
}

/*!
    @brief  複合化
    @param[in]  void* ptr       暗号化されたデータ
    @param[in]  u32 length      データの長さ
    @return     void
 */
void CDecryptBaseClass::decrypt(void* ptr, u32 length) {
	// Version 2 encryption. Decrypt
	u8* file_buffer = reinterpret_cast<u8*>(ptr);
	size_t decrypt_size;
	
	/* Check if the last decrypt position is odd */
	if (m_userCtx.pos % 2 == 1)
	{
		/* Then we'll decrypt it and update the key */
		file_buffer[0] ^= u8(m_userCtx.xor_key>>8);
		file_buffer++;
		m_userCtx.pos++;
		length--;
		
		updateV2(this);
	}
	
	/* Because we'll decrypt 2 bytes in every loop, divide by 2 */
	decrypt_size = length / 2;
	
	for (; decrypt_size!=0; decrypt_size--, file_buffer+=2)
	{
		file_buffer[0] ^= u8(m_userCtx.xor_key);
		file_buffer[1] ^= u8(m_userCtx.xor_key>>8);
		
		updateV2(this);
	}
	
	/* If it's not equal, there should be 1 character need to decrypted. 
	   In this case, we decrypt the last byte but don't update the key */
	if ((length / 2) * 2 != length)
		file_buffer[0] ^= u8(m_userCtx.xor_key);

	m_userCtx.pos += length;
}

#define DECRYPT_KEY "BFd3EnkcKa"
#define DECRYPT_LEN 10

/*!
    @brief  複合化準備
    @param[in]  const u8* ptr   ファイルパス
    @return     void
 */
u32 CDecryptBaseClass::decryptSetup(const u8* ptr, const u8* hdr) {
	MD5_CTX* mctx;
	size_t basename_len;
	const u8* basename;
	u8 digest[MD5_DIGEST_LENGTH];

	basename = ptr + strlen(reinterpret_cast<const char*>(ptr));
	mctx = KLBNEW(MD5_CTX);

	for(; *basename != '/' && *basename != '\\' && basename >= ptr; basename--) {}
	basename_len = strlen(reinterpret_cast<const char*>(++basename));

	MD5_Init(mctx);
	MD5_Update(mctx, DECRYPT_KEY, DECRYPT_LEN);
	MD5_Update(mctx, basename, basename_len);
	MD5_Final(digest, mctx);

	if(memcmp(digest + 4, hdr, 4) == 0)
	{
		// OK. Version 2
		m_userCtx.init_key =
			m_userCtx.update_key =
				((digest[0] & 127) << 24) |
				(digest[1] << 16) |
				(digest[2] << 8) |
				digest[3];
		m_userCtx.xor_key = ((m_userCtx.init_key >> 23) & 255) | ((m_userCtx.init_key >> 7) & 65280);
		m_userCtx.pos = 0;
		m_useNew = true;
		m_decrypt = true;

		return 1;
	}
	else
	{
		// Version 1. Not implemented = transparent
		printf("%s: unsupported encryption scheme\n", ptr);

		m_useNew = false;
		m_decrypt = false;

		return 0;
	}
}

void CDecryptBaseClass::gotoOffset(u32 offset) {
	// Recompute and update your encryption context if we jump at a certain position into the encrypted stream.
	// gotoOffset is ALWAYS called BEFORE decrypt if a jump in the decoding stream occurs.
	unsigned int loop_times;

	// Is seeking to same position?
	if(offset == m_userCtx.pos)
		return;
	// Is seeking forward?
	else if(offset > m_userCtx.pos)
		loop_times = offset - m_userCtx.pos;
	// Seeking backward
	else
	{
		loop_times = offset;

		// Reset position
		m_userCtx.update_key = m_userCtx.init_key;
		m_userCtx.xor_key = ((m_userCtx.init_key >> 23) & 255) |
							((m_userCtx.init_key >> 7) & 65280);
	}

	loop_times /= 2;
	for(; loop_times > 0; loop_times--)
		updateV2(this);

	m_userCtx.pos = offset;
}
