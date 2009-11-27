/*
	mpg123clr: MPEG Audio Decoder library Common Language Runtime version.

	copyright 2009 by Malcolm Boczek - free software under the terms of the LGPL 2.1
	mpg123clr.dll is a derivative work of libmpg123 - all original mpg123 licensing terms apply.

	All rights to this work freely assigned to the mpg123 project.
*/
/*
	libmpg123: MPEG Audio Decoder library

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

*/
/*
	1.8.1.0	04-Aug-09	Initial release.
*/

#pragma once

#pragma warning(disable : 4635)
#include "mpg123.h"
#pragma warning(default : 4635)


namespace mpg123clr
{
	///<summary>Mpg123 enumerations.</summary>
	namespace mpg
	{

		///<summary>Enumeration of the parameters types that it is possible to set/get.</summary>
		public enum class parms
		{
			verbose = MPG123_VERBOSE,				/// set verbosity value for enabling messages to stderr, >= 0 makes sense (integer)
			flags = MPG123_FLAGS,					/// set all flags, p.ex val = MPG123_GAPLESS|MPG123_MONO_MIX (integer)
			add_flags = MPG123_ADD_FLAGS,			/// add some flags (integer)
			force_rate = MPG123_FORCE_RATE,			/// when value > 0, force output rate to that value (integer)
			down_sample = MPG123_DOWN_SAMPLE,		/// 0=native rate, 1=half rate, 2=quarter rate (integer)
			rva = MPG123_RVA,						/// one of the RVA choices above (integer)
			downspeed = MPG123_DOWNSPEED,			/// play a frame N times (integer)
			upspeed = MPG123_UPSPEED,				/// play every Nth frame (integer)
			start_frame = MPG123_START_FRAME,		/// start with this frame (skip frames before that, integer)
			decode_frames = MPG123_DECODE_FRAMES,	/// decode only this number of frames (integer)
			icy_interval = MPG123_ICY_INTERVAL,		/// stream contains ICY metadata with this interval (integer)
			outscale = MPG123_OUTSCALE,				/// the scale for output samples (amplitude - integer or float according to mpg123 output format, normally integer)
			timeout = MPG123_TIMEOUT,				/// timeout for reading from a stream (not supported on win32, integer)
			remove_flags = MPG123_REMOVE_FLAGS,		/// remove some flags (inverse of MPG123_ADD_FLAGS, integer)
			resync_limit = MPG123_RESYNC_LIMIT,		/// Try resync on frame parsing for that many bytes or until end of stream (<0 ... integer).
			index_size = MPG123_INDEX_SIZE,			/// Set the frame index size (if supported). Values <0 mean that the index is allowed to grow dynamically in these steps (in positive direction, of course) -- Use this when you really want a full index with every individual frame.
			preframes = MPG123_PREFRAMES			/// Decode/ignore that many frames in advance for layer 3. This is needed to fill bit reservoir after seeking, for example (but also at least one frame in advance is needed to have all "normal" data for layer 3). Give a positive integer value, please.

		};


		///<summary>Parameter flag bits.</summary>
		///<remarks>Equivalent to MPG123_FLAGS, use the usual binary or ( | ) to combine.</remarks>
		public enum class param_flags
		{
			force_mono = MPG123_FORCE_MONO,			///     0111 Force some mono mode: This is a test bitmask for seeing if any mono forcing is active. 
			mono_left = MPG123_MONO_LEFT,			///     0001 Force playback of left channel only.  
			mono_right = MPG123_MONO_RIGHT,			///     0010 Force playback of right channel only. 
			mono_mix = MPG123_MONO_MIX,				///     0100 Force playback of mixed mono.         
			force_stereo = MPG123_FORCE_STEREO,		///     1000 Force stereo output.                  
			force_8bit = MPG123_FORCE_8BIT,			/// 00010000 Force 8bit formats.                   
			quiet = MPG123_QUIET,					/// 00100000 Suppress any printouts (overrules verbose).                    
			gapless = MPG123_GAPLESS,				/// 01000000 Enable gapless decoding (default on if libmpg123 has support). 
			no_resync = MPG123_NO_RESYNC,			/// 10000000 Disable resync stream after error.                             
			seekbuffer = MPG123_SEEKBUFFER,			/// 000100000000 Enable small buffer on non-seekable streams to allow some peek-ahead (for better MPEG sync). 
			fuzzy = MPG123_FUZZY,					/// 001000000000 Enable fuzzy seeks (guessing byte offsets or using approximate seek points from Xing TOC) 
			force_float = MPG123_FORCE_FLOAT,		/// 010000000000 Force floating point output (32 or 64 bits depends on mpg123 internal precision). 
		};

		///<summary>RVA enumeration.</summary>
		///<remarks>Equivalent to MPG123_RVA.</remarks>
		public enum class rva
		{
			rva_off   = MPG123_RVA_OFF,		/// RVA disabled (default).   
			rva_mix   = MPG123_RVA_MIX,		/// Use mix/track/radio gain. 
			rva_album = MPG123_RVA_ALBUM,	/// Use album/audiophile gain 
			rva_max   = MPG123_RVA_ALBUM,	/// The maximum RVA code, may increase in future. 
		};

		///<summary>An enum over all sample types possibly known to mpg123.</summary>
		///<remarks><para>The values are designed as bit flags to allow bitmasking for encoding families.</para>
		///
		///<para>Note that (your build of) libmpg123 does not necessarily support all these.
		/// Usually, you can expect the 8bit encodings and signed 16 bit.
		/// Also 32bit float will be usual beginning with mpg123-1.7.0 .</para>
		///
		///<para>What you should bear in mind is that (SSE, etc) optimized routines are just for
		/// signed 16bit (and 8bit derived from that). Other formats use plain C code.</para>
		///
		///<para>All formats are in native byte order. On a little endian machine this should mean
		/// that you can just feed the MPG123_ENC_SIGNED_32 data to common 24bit hardware that
		/// ignores the lowest byte (or you could choose to do rounding with these lower bits).</para>
		///</remarks>
		public enum class enc
		{
			enc_8			= MPG123_ENC_8,				/// 0000 0000 1111 Some 8 bit  integer encoding. 
			enc_16			= MPG123_ENC_16,			/// 0000 0100 0000 Some 16 bit integer encoding.
			enc_32			= MPG123_ENC_32,			/// 0001 0000 0000 Some 32 bit integer encoding.
			enc_signed		= MPG123_ENC_SIGNED,		/// 0000 1000 0000 Some signed integer encoding.
			enc_float		= MPG123_ENC_FLOAT,			/// 1110 0000 0000 Some float encoding.
			enc_signed_16   = MPG123_ENC_SIGNED_16,		///           1101 0000 signed 16 bit
			enc_unsigned_16 = MPG123_ENC_UNSIGNED_16,	///           0110 0000 unsigned 16 bit
			enc_unsigned_8  = MPG123_ENC_UNSIGNED_8,    ///           0000 0001 unsigned 8 bit
			enc_signed_8    = MPG123_ENC_SIGNED_8,      ///           1000 0010 signed 8 bit
			enc_ulaw_8		= MPG123_ENC_ULAW_8,		///           0000 0100 ulaw 8 bit
			enc_alaw_8		= MPG123_ENC_ALAW_8,		///           0000 1000 alaw 8 bit
			enc_signed_32   = MPG123_ENC_SIGNED_32,		/// 0001 0001 1000 0000 signed 32 bit
			enc_unsigned_32 = MPG123_ENC_UNSIGNED_32,   /// 0010 0001 0000 0000 unsigned 32 bit
			enc_float_32    = MPG123_ENC_FLOAT_32,      ///      0010 0000 0000 32bit float
			enc_float_64    = MPG123_ENC_FLOAT_64,      ///      0100 0000 0000 64bit float
			enc_any			= MPG123_ENC_ANY,			/// any encoding
		};


		///<summary>Channel count enumeration</summary>
		///<remarks>clr added <cref name="both">both</cref></remarks>
		public enum class channelcount
		{
			mono   = MPG123_MONO,
			stereo = MPG123_STEREO,
			both   = MPG123_MONO | MPG123_STEREO,
		};

		///<summary>Channel enumeration.</summary>
		public enum class channels
		{
			left	= MPG123_LEFT,	/// The Left Channel. 
			right	= MPG123_RIGHT,	/// The Right Channel. 
			both	= MPG123_LR,	/// Both left and right channel; same as MPG123_LEFT|MPG123_RIGHT 
		};

		///<summary>VBR enumeration.</summary>
		public enum class mpeg_vbr
		{
			cbr = MPG123_CBR,		/// Constant Bitrate Mode (default) 
			vbr = MPG123_VBR,		/// Variable Bitrate Mode 
			abr = MPG123_ABR,		/// Average Bitrate Mode 
		};

		///<summary>MPEG Version enumeration.</summary>
		public enum class mpeg_version
		{
			mpeg_1_0 = MPG123_1_0,	/// MPEG Version 1.0 
			mpeg_2_0 = MPG123_2_0,	/// MPEG Version 2.0 
			mpeg_2_5 = MPG123_2_5,	/// MPEG Version 2.5 
		};

		///<summary>MPEG Mode enumeration.</summary>
		public enum class mpeg_mode
		{
			m_stereo	= MPG123_M_STEREO,	/// Standard Stereo. 
			m_joint		= MPG123_M_JOINT,	/// Joint Stereo. 
			m_dual		= MPG123_M_DUAL,	/// Dual Channel. 
			m_mono		= MPG123_M_MONO,	/// Single Channel. 
		};

		///<summary>MPEG Flags enumeration.</summary>
		public enum class mpeg_flags
		{
			CRC			= MPG123_CRC,		/// The bitstream is error protected using 16-bit CRC. 
			COPYRIGHT	= MPG123_COPYRIGHT,	/// The bitstream is copyrighted. 
			PRIVATE		= MPG123_PRIVATE,	/// The private bit has been set. 
			ORIGINAL	= MPG123_ORIGINAL,	/// The bitstream is an original, not a copy. 
		};

		///<summary>Positional state.</summary>
		public enum class state
		{
			accurate	= MPG123_ACCURATE	/// Query if positons are currently accurate (integer value, 0 if false, 1 if true) 
		};


	}
}