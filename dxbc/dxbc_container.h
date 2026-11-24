#pragma once

#include <array>

#include "../util/util_byte_stream.h"
#include "../util/util_fourcc.h"
#include "../util/util_md5.h"
#include "../util/util_small_vector.h"

namespace dxbc_spv::dxbc {

class Builder;
class Signature;

/** Header of the contrainer file */
struct FileHeader {
  /* Always 'DXBC' */
  util::FourCC magic = util::FourCC("DXBC");
  /* MD5 digest */
  util::md5::Digest hash = { };
  /* Always 1 */
  uint32_t version = 1u;
  /* Total file size, in bytes */
  uint32_t fileSize = 0u;
  /* Number of chunks */
  uint32_t chunkCount = 0u;
};


/** Header of each chunk */
struct ChunkHeader {
  ChunkHeader() = default;

  explicit ChunkHeader(util::ByteReader& reader);

  /* Chunk identifier */
  util::FourCC tag = { };
  /* Chunk size, in bytes */
  uint32_t size = 0u;

  /** Writes chunk header to byte array */
  bool write(util::ByteWriter& writer) const;

  /** Checks whether chunk header is valid */
  explicit operator bool () const {
    return tag != util::FourCC();
  }
};


/** Chunk entry */
struct ChunkEntry {
  /* Chunk identifier */
  util::FourCC tag = { };
  /* Chunk offset and size, in bytes */
  uint32_t offset = 0u;
  uint32_t size = 0u;
};


/** Parser for the container format */
class Container {

public:

  explicit Container(const void* data, size_t size)
  : Container(util::ByteReader(data, size)) { }

  explicit Container(util::ByteReader reader);

  /** Looks up chunk by its four-character code and returns a
   *  byte reader covering the chunk's memory region, if any. */
  util::ByteReader findChunk(util::FourCC tag) const;

  /** Queries the number of chunks in the container. */
  uint32_t getChunkCount() const {
    return uint32_t(m_chunks.size());
  }

  /** Queries the four-character code of a chunk. Chunks
   *  are numbered in the order of the container. */
  util::FourCC getChunkTag(uint32_t index) const {
    return index < m_chunks.size() ? m_chunks[index].tag : util::FourCC();
  }

  /** Returns a byte reader covering the memory region of a chunk,
   *  in the same way as findChunk does. Together with getChunkTag,
   *  this allows to rebuild a container with some chunks changed
   *  and all others, known or not, passed through. */
  util::ByteReader getChunk(uint32_t index) const;

  /** Finds input signature chunk */
  util::ByteReader getInputSignatureChunk() const;

  /** Finds output signature chunk */
  util::ByteReader getOutputSignatureChunk() const;

  /** Finds patch constant signature chunk */
  util::ByteReader getPatchConstantSignatureChunk() const;

  /** Finds shader code chunk */
  util::ByteReader getCodeChunk() const;

  /** Finds interface chunk */
  util::ByteReader getInterfaceChunk() const;

  /** Checks whether the binary hash is valid */
  bool validateHash() const;

  /** Retrieves MD5 hash of the shader. */
  util::md5::Digest getHash() const {
    return m_header.hash;
  }

  static bool checkFourCC(util::ByteReader reader);

  /** Checks whether parser successfully parsed header and
   *  chunk metadata */
  explicit operator bool () const {
    return bool(m_reader);
  }

private:

  util::ByteReader  m_reader;
  FileHeader        m_header = { };

  util::small_vector<ChunkEntry, 16u> m_chunks;

  bool parseHeader();

  void resetOnError();

};


/** Chunk whose payload is emitted as it is. The payload
 *  does not include the chunk header, and is padded to a
 *  multiple of four bytes. It must not point into the
 *  writer the container is built into. */
struct RawChunk {
  util::FourCC tag = { };
  const void* data = nullptr;
  size_t size = 0u;
};


/** Info for building DXBC containers with all
 *  the chunk types currently implemented. */
struct ContainerInfo {
  /** Input signature */
  const Signature* inputSignature = nullptr;
  /** Patch constant signature for tessellation shaders */
  const Signature* patchSignature = nullptr;
  /** Output signature */
  const Signature* outputSignature = nullptr;
  /** Code chunk */
  const Builder* code = nullptr;
  /** Additional chunks, emitted after all of the above in the
   *  order given. Useful to pass through an already encoded code
   *  chunk, or chunks that this library does not know about. */
  const RawChunk* rawChunks = nullptr;
  size_t rawChunkCount = 0u;
};


/** Computes the hash that a container with the given contents
 *  must carry in its header. The hash field itself is ignored. */
util::md5::Digest hashDxbcBinary(const void* data, size_t size);


/** Function to build a DXBC container. This will insert a header with a
 *  valid hash, and chain the given non-null chunks into the binary. */
bool buildContainer(util::ByteWriter& writer, const ContainerInfo& info);

}
