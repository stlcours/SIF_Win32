// Workaround to use OGG audio in CWin32MP3

#include "CWin32MP3.h"
#include "CWin32Platform.h"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

struct Win32OGG_DecoderS
{
	FILE* fp;
	CDecryptBaseClass* dctx;
};

size_t Win32OGG_Read(void* dst, size_t count, size_t size, void* fh)
{
	Win32OGG_DecoderS* decoder = (Win32OGG_DecoderS*)fh;
	size_t readed = fread(dst, count, size, decoder->fp);

	decoder->dctx->decryptBlck(dst, readed);

	return readed;
}

int Win32OGG_Seek(void *fh, ogg_int64_t to, int type)
{
	Win32OGG_DecoderS* decoder = (Win32OGG_DecoderS*)fh;
	CDecryptBaseClass* dctx = decoder->dctx;
	int header_size = dctx->m_useNew ? 4 : 0;
	int pos = 0;
	int retval;
	
	if(type == SEEK_SET)
		retval = fseek(decoder->fp, to + header_size, SEEK_SET);
	else
		retval = fseek(decoder->fp, to, type);

	if(retval >= 0)
		dctx->gotoOffset(ftell(decoder->fp) - header_size);

	return retval;
}

long Win32OGG_Tell(void* fh)
{
	Win32OGG_DecoderS* decoder = (Win32OGG_DecoderS*)fh;

	return ftell(decoder->fp) - (decoder->dctx->m_useNew ? 4 : 0);
}

ov_callbacks Win32OGG_Callbacks = {&Win32OGG_Read, &Win32OGG_Seek, NULL, &Win32OGG_Tell};
//ov_callbacks Win32OGG_Callbacks = {&Win32OGG_Read, NULL, NULL, NULL};

bool CWin32MP3::loadOGG(const char* name)
{
	char* PCMOut;
	Win32OGG_DecoderS decoder;
	OggVorbis_File vf;
	FILE* fp;

	// Open
	PCMOut = NULL;
	fp = fopen(name, "rb");

	if(fp == NULL)
		return false;

	// Setup decrypter and variables
	if(CWin32Platform::g_useDecryption)
	{
		u8 hdr[4];
		fread(hdr, 1, 4, fp);

		if(m_decrypter.decryptSetup((const u8*)name, hdr) == 0)
			fseek(fp, 0, SEEK_SET);
	}

	decoder.fp = fp;
	decoder.dctx = &m_decrypter;

	if(ov_open_callbacks(&decoder, &vf, NULL, 0, Win32OGG_Callbacks) < 0)
	{
		fclose(fp);
		return false;
	}

	vorbis_info* vi = ov_info(&vf, -1);
	m_format.wFormatTag     = WAVE_FORMAT_PCM;
	m_format.nChannels      = vi->channels;
	m_format.nSamplesPerSec = vi->rate;
	m_format.wBitsPerSample = 16;
	m_format.nBlockAlign    = m_format.nChannels * (m_format.wBitsPerSample / 8);
	m_format.nAvgBytesPerSec = m_format.nSamplesPerSec * m_format.nBlockAlign;
	m_format.cbSize         = 0;
	m_begin = m_end = NULL;
	m_totalSize = 0;

	ogg_int64_t PCMSize = ov_pcm_total(&vf, -1) * sizeof(s16) * vi->channels;
	int current_section = 0;
	PCMOut = new char[PCMSize];
	char* PCMPos = PCMOut;
	char* pcm_eof = PCMOut + PCMSize;

	while(true)
	{
		int x = min(pcm_eof - PCMPos, 4096);
		long ret = ov_read(&vf, PCMPos, x, 0, 2, 1, &current_section);
		
		if(ret == 0 || x < 4096)
		{
			PCMPos += ret;
			break;
		}
		else if(ret < 0)
		{
			delete[] PCMOut;
			ov_clear(&vf);
			fclose(fp);

			DEBUG_PRINT("Invalid OGG. Cannot decode");
			return false;
		}
		else
			PCMPos += ret;
	}

	m_totalSize = PCMSize;
	m_is_ogg = true;
	m_ogg_buffer = PCMOut;

	ov_clear(&vf);
	fclose(fp);

	/*
	// Convert to MP3BLOCK
	for(char* i = PCMOut; i < pcm_eof; i += PCM_BUF_SIZE)
	{
		size_t copy_size = min(pcm_eof - i, PCM_BUF_SIZE);
		MP3BLOCK* block = new MP3BLOCK;

		m_totalSize += copy_size;
		block->size = copy_size / 2;
		block->prev = m_end;
		block->next = NULL;
		if(block->prev)
			block->prev->next = block;
		else
			m_begin = block;
		m_end = block;

		memcpy(block->buf, i, copy_size);
	}

	// Sanity check
	{
		size_t total_size = 0;
		MP3BLOCK* cur = m_begin;

		while(true)
		{
			total_size += cur->size;

			if(cur->next == NULL) break;
			else cur = cur->next;
		}

		klb_assert(total_size * 2 == m_totalSize, "m_totalSize = %d; total_size = %d", int(m_totalSize), int(total_size));
		//m_totalSize = total_size;
	}
	*/

	return true;
}