/*==============================================================================
 Copyright (c) 2016-2018, The Linux Foundation.
 Copyright (c) 2018-2020, Laurence Lundblade.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above
 copyright notice, this list of conditions and the following
 disclaimer in the documentation and/or other materials provided
 with the distribution.
 * Neither the name of The Linux Foundation nor the names of its
 contributors, nor the name "Laurence Lundblade" may be used to
 endorse or promote products derived from this software without
 specific prior written permission.

 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 =============================================================================*/


#ifndef qcbor_private_h
#define qcbor_private_h


#include <stdint.h>
#include "UsefulBuf.h"


#ifdef __cplusplus
extern "C" {
#if 0
} // Keep editor indention formatting happy
#endif
#endif


/*
 The maxium nesting of arrays and maps when encoding or decoding.
 (Further down in the file there is a definition that refers to this
 that is public. This is done this way so there can be a nice
 separation of public and private parts in this file.
*/
#define QCBOR_MAX_ARRAY_NESTING1 15 // Do not increase this over 255


/* The largest offset to the start of an array or map. It is slightly
 less than UINT32_MAX so the error condition can be tests on 32-bit machines.
 UINT32_MAX comes from uStart in QCBORTrackNesting being a uin32_t.

 This will cause trouble on a machine where size_t is less than 32-bits.
 */
#define QCBOR_MAX_ARRAY_OFFSET  (UINT32_MAX - 100)

/*
 PRIVATE DATA STRUCTURE

 Holds the data for tracking array and map nesting during encoding. Pairs up
 with the Nesting_xxx functions to make an "object" to handle nesting encoding.

 uStart is a uint32_t instead of a size_t to keep the size of this
 struct down so it can be on the stack without any concern.  It would be about
 double if size_t was used instead.

 Size approximation (varies with CPU/compiler):
    64-bit machine: (15 + 1) * (4 + 2 + 1 + 1 pad) + 8 = 136 bytes
    32-bit machine: (15 + 1) * (4 + 2 + 1 + 1 pad) + 4 = 132 bytes
*/
typedef struct __QCBORTrackNesting {
   // PRIVATE DATA STRUCTURE
   struct {
      // See function QCBOREncode_OpenMapOrArray() for details on how this works
      uint32_t  uStart;   // uStart is the byte position where the array starts
      uint16_t  uCount;   // Number of items in the arrary or map; counts items
                          // in a map, not pairs of items
      uint8_t   uMajorType; // Indicates if item is a map or an array
   } pArrays[QCBOR_MAX_ARRAY_NESTING1+1], // stored state for the nesting levels
   *pCurrentNesting; // the current nesting level
} QCBORTrackNesting;


/*
 PRIVATE DATA STRUCTURE

 Context / data object for encoding some CBOR. Used by all encode functions to
 form a public "object" that does the job of encdoing.

 Size approximation (varies with CPU/compiler):
   64-bit machine: 27 + 1 (+ 4 padding) + 136 = 32 + 136 = 168 bytes
   32-bit machine: 15 + 1 + 132 = 148 bytes
*/
struct _QCBOREncodeContext {
   // PRIVATE DATA STRUCTURE
   UsefulOutBuf      OutBuf;  // Pointer to output buffer, its length and
                              // position in it
   uint8_t           uError;  // Error state, always from QCBORError enum
   QCBORTrackNesting nesting; // Keep track of array and map nesting
};


/*
 PRIVATE DATA STRUCTURE

 Holds the data for array and map nesting for decoding work. This structure
 and the DecodeNesting_xxx functions form an "object" that does the work
 for arrays and maps.

 Size approximation (varies with CPU/compiler):
   64-bit machine: 4 * 16 + 8 = 72
   32-bit machine: 4 * 16 + 4 = 68
 */
typedef struct __QCBORDecodeNesting  {
  // PRIVATE DATA STRUCTURE
   struct {
      uint16_t uCount;
      uint8_t  uMajorType;
   } pMapsAndArrays[QCBOR_MAX_ARRAY_NESTING1+1],
   *pCurrent;
} QCBORDecodeNesting;


typedef struct  {
   // PRIVATE DATA STRUCTURE
   void *pAllocateCxt;
   UsefulBuf (* pfAllocator)(void *pAllocateCxt, void *pOldMem, size_t uNewSize);
} QCORInternalAllocator;


/*
 PRIVATE DATA STRUCTURE

 The decode context. This data structure plus the public QCBORDecode_xxx
 functions form an "object" that does CBOR decoding.

 Size approximation (varies with CPU/compiler):
   64-bit machine: 32 + 1 + 1 + 6 bytes padding + 72 + 16 + 8 + 8 = 144 bytes
   32-bit machine: 16 + 1 + 1 + 2 bytes padding + 68 +  8 + 8 + 4 = 108 bytes
 */
struct _QCBORDecodeContext {
   // PRIVATE DATA STRUCTURE
   UsefulInputBuf InBuf;

   uint8_t        uDecodeMode;
   uint8_t        bStringAllocateAll;

   QCBORDecodeNesting nesting;

   // If a string allocator is configured for indefinite-length
   // strings, it is configured here.
   QCORInternalAllocator StringAllocator;

   // These are special for the internal MemPool allocator.
   // They are not used otherwise. We tried packing these
   // in the MemPool itself, but there are issues
   // with memory alignment.
   uint32_t uMemPoolSize;
   uint32_t uMemPoolFreeOffset;

   // This is NULL or points to QCBORTagList.
   // It is type void for the same reason as above.
   const void *pCallerConfiguredTagList;
};

// Used internally in the impementation here
// Must not conflict with any of the official CBOR types
#define CBOR_MAJOR_NONE_TYPE_RAW  9
#define CBOR_MAJOR_NONE_TAG_LABEL_REORDER 10
#define CBOR_MAJOR_NONE_TYPE_BSTR_LEN_ONLY 11
#define CBOR_MAJOR_NONE_TYPE_ARRAY_INDEFINITE_LEN 12
#define CBOR_MAJOR_NONE_TYPE_MAP_INDEFINITE_LEN 13

#ifdef __cplusplus
}
#endif

#endif /* qcbor_private_h */
