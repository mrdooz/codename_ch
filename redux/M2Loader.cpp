#include "stdafx.h"
#include "Scene.hpp"
#include "M2Loader.hpp"
#include "Mesh.hpp"

#define THROW_ON_FALSE(x) if (!(x)) { throw std::runtime_error("Error calling: " # x); }

namespace filesystem = boost::filesystem;



// note, the m2 file format changed with lich king, so most specs online are out of date.

// http://madx.dk/wowdev/wiki/index.php?title=M2/WotLK
// http://madx.dk/wowdev/wiki/index.php?title=M2/WotLK/.skin

struct FileReader
{
  FileReader();
  ~FileReader();

  bool open(const char* filename);

  template<typename T>
  bool read(T* t)
  {
    return read_raw(t, sizeof(T));
  }

  template<typename T>
  bool read_raw(T* t, const int32_t size)
  {
    if (idx_ + size > len_) {
      return false;
    }
    memcpy(t, &buf_[idx_], size);
    idx_ += size;
    return true;
  }

  uint32_t pos() const { return idx_; }
  void set_pos(const uint32_t idx) { idx_ = idx; }

  uint8_t* buf_;
  uint32_t  len_;
  uint32_t  idx_;
};

FileReader::FileReader()
: buf_(NULL)
, len_(0)
, idx_(0)
{
}

FileReader::~FileReader()
{
  SAFE_ADELETE(buf_);
}

bool FileReader::open(const char* filename)
{
  if (!load_file(buf_, len_, filename)) {
    return false;
  }
  idx_ = 0;

  return true;
}


#pragma pack(push, 1)
struct CountOffset
{
  uint32_t count;
  uint32_t offset;
};

struct MinMaxRadius
{
  D3DXVECTOR3 min;
  D3DXVECTOR3 max;
  float radius;
};

struct SkinHeader
{
  char id[4];
  CountOffset indices;
  CountOffset triangles;
  CountOffset vertex_props;
  CountOffset submeshes;
  CountOffset texture_units;
  uint32_t  bones;
};

struct Submesh
{
  int32_t part_id;
  int16_t vtx_ofs;
  int16_t vtx_cnt;
  int16_t tri_ofs;
  int16_t tri_cnt;
  int16_t bone_cnt;
  int16_t bone_idx;
  int16_t unknown;
  int16_t root_bone_idx;
  D3DXVECTOR3 bb;
  float unknown2[4];
};

struct TextureUnit
{
  int16_t flags;
  int16_t shading;
  int16_t submesh_idx;
  int16_t submesh_idx2;
  int16_t color_idx;
  int16_t tex_unit_idx;
  int16_t mode;
  int16_t texture_idx;
  int16_t tex_unit_idx2;
  int16_t transparency_idx;
  int16_t uv_anim_idx;
};

struct M2Header
{
  char  id[4];            // 0
  uint8_t version[4];     // 4
  CountOffset name;       // 8
  uint32_t  model_type;   // 16
  CountOffset global_sequence;  // 20
  CountOffset animation;  // 28
  CountOffset c;  // 36
  CountOffset d;  // 44
  CountOffset bones;  // 52
  //CountOffset f;  // 60
  CountOffset vertices; // 68
  uint32_t  num_views;
  //CountOffset views;  // 76
  CountOffset colors;
  CountOffset textures;
  CountOffset transparency;
  CountOffset i;
  CountOffset tex_anims;
  CountOffset k;
  CountOffset render_flags;
  CountOffset bone_lookup;
  CountOffset texture_lookup;
  CountOffset texture_unit_def;
  CountOffset transparency_lookup;
  //CountOffset texture_anim_lookup;
  MinMaxRadius dummy0;
  MinMaxRadius dummy1;
  CountOffset bounding_triangles;
  CountOffset bounding_vertices;
  CountOffset bounding_normals;
  CountOffset o;
  CountOffset p;
  CountOffset q;
  CountOffset lights;
  CountOffset cameras;
  CountOffset camera_lookup;
  CountOffset ribbon_emitters;
  CountOffset particle_emitters;
};

struct Vertex
{
  D3DXVECTOR3 pos;
  uint8_t bone_weights[4];
  uint8_t bone_index[4];
  D3DXVECTOR3 normal;
  D3DXVECTOR2 uv;
  D3DXVECTOR2 pad;
};

