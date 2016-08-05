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
#include "encryptFile.h"
#include "CPFInterface.h"

/*
 * To decrypt SIF files regardless of the encryption type, libHonoka (HonokaMiku C89) is used
 */
CDecryptBaseClass::CDecryptBaseClass()
: m_decrypt	(false)
, m_useNew	(false)
, m_header_size(0)
{
	// User context.
	// m_userCtx.members = initValue
}

/*!
    @brief  複合化
    @param[in]  void* ptr       暗号化されたデータ
    @param[in]  u32 length      データの長さ
    @return     void
 */
void CDecryptBaseClass::decrypt(void* ptr, u32 length) {
	honokamiku_decrypt_block(&m_dctx, ptr, length);
}

// Uses part of libhonoka 
u32 CDecryptBaseClass::decryptSetup(const u8* ptr, const u8* hdr) {
	honokamiku_gamefile_id x = honokamiku_decrypt_init_auto(&m_dctx, (const char*)ptr, hdr);

	if(x == honokamiku_gamefile_unknown)
	{
		DEBUG_PRINT("%s: unsupported encryption scheme", ptr);

		m_useNew = false;
		m_decrypt = false;
		m_header_size = 0;

		return 0;
	}

	switch(x)
	{
		case honokamiku_gamefile_jp:
		{
			m_header_size = 16;
			break;
		}
		default:
		{
			m_header_size = 4;
			break;
		}
	}

	m_useNew = true;
	m_decrypt = true;

	return 1;
}

void CDecryptBaseClass::gotoOffset(u32 offset) {
	// Recompute and update your encryption context if we jump at a certain position into the encrypted stream.
	// gotoOffset is ALWAYS called BEFORE decrypt if a jump in the decoding stream occurs.
	honokamiku_jump_offset(&m_dctx, offset);
}