struct Triangle
{
  uint16_t  indices[3];

};

struct Texture
{
  uint32_t  type;
  uint32_t  flags;
  int32_t   filename_len;
  int32_t   filename_ofs;
};


template<typename T>
bool read_generic_vector(std::vector<T>& v, FileReader& f, const int32_t lump_size)
{
  const uint32_t count = lump_size / sizeof(T);
  v.resize(count);
  return f.read_raw((void*)&v[0], count * sizeof(T));
}

template<typename T>
bool read_generic_vector_count(std::vector<T>& v, FileReader& f, const int32_t count)
{
  return read_generic_vector(v, f, count * sizeof(T));
}


M2Loader::M2Loader()
  : scene_(NULL)
{
}

struct M2Vertex
{
  D3DXVECTOR3 pos;
  D3DXVECTOR3 normal;
  D3DXVECTOR2 uv;
};

D3DXVECTOR3 swap_yz(const D3DXVECTOR3& v)
{
  return D3DXVECTOR3(v.x, v.z, v.y);
}

struct BlpHeader
{
  char  id[4];
  uint32_t  version;
  uint8_t encoding;
  uint8_t alpha_depth;
  uint8_t alpha_encoding;
  uint8_t has_mip_maps;
  uint32_t  width;
  uint32_t  height;
  uint32_t  mip_offsets[16];
  uint32_t  mip_sizes[16];
  uint8_t   palette[256][4];
};

enum
{
  //! Use DXT1 compression.
  kDxt1 = ( 1 << 0 ), 

  //! Use DXT3 compression.
  kDxt3 = ( 1 << 1 ), 

  //! Use DXT5 compression.
  kDxt5 = ( 1 << 2 ), 

  //! Use a very slow but very high quality colour compressor.
  kColourIterativeClusterFit = ( 1 << 8 ),	

  //! Use a slow but high quality colour compressor (the default).
  kColourClusterFit = ( 1 << 3 ),	

  //! Use a fast but low quality colour compressor.
  kColourRangeFit	= ( 1 << 4 ),

  //! Use a perceptual metric for colour error (the default).
  kColourMetricPerceptual = ( 1 << 5 ),

  //! Use a uniform metric for colour error.
  kColourMetricUniform = ( 1 << 6 ),

  //! Weight the colour by alpha during cluster fit (disabled by default).
  kWeightColourByAlpha = ( 1 << 7 )
};

void DecompressAlphaDxt3( uint8_t* rgba, void const* block )
{
  uint8_t const* bytes = reinterpret_cast< uint8_t const* >( block );

  // unpack the alpha values pairwise
  for( int32_t i = 0; i < 8; ++i )
  {
    // quantise down to 4 bits
    uint8_t quant = bytes[i];

    // unpack the values
    uint8_t lo = quant & 0x0f;
    uint8_t hi = quant & 0xf0;

    // convert back up to bytes
    rgba[8*i + 3] = lo | ( lo << 4 );
    rgba[8*i + 7] = hi | ( hi >> 4 );
  }
}

void DecompressAlphaDxt5( uint8_t* rgba, void const* block )
{
  // get the two alpha values
  uint8_t const* bytes = reinterpret_cast< uint8_t const* >( block );
  int32_t alpha0 = bytes[0];
  int32_t alpha1 = bytes[1];

  // compare the values to build the codebook
  uint8_t codes[8];
  codes[0] = ( uint8_t )alpha0;
  codes[1] = ( uint8_t )alpha1;
  if( alpha0 <= alpha1 )
  {
    // use 5-alpha codebook
    for( int32_t i = 1; i < 5; ++i )
      codes[1 + i] = ( uint8_t )( ( ( 5 - i )*alpha0 + i*alpha1 )/5 );
    codes[6] = 0;
    codes[7] = 255;
  }
  else
  {
    // use 7-alpha codebook
    for( int32_t i = 1; i < 7; ++i )
      codes[1 + i] = ( uint8_t )( ( ( 7 - i )*alpha0 + i*alpha1 )/7 );
  }

  // decode the indices
  uint8_t indices[16];
  uint8_t const* src = bytes + 2;
  uint8_t* dest = indices;
  for( int32_t i = 0; i < 2; ++i )
  {
    // grab 3 bytes
    int32_t value = 0;
    for( int32_t j = 0; j < 3; ++j )
    {
      int32_t byte = *src++;
      value |= ( byte << 8*j );
    }

    // unpack 8 3-bit values from it
    for( int32_t j = 0; j < 8; ++j )
    {
      int32_t index = ( value >> 3*j ) & 0x7;
      *dest++ = ( uint8_t )index;
    }
  }

  // write out the indexed codebook values
  for( int32_t i = 0; i < 16; ++i )
    rgba[4*i + 3] = codes[indices[i]];
}

static int32_t Unpack565( uint8_t const* packed, uint8_t* colour )
{
  // build the packed value
  int32_t value = ( int32_t )packed[0] | ( ( int32_t )packed[1] << 8 );

  // get the components in the stored range
  uint8_t red = ( uint8_t )( ( value >> 11 ) & 0x1f );
  uint8_t green = ( uint8_t )( ( value >> 5 ) & 0x3f );
  uint8_t blue = ( uint8_t )( value & 0x1f );

  // scale up to 8 bits
  colour[0] = ( red << 3 ) | ( red >> 2 );
  colour[1] = ( green << 2 ) | ( green >> 4 );
  colour[2] = ( blue << 3 ) | ( blue >> 2 );
  colour[3] = 255;

  // return the value
  return value;
}

void DecompressColour( uint8_t* rgba, void const* block, bool isDxt1 )
{
  // get the block bytes
  uint8_t const* bytes = reinterpret_cast< uint8_t const* >( block );

  // unpack the endpoints
  uint8_t codes[16];
  int32_t a = Unpack565( bytes, codes );
  int32_t b = Unpack565( bytes + 2, codes + 4 );

  // generate the midpoints
  for( int32_t i = 0; i < 3; ++i )
  {
    int32_t c = codes[i];
    int32_t d = codes[4 + i];

    if( isDxt1 && a <= b )
    {
      codes[8 + i] = ( uint8_t )( ( c + d )/2 );
      codes[12 + i] = 0;
    }
    else
    {
      codes[8 + i] = ( uint8_t )( ( 2*c + d )/3 );
      codes[12 + i] = ( uint8_t )( ( c + 2*d )/3 );
    }
  }

  // fill in alpha for the intermediate values
  codes[8 + 3] = 255;
  codes[12 + 3] = ( isDxt1 && a <= b ) ? 0 : 255;

  // unpack the indices
  uint8_t indices[16];
  for( int32_t i = 0; i < 4; ++i )
  {
    uint8_t* ind = indices + 4*i;
    uint8_t packed = bytes[4 + i];

    ind[0] = packed & 0x3;
    ind[1] = ( packed >> 2 ) & 0x3;
    ind[2] = ( packed >> 4 ) & 0x3;
    ind[3] = ( packed >> 6 ) & 0x3;
  }

  // store out the colours
  for( int32_t i = 0; i < 16; ++i )
  {
    uint8_t offset = 4*indices[i];
    for( int32_t j = 0; j < 4; ++j )
      rgba[4*i + j] = codes[offset + j];
  }
}


static int32_t FixFlags( int32_t flags )
{
  // grab the flag bits
  int32_t method = flags & ( kDxt1 | kDxt3 | kDxt5 );
  int32_t fit = flags & ( kColourIterativeClusterFit | kColourClusterFit | kColourRangeFit );
  int32_t metric = flags & ( kColourMetricPerceptual | kColourMetricUniform );
  int32_t extra = flags & kWeightColourByAlpha;

  // set defaults
  if( method != kDxt3 && method != kDxt5 )
    method = kDxt1;
  if( fit != kColourRangeFit )
    fit = kColourClusterFit;
  if( metric != kColourMetricUniform )
    metric = kColourMetricPerceptual;

  // done
  return method | fit | metric | extra;
}


void Decompress( uint8_t* rgba, void const* block, int32_t flags )
{
  // get the block locations
  void const* colourBlock = block;
  void const* alphaBock = block;
  if( ( flags & ( kDxt3 | kDxt5 ) ) != 0 )
    colourBlock = reinterpret_cast< uint8_t const* >( block ) + 8;

  // decompress colour
  DecompressColour( rgba, colourBlock, ( flags & kDxt1 ) != 0 );

  // decompress alpha separately if necessary
  if( ( flags & kDxt3 ) != 0 )
    DecompressAlphaDxt3( rgba, alphaBock );
  else if( ( flags & kDxt5 ) != 0 )
    DecompressAlphaDxt5( rgba, alphaBock );
}

void DecompressImage( uint8_t* rgba, int32_t width, int32_t height, void const* blocks, int32_t flags )
{
  // fix any bad flags
  flags = FixFlags( flags );

  // initialise the block input
  uint8_t const* sourceBlock = reinterpret_cast< uint8_t const* >( blocks );
  int32_t bytesPerBlock = ( ( flags & kDxt1 ) != 0 ) ? 8 : 16;

  // loop over blocks
  for( int32_t y = 0; y < height; y += 4 )
  {
    for( int32_t x = 0; x < width; x += 4 )
    {
      // decompress the block
      uint8_t targetRgba[4*16];
      Decompress( targetRgba, sourceBlock, flags );

      // write the decompressed pixels to the correct image locations
      uint8_t const* sourcePixel = targetRgba;
      for( int32_t py = 0; py < 4; ++py )
      {
        for( int32_t px = 0; px < 4; ++px )
        {
          // get the target location
          int32_t sx = x + px;
          int32_t sy = y + py;
          if( sx < width && sy < height )
          {
            uint8_t* targetPixel = rgba + 4*( width*sy + sx );

            // copy the rgba value
            for( int32_t i = 0; i < 4; ++i )
              *targetPixel++ = *sourcePixel++;
          }
          else
          {
            // skip this pixel as its outside the image
            sourcePixel += 4;
          }
        }
      }

      // advance
      sourceBlock += bytesPerBlock;
    }
  }
}

#define BITMAP_SIGNATURE 'MB'

struct BitmapFileheader
{
  uint16_t Signature;
  uint32_t Size;
  uint32_t Reserved;
  uint32_t BitsOffset;
};

struct BitmapHeader
{
  uint32_t HeaderSize;
  int32_t Width;
  int32_t Height;
  uint16_t Planes;
  uint16_t BitCount;
  uint32_t Compression;
  uint32_t SizeImage;
  int32_t PelsPerMeterX;
  int32_t PelsPerMeterY;
  uint32_t ClrUsed;
  uint32_t ClrImportant;
};


struct Dbc
{
  uint8_t header[4];
  uint32_t  record_count;
  uint32_t  fields_per_record;
  uint32_t  record_size;
  uint32_t  string_block_size;
};

struct ItemDisplayInfoRecord
{
  uint32_t  item_id;
  uint32_t  filename1;
  uint32_t  filename2;
  uint32_t  texture1;
  uint32_t  texture2;
  uint32_t  inventory_icon;
  uint32_t  geoset_a;
  uint32_t  geoset_b;
  uint32_t  geoset_c;
  uint32_t  unknown1;
  uint32_t  unknown2;
  uint32_t  flags;
  uint32_t  unknown3;
  uint32_t  unknown4;
  uint32_t  upper_arm;
  uint32_t  lower_arm;
  uint32_t  hands;
  uint32_t  chest_upper;
  uint32_t  chest_lower;
  uint32_t  leg_upper;
  uint32_t  leg_lower;
  uint32_t  feet;
  uint32_t  visual_id;
  uint32_t  pad1;
  uint32_t  pad2;
};
/*
Column 	 Field 	 Type 	 Notes
1 	ID 	Integer 	
2 	Model 	iRefID 	A model to be used.
3 	Sound 	iRefID 	Not set for that much models. Can also be set in CreatureModelData.
4 	ExtraDisplayInformation 	iRefID 	If this display-id is a NPC wearing things that are described in there.
5 	Scale 	Float 	Default scale, if not set by server. 1 is the normal size.
6 	Opacity 	Integer 	0 (transparent) to 255 (opaque).
7 	Skin1 	String 	Skins that are used in the model.
8 	Skin2 	String 	See this for information when they are used.
9 	Skin3 	String 	
10 	Icon 	String 	Holding an icon like INV_Misc_Food_59. Only on a few.
11 	bloodLevel 	iRefID 	If 0, this is read from CreatureModelData. (CGUnit::RefreshDataPointers)
12 	blood 	iRefID 	
13 	NPCSounds 	iRefID 	Sounds used when interacting with the NPC.
14 	Particles 	iRefID 	Values are 0 and >281. Wherever they are used ..
15 	creatureGeosetData 	Integer 	With this one, you can select an geoset out of the first 8 groups. 0x00200000 will select geoset 2 out of group 600 and therefore 602.
16 	objectEffectPackageID 	iRefID 	Set for gyrocopters, catapults, rocketmounts and siegevehicles. (WotLK) 
*/

struct CreatureDisplayInfoRecord
{
  uint32_t id;
  uint32_t  model_id;
  uint32_t  sound_id;
  uint32_t  extra_info;
  float scale;
  uint32_t opacity;
  uint32_t  skin1;
  uint32_t  skin2;
  uint32_t  skin3;
  uint32_t  icon;
  uint32_t  blood_level;
  uint32_t    blood;
  uint32_t  npc_sounds;
  uint32_t  particles;
  uint32_t  create_geoset_data;
  uint32_t  object_effect_package_id;
};

/*
1 	 ID 	 Integer 	
2 	Flags 	Integer 	Known to be checked: 8, 0x40, 0x80. Known: 4: Has death corpse.
3 	ModelPath 	String 	*.MDX!
4	AlternateModel	String 	This is always 0. It would be used, if something was in here. Its pushed into M2Scene::AddNewModel(GetM2Cache(), Modelpath, AlternateModel, 0).
5 	Unknown 	Integer* 	4 got mostly big models (ragnaros, nef.) but again, not all big models got 4 ...
6 	Scale 	Float 	CMD.Scale * CDI.Scale is used in CUnit.
7 	BloodLevel 	iRefID 	
8 	Footprint 	iRefID 	Defines the footpritns you leave in snow.
9 	FootprintWidth	Float 	most time 18.0
10 	FootprintLength	Float 	mostly 12, others are 0.0 - 20.0
11	FootprintDepth(?)	Float 	mostly 1.0, others are 0.0 - 5.0
12 	Unknown 	Integer* 	always 0.
13 	GroundShake 	iRefID 	ground shake? (Kunga)
14 	Unknown 	Integer* 	0 most of the time.
15 	SoundData 	iRefID 	
16 	CollisionWidth 	Float 	Size of collision for the model. Has to be bigger than 0.41670012920929, else "collision width is too small.".
17 	CollisionHeight	Float 	ZEROSCALEUNIT when 0-CollisionHeight < 0
18	Unknown 	Float 	other collision data?
19 	MinVert	Vec3F 	These values are the actually maximum and minimum coordinates of the

vertices.
22 	MaxVert 	Vec3F 	
25 	Unknown 	Float 	mostly 1.0, others are 0.03 - 0.9
26 	Unknown 	Float 	mostly 1.0, others are 0.5 - 2.9 
*/

struct CreatureModelInfoRecord
{
  int32_t id;
  int32_t flags;
  int32_t model_path_ofs;
  int32_t alt_model;
  int32_t unused0;
  float scale;
  int32_t blood;
  int32_t footprint;
  float foot_print_width;
  float foot_print_length;
  float foot_print_depth;
  int32_t unknown0;
  int32_t ground_shake;
  int32_t unknown1;
  int32_t sound_data;
  float collision_width;
  float collision_height;
  float unknown2;
  float min_v[3];
  float max_v[3];
  float unknown3;
  float unknown4;
  int32_t unknown5;
  int32_t unknown6;
};

struct DirEntry
{
  DirEntry() : creature_model_info(NULL), creature_display_info(NULL), item_display_info(NULL) {}
  CreatureModelInfoRecord* creature_model_info;
  CreatureDisplayInfoRecord*  creature_display_info;
  ItemDisplayInfoRecord*  item_display_info;
};


struct ItemDatabase
{

  DirEntry* find_dir_entry(const int32_t id);
  void  load();

  typedef std::map<int32_t, DirEntry> Database;
  typedef Database::iterator DatabaseIt;


  std::vector<ItemDisplayInfoRecord> _item_display_info;
  std::vector<CreatureDisplayInfoRecord> _creature_display_info;
  std::vector<CreatureModelInfoRecord> _creature_model_info;

  Database _database;
};

DirEntry* ItemDatabase::find_dir_entry(const int32_t id)
{
  DatabaseIt it = _database.find(id);
  if (it != _database.end()) {
    return &it->second;
  }
  return NULL;

}

void ItemDatabase::load()
{

  FileReader f;
  THROW_ON_FALSE(f.open("ItemDisplayInfo.dbc"));

  Dbc dbc;
  f.read(&dbc);
  read_generic_vector_count(_item_display_info, f, dbc.record_count);
  for (int32_t i = 0, e = _item_display_info.size(); i < e; ++i) {
    _database[_item_display_info[i].item_id].item_display_info = &_item_display_info[i];
  }


  THROW_ON_FALSE(f.open("CreatureDisplayInfo.dbc"));
  f.read(&dbc);

  read_generic_vector_count(_creature_display_info, f, dbc.record_count);
  for (int32_t i = 0, e = _creature_display_info.size(); i < e; ++i) {
    _database[_creature_display_info[i].id].creature_display_info = &_creature_display_info[i];
  }

  THROW_ON_FALSE(f.open("CreatureModelData.dbc"));
  f.read(&dbc);

  read_generic_vector_count(_creature_model_info, f, dbc.record_count);
  for (int32_t i = 0, e = _creature_model_info.size(); i < e; ++i) {
    _database[_creature_model_info[i].id].creature_model_info = &_creature_model_info[i];
  }
}

ID3D10ShaderResourceView* load_blp(cstr filename)
{

  FileReader f;
  if (!f.open(filename)) {
    throw std::runtime_error(to_string("unable to load file: %s", filename));
  }

  BlpHeader header;
  f.read(&header);

  // create a bmp in memory
  const uint32_t header_size = sizeof(BitmapFileheader) + sizeof(BitmapHeader);
  const uint32_t total_size = header_size + header.width * header.height * 4;

  uint8_t* bmp = new uint8_t[total_size];
  BitmapFileheader* file_header = (BitmapFileheader*)&bmp[0];
  BitmapHeader* bitmap_header = (BitmapHeader*)&bmp[sizeof(BitmapFileheader)];
  uint8_t* decoded_data = bmp + header_size;

  DecompressImage(decoded_data, header.width, header.height, &f.buf_[header.mip_offsets[0]], kDxt1);

  ZeroMemory(file_header, sizeof(BitmapFileheader));
  ZeroMemory(bitmap_header, sizeof(BitmapHeader));
  file_header->Signature = BITMAP_SIGNATURE;
  file_header->Size = header.width * header.height * 4 + header_size;
  file_header->BitsOffset = header_size;

  bitmap_header->HeaderSize = sizeof(BitmapHeader);
  bitmap_header->Width = header.width;
  bitmap_header->Height = header.height;
  bitmap_header->Planes = 1;
  bitmap_header->BitCount = 32;
  bitmap_header->SizeImage = header.width * header.height * 4;

  ID3D10ShaderResourceView* texture = NULL;
  if (FAILED(D3DX10CreateShaderResourceViewFromMemory(g_d3d_device, bmp, total_size, NULL, NULL, &texture, NULL))) {
    return NULL;
  }

  SAFE_ADELETE(bmp);

  return texture;
};

const ItemDisplayInfoRecord* find_record(const uint32_t part_id, const std::vector<ItemDisplayInfoRecord>& records)
{
  for (size_t i = 0; i < records.size(); ++i) {
    if (records[i].item_id == part_id) {
      return &records[i];
    }
  }

  return NULL;
}


void M2Loader::load(const char* filename, Scene* scene)
{
  filesystem::path root(filesystem::path(filename).replace_extension());
  const std::string skin_filename(root.string() + "00.skin");

  FileReader f2;
  f2.open(skin_filename.c_str());

  SkinHeader skin_header;
  f2.read(&skin_header);

  scene_ = scene;

  FileReader f;
  f.open(filename);
  M2Header header;
  f.read(&header);

  uint32_t p = (uint32_t)(&header.vertices.count) - (uint32_t)(&header);

  // load global vertex list
  typedef std::vector<Vertex> Vertices;
  std::vector<Vertex> vertices;
  f.set_pos(header.vertices.offset);
  read_generic_vector_count(vertices, f, header.vertices.count);

  // load textures
  std::vector<Texture> textures;
  f.set_pos(header.textures.offset);
  read_generic_vector_count(textures, f, header.textures.count);

  int32_t *textures_remap = (int32_t*)(f.buf_ + header.texture_lookup.offset);

  const std::string texture_path("C:/projects/MpqExtract/dump/");

  for (int32_t i = 0, e = textures.size(); i < e; ++i) {
    if (textures[i].filename_len > 1) {
      const std::string texture_filename(texture_path + (const char*)&f.buf_[textures[i].filename_ofs]);
      LOG_INFO_LN("loading texture: %s", texture_filename.c_str());
      if (textures[i].type == 0) {
        ID3D10ShaderResourceView* texture = load_blp(texture_filename.c_str());
        scene->textures_.push_back(texture);
      }
    }
  }
  const char* texture_names = (const char*)&f.buf_[header.textures.offset + header.textures.count * sizeof(Texture)];

  ItemDatabase db;
  db.load();

  const DirEntry *entry = db.find_dir_entry(1);

  std::vector<int16_t> indices;
  f2.set_pos(skin_header.indices.offset);
  read_generic_vector_count(indices, f2, skin_header.indices.count);

  std::vector<Triangle> triangles;
  f2.set_pos(skin_header.triangles.offset);
  read_generic_vector_count(triangles, f2, skin_header.triangles.count / 3);

  std::vector<Submesh> sub_meshes;
  f2.set_pos(skin_header.submeshes.offset);
  read_generic_vector_count(sub_meshes, f2, skin_header.submeshes.count);

  std::vector<TextureUnit> texture_units;
  f2.set_pos(skin_header.texture_units.offset);
  read_generic_vector_count(texture_units, f2, skin_header.texture_units.count);

/*

  look up stuff like this:

    - read the triangles array from the skin (using triangles.count / 3)
    - this is the global index list, pointing into the verts

    - for a sub mesh, use triangle_ofs/3 and triangle_count/3
      - use triangle_ofs/3 to find the starting index in the triangle list
      - triangle_count/3 says how many triangles to use


*/


  Mesh* mesh = new Mesh("test");
  mesh->vertex_buffer_stride_ = sizeof(M2Vertex);
  mesh->index_buffer_format_ = DXGI_FORMAT_R16_UINT;

  // create a mesh from the first submesh
  const Submesh &submesh = sub_meshes[0];

  const int32_t vertex_count = submesh.tri_cnt;
  std::vector<M2Vertex> verts;
  verts.resize(submesh.vtx_cnt);
  std::vector<int16_t> idx;
  idx.resize(vertex_count);
  int max_idx = 0;
  for (int32_t i = 0, e = submesh.tri_cnt / 3; i < e; ++i) {
    const Triangle &cur_triangle = triangles[submesh.tri_ofs / 3 + i];
    for (int32_t j = 0; j < 3; ++j) {
      static int32_t swapper[] = { 0, 2, 1 };
      const int16_t cur_idx = indices[cur_triangle.indices[swapper[j]]];
      idx[i*3+j] = cur_idx;
    }
  }

  for (int32_t i = 0, e = submesh.vtx_cnt; i < e; ++i) {
    const Vertex &cur_vtx = vertices[submesh.vtx_ofs + i];
    verts[i].pos = swap_yz(cur_vtx.pos);
    verts[i].normal = swap_yz(cur_vtx.normal);
    verts[i].uv = cur_vtx.uv;
  }

  create_static_vertex_buffer(mesh->vertex_buffer_, g_d3d_device, (uint8_t*)&verts[0].pos.x, submesh.vtx_cnt, sizeof(M2Vertex));
  create_static_index_buffer(mesh->index_buffer_, g_d3d_device, (uint8_t*)&idx[0], vertex_count, 2);
  mesh->index_count_ = vertex_count;

  scene_->meshes_.push_back(MeshSPtr(mesh));

}

#pragma pack(pop)
