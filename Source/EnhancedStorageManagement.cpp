// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Ultimate-Universe
//
// Enhanced Storage Management 1.0.1 for Workers & Resources: Soviet Republic 1.1.1.9.
//
// Compatible nearby storage and resource buildings form a logical resource pool.
// Freight stations/loaders are zero-capacity access ports into that pool, while
// WRSR's native transfer authorization, storage allocation policy and save data
// remain authoritative. Storage-to-storage, delivery-to-delivery and resource-to-
// resource propagation are intentionally not introduced.

using u8 = unsigned char;
using u32 = unsigned int;
using u64 = unsigned long long;
using usize = unsigned long long;
using i32 = int;
using i64 = long long;

static constexpr const char* PLUGIN_NAME = "Enhanced Storage Management";
static constexpr const char* PLUGIN_VERSION = "1.0.1";

extern "C" int _fltused = 0;

extern "C" __declspec(noinline) void* memset(void* dst, int value, usize count) {
    volatile u8* d = (volatile u8*)dst;
    for (usize i = 0; i < count; ++i) d[i] = (u8)value;
    return dst;
}

extern "C" __declspec(noinline) void* memcpy(void* dst, const void* src, usize count) {
    volatile u8* d = (volatile u8*)dst;
    const volatile u8* s = (const volatile u8*)src;
    for (usize i = 0; i < count; ++i) d[i] = s[i];
    return dst;
}

extern "C" __declspec(dllimport) int __stdcall DisableThreadLibraryCalls(void* module);
extern "C" __declspec(dllexport) unsigned TsmPluginApiVersion(void);
static void* volatile g_aslrAnchor = (void*)&TsmPluginApiVersion;

struct TsmPluginInfoCompat {
    const char* name;
    const char* version;
};

struct TsmHostCompat {
    unsigned apiVersion;
    unsigned structSize;
    void* exeModule;
    u8* exeBase;
    usize exeSize;
    void* engineModule;
    const char* baseDir;
    const char* pluginDir;
    void (*log)(const char* fmt, ...);
    void** (*findIatSlot)(void* module, const char* dll, const char* fn);
    int (*patchIat)(void* module, const char* dll, const char* fn, void* detour, void** original, const char* label);
    int (*installInlineHook)(void* target, void* detour, void** trampoline,
                             const u8* expect, usize stolen, const char* label);
    u8* (*allocNear)(u8* anchor, usize size);
    int (*readablePtr)(const void* p, usize n);
    long (*faultFilter)(const char* what, void* exceptionPointers);
    int (*configInt)(const char* iniName, const char* section, const char* key, int fallback);
    int (*configString)(const char* iniName, const char* section, const char* key,
                        char* out, int outSize, const char* fallback);
    int (*provide)(const char* service, unsigned version, const void* iface);
    const void* (*consume)(const char* service, unsigned version);
    const char* vfsRoot;
};

static_assert(sizeof(TsmHostCompat) == 0x98, "Tesmio API 4 host ABI drift");

// WRSR 1.2.0.8 RVAs. Each active hook target is checked against its expected entry bytes.
static constexpr usize RVA_BUILDING_SIMULATION_DISPATCH = 0x00152520ull;
static constexpr usize RVA_CONSTRUCTION_TOOL_FRAME      = 0x0030DBC0ull;
static constexpr usize RVA_GENERIC_STORAGE_UI           = 0x0073CA40ull;
static constexpr usize RVA_FACTORY_CONNECTION_QUERY     = 0x00446B40ull;
static constexpr usize RVA_RESOURCE_TRANSFER              = 0x00444210ull;
static constexpr usize RVA_VECTOR_PUSH_BACK_U64        = 0x00054960ull;
static constexpr usize RVA_BUILDING_HIGHLIGHT              = 0x00000000ull;
static constexpr usize RVA_GAME_ROOT_SLOT               = 0x009DDC88ull;
static constexpr usize RVA_C3D_COLLISION_2D_BBOX        = 0x0003E410ull;
static constexpr usize RVA_C3D_GET_ROTATION_STEP        = 0x0003A0E0ull;
static constexpr usize RVA_C3D_VEC3_TRANSFORM_COORD     = 0x000A13E0ull;
static constexpr usize ABSOLUTE_ENTRY_BRIDGE_SPAN       = 14ull;
static constexpr usize DISPATCH_HOOK_SPAN               = 19ull;
static constexpr usize CONSTRUCTION_HOOK_SPAN           = 14ull;
static constexpr usize GENERIC_UI_HOOK_SPAN             = 15ull;
static constexpr usize FACTORY_QUERY_HOOK_SPAN          = 14ull;
static constexpr usize RESOURCE_TRANSFER_HOOK_SPAN      = 17ull;
static const u8 VECTOR_PUSH_BACK_U64_EXPECTED[] = {
    0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20,0x48,0x8B,0x41,0x08
};
static const u8 DISPATCH_EXPECTED[DISPATCH_HOOK_SPAN] = {
    0x48,0x8B,0xC4,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8D,0xA8,0x78,0xFA,0xFF,0xFF
};
static const u8 CONSTRUCTION_EXPECTED[CONSTRUCTION_HOOK_SPAN] = {
    0x48,0x8B,0xC4,0x48,0x89,0x58,0x20,0x55,0x56,0x57,0x41,0x55,0x41,0x56
};
static const u8 GENERIC_UI_EXPECTED[GENERIC_UI_HOOK_SPAN] = {
    0x48,0x8B,0xC4,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57
};
static const u8 FACTORY_QUERY_EXPECTED[FACTORY_QUERY_HOOK_SPAN] = {
    0x44,0x88,0x4C,0x24,0x20,0x44,0x89,0x44,0x24,0x18,0x55,0x56,0x41,0x55
};
static const u8 RESOURCE_TRANSFER_EXPECTED[RESOURCE_TRANSFER_HOOK_SPAN] = {
    0x40,0x53,0x57,0x41,0x54,0x48,0x81,0xEC,0x80,0x00,0x00,0x00,0x41,0x83,0x79,0x44,0xFF
};
static const u8 BUILDING_HIGHLIGHT_EXPECTED[] = {
    0x40,0x55,0x53,0x56,0x57,0x48,0x8D,0xAC,0x24,0xF8,0xE5,0xFF,0xFF,0xB8,0x08,0x1B,0x00,0x00
};

// 1.2.0.8 layout fields corroborated in the supplied executable are recorded in docs/BINARY_ANALYSIS_1.2.0.8.md.
static constexpr usize GAME_BUILDINGS_BEGIN = 0x11E70ull;
static constexpr usize GAME_BUILDINGS_END   = 0x11E78ull;
static constexpr usize GAME_CURRENT_TOOL     = 0x0D4C0ull;
static constexpr usize GAME_TOOLS_BEGIN      = 0x0D280ull;
static constexpr usize GAME_TOOLS_END        = 0x0D288ull;
static constexpr usize TOOL_STRIDE           = 0x2D0ull;
static constexpr usize GAME_CURSOR_X         = 0x00F90ull;
static constexpr usize GAME_CURSOR_Y         = 0x00F94ull;
static constexpr usize GAME_CURSOR_Z         = 0x00F98ull;
static constexpr usize GAME_CURSOR_ROTATION  = 0x00F9Cull;
// FUN_140422C30 resets/fills this temporary std::vector<building*> every query.
// FUN_1407CDC40 uses it only to decide whether the cargo station should display
// language ID 0x712: "This building has no connection to warehouses or factories!".
static constexpr usize GAME_TEMP_CONNECTIONS_BEGIN = 0x14528ull;
static constexpr usize GAME_TEMP_CONNECTIONS_END   = 0x14530ull;
static constexpr usize GAME_TEMP_CONNECTIONS_CAP   = 0x14538ull;
static constexpr usize BUILDING_TEMPLATE     = 0x318ull;
// Position fields remain inherited from upstream and are not yet independently mapped.
static constexpr usize BUILDING_COMPLETION   = 0x604ull;
// 1.2.0.8 simulation dispatch reads building+0x5F4 as its C3D instance ID.
static constexpr usize BUILDING_INSTANCE_ID  = 0x5F4ull;
static constexpr usize BUILDING_STORES_BEGIN = 0x970ull;
static constexpr usize BUILDING_STORES_END   = 0x978ull;
static constexpr usize TOOL_TEMPLATE          = 0x48ull;
// Building-definition layout: the 1.2.0.8 runtime simulation dispatcher reads
// the building class at template+0x368:
// 5=$TYPE_STORAGE, 6=$TYPE_FACTORY, 7=$TYPE_MINE_*.  Do NOT use the
// building-editor type-table/index at +0x5E0/+0x610 for loaded native
// definitions. FUN_140177010 reads the live recipe vectors directly from the
// template: +0x688/+0x690 consumption and +0x6B8/+0x6C0 production. Each
// record is 0x10 bytes and begins with the resolved resource pointer; input
// rate is at +0x08, output rate at +0x0C. Storage compatibility is
// deliberately NOT read from a template-side descriptor table: live building
// instances expose the authoritative 0xE0 storage descriptors at +0x970.
static constexpr usize TEMPLATE_RUNTIME_TYPE     = 0x368ull;
static constexpr usize TEMPLATE_CONSUME_BEGIN    = 0x688ull;
static constexpr usize TEMPLATE_CONSUME_END      = 0x690ull;
static constexpr usize TEMPLATE_PRODUCE_BEGIN    = 0x6B8ull;
static constexpr usize TEMPLATE_PRODUCE_END      = 0x6C0ull;
// FUN_1400DEB70 and the building editor/export path iterate template+0x778/+0x780
// as the authoritative 0xE0 storage-blueprint vector. Placement
// and specialised cargo-gateway discovery can therefore use exact storage
// transports/roles before a live building store vector has materialised.
static constexpr usize TEMPLATE_STORES_BEGIN      = 0x778ull;
static constexpr usize TEMPLATE_STORES_END        = 0x780ull;
static constexpr usize RECIPE_STRIDE             = 0x10ull;
static constexpr usize RECIPE_RESOURCE           = 0x00ull;
static constexpr usize RECIPE_INPUT_RATE         = 0x08ull;
static constexpr usize RECIPE_OUTPUT_RATE        = 0x0Cull;
static constexpr i32 BUILDING_TYPE_CARGO_STATION = 0;
static constexpr i32 BUILDING_TYPE_STORAGE       = 5;
static constexpr i32 BUILDING_TYPE_FACTORY       = 6;
static constexpr i32 BUILDING_TYPE_MINE          = 7;
static constexpr i32 BUILDING_TYPE_FIELD         = 8;
static constexpr i32 BUILDING_TYPE_FARM          = 9;
static constexpr i32 BUILDING_TYPE_CONSTRUCTION_OFFICE = 0x0C;
static constexpr i32 BUILDING_TYPE_RAIL_CONSTRUCTION_OFFICE = 0x1C;
static constexpr i32 BUILDING_TYPE_GAS_STATION   = 0x22;
static constexpr i32 BUILDING_TYPE_PRODUCTION_LINE = 0x28;
static constexpr i32 BUILDING_TYPE_DISTRIBUTION_OFFICE = 0x2B;
static constexpr i32 BUILDING_TYPE_TECHNICAL_SERVICES = 0x31;
static constexpr i32 BUILDING_TYPE_RAIL_DISTRIBUTION_OFFICE = 0x34;
static constexpr i32 BUILDING_TYPE_HEATING_PLANT = 0x46;
static constexpr i32 BUILDING_TYPE_REPAIR_OFFICE = 0x6C;
static constexpr usize TEMPLATE_COLLISION_TYPE = 0x418ull;
static constexpr usize GAME_C3D_BUILDING_MANAGER = 0x1098ull;

// Cargo loaders/unloaders are direct proximity annex gateways. Their
// resource-flow direction is derived from each building type's own C3D
// connection-node metadata, not from traversing live network paths.
// C3DDLL64 1.2.0.8 C3D_BUILDING layout, verified against the exported helper entries.
static constexpr usize C3D_ROTATION_STEP_COUNT = 0x48ull;
static constexpr usize C3D_ROT_MATRIX_FWD_BASE = 0x54ull;
static constexpr usize C3D_ROT_MATRIX_INV_BASE = 0x94ull;
static constexpr usize C3D_ROT_MATRIX_STRIDE   = 0xA8ull;
static constexpr usize C3D_TYPES_BEGIN         = 0xE718ull;
static constexpr usize C3D_TYPES_END           = 0xE720ull;
static constexpr usize C3D_TYPE_STRIDE         = 0x2330ull;
static constexpr usize C3D_TYPE_PRIMARY_MESH   = 0x58ull;
static constexpr usize C3D_TYPE_SECONDARY_MESH = 0xA0ull;
static constexpr usize C3D_TYPE_FLIP_Z         = 0x45Full;
static constexpr usize C3D_TYPE_INSTANCES_BEGIN = 0x2C0ull;
static constexpr usize C3D_TYPE_INSTANCES_END   = 0x2C8ull;
static constexpr usize C3D_TYPE_NODES_BEGIN      = 0x00ull;
static constexpr usize C3D_TYPE_NODES_END        = 0x08ull;
// FUN_1400BFF40 exports a C3D building type by iterating its node-pointer
// vector at +0/+8. Each node points at connection metadata via +0xE0;
// metadata+0x20 is the native connection-kind code.
static constexpr usize C3D_NODE_CONNECTION_META  = 0xE0ull;
static constexpr usize C3D_CONNECTION_KIND       = 0x20ull;
static constexpr usize C3D_INSTANCE_STRIDE      = 0x118ull;
static constexpr usize C3D_INSTANCE_POS_X       = 0x10ull;
static constexpr usize C3D_INSTANCE_POS_Y       = 0x14ull;
static constexpr usize C3D_INSTANCE_POS_Z       = 0x18ull;
static constexpr usize C3D_INSTANCE_ROT_STEP    = 0x1Cull;
static constexpr usize C3D_MESH_BBOX_MIN_X     = 0x74ull;
static constexpr usize C3D_MESH_BBOX_MIN_Z     = 0x7Cull;
static constexpr usize C3D_MESH_BBOX_MAX_X     = 0x80ull;
static constexpr usize C3D_MESH_BBOX_MAX_Z     = 0x88ull;

// Live storage descriptor (0xE0 bytes).
static constexpr usize STORE_STRIDE            = 0xE0ull;
static constexpr usize STORE_RECORDS_BEGIN     = 0x00ull;
static constexpr usize STORE_RECORDS_END       = 0x08ull;
static constexpr usize STORE_ALLOC_BEGIN       = 0x18ull;
static constexpr usize STORE_ALLOC_END         = 0x20ull;
static constexpr usize STORE_BLOCK_OUTPUT      = 0x84ull;
static constexpr usize STORE_BLOCK_INPUT       = 0x85ull;
static constexpr usize STORE_CAPACITY          = 0x8Cull;
static constexpr usize STORE_TRANSPORT         = 0x90ull;

static constexpr usize RECORD_STRIDE           = 0x10ull;
static constexpr usize RECORD_RESOURCE         = 0x00ull;
static constexpr usize RECORD_CURRENT          = 0x08ull;

static constexpr usize ALLOC_STRIDE            = 0x10ull;
static constexpr usize ALLOC_WEIGHT            = 0x08ull;
static constexpr usize ALLOC_ACTIVE            = 0x0Cull;

static constexpr i32 TRANSPORT_COVERED         = 0;
static constexpr i32 TRANSPORT_OPEN            = 1;
static constexpr i32 TRANSPORT_AGGREGATE       = 2;
static constexpr i32 TRANSPORT_OIL             = 3;
static constexpr i32 TRANSPORT_DRY_BULK        = 4;
static constexpr i32 TRANSPORT_COOLER          = 5;
static constexpr i32 TRANSPORT_LIVESTOCK       = 6;
static constexpr i32 TRANSPORT_CONCRETE        = 8;
static constexpr i32 TRANSPORT_WATER           = 15;
static constexpr i32 TRANSPORT_WASTE           = 17;
static constexpr usize RESOURCE_COEFF_BASE     = 0xCCull;
static constexpr usize RESOURCE_COEFF_STRIDE   = 0x20ull;
static constexpr usize STORE_ROLE              = 0x94ull;
static constexpr usize TEMPLATE_ENGINE_SPEED   = 0x6D8ull;

static constexpr i32 BUILDING_TYPE_SHOP        = 3;
static constexpr i32 BUILDING_TYPE_POWERPLANT  = 0x11;
static constexpr i32 BUILDING_TYPE_PUB         = 0x17;
static constexpr i32 BUILDING_TYPE_WATER_PUMP  = 0x5A;
static constexpr i32 BUILDING_TYPE_WATER_TREATMENT = 0x61;
static constexpr i32 BUILDING_TYPE_TRASH_CONTAINER = 0x69;

static constexpr i32 STORE_ROLE_DEMAND_BASIC   = 2;
static constexpr i32 STORE_ROLE_DEMAND_MEDIUM  = 3;
static constexpr i32 STORE_ROLE_DEMAND_ADV     = 4;
static constexpr i32 STORE_ROLE_DEMAND_MIDADV  = 5;
static constexpr i32 STORE_ROLE_IMPORT         = 6;
static constexpr i32 STORE_ROLE_EXPORT         = 7;
static constexpr i32 STORE_ROLE_IMPORT_SPECIAL = 8;
static constexpr i32 STORE_ROLE_EXPORT_SPECIAL = 9;
static constexpr i32 STORE_ROLE_DEMAND_HOTEL   = 10;
static constexpr i32 STORE_ROLE_FUEL           = 11;
static constexpr i32 STORE_ROLE_IMPORT_CAR     = 12;
static constexpr i32 STORE_ROLE_IMPORT_SPECIAL2= 14;
static constexpr i32 STORE_ROLE_IMPORT_SPECIAL_TRASH_BIG = 15;
static constexpr i32 STORE_ROLE_TRASH_SMALL    = 16;
static constexpr i32 STORE_ROLE_TRASH_BIG      = 17;

static constexpr float WATER_TANK_MIN_CAPACITY = 10.0f;

static constexpr u32 MAX_MINED_RESOURCES = 96;
static constexpr u32 MAX_HOST_LANES = 1536;
static constexpr u32 MAX_STORAGES = 1536;
static constexpr u32 MAX_ANNEX_STATES = 768;
static constexpr u32 MAX_UI_SHADOW_ENTRIES = 96;
static constexpr u32 MAX_STORAGE_TEMPLATE_OBSERVATIONS = 768;
static constexpr u32 MAX_POOL_RESOURCES = 64;
static constexpr u32 SCAN_DIVISOR = 16;

enum AnnexMode : u8 { ANNEX_INPUT = 1, ANNEX_OUTPUT = 2 };
static constexpr u8 LANE_MASK_INPUT  = 0x01;
static constexpr u8 LANE_MASK_OUTPUT = 0x02;
static constexpr u8 LANE_MASK_BOTH   = LANE_MASK_INPUT | LANE_MASK_OUTPUT;
static inline u8 lane_mode_mask(AnnexMode mode) {
    return mode == ANNEX_INPUT ? LANE_MASK_INPUT : LANE_MASK_OUTPUT;
}

using SimulationDispatchFn = void (__fastcall*)(void* game);
using ConstructionToolFrameFn = void (__fastcall*)(void* game);
using AggregateStorageTouchFn = void (__fastcall*)(void* game, void* building, void* resource);
using GenericStorageUiFn = void (__fastcall*)(i64 param1, i64* param2, i64* param3,
                                              float* param4, float* param5, int param6,
                                              char param7, float param8, u32 param9,
                                              int param10, u32 param11, float param12,
                                              float param13, float param14, char param15,
                                              char param16);
using FactoryConnectionQueryFn = void (__fastcall*)(i64 game, u64 building, int filterType,
                                                    u64 param4, u64 param5, int queryMode,
                                                    char directionFlag);
using VectorPushBackU64Fn = void (__fastcall*)(i64* vectorBeginField, const u64* value);
using ResourceTransferFn = u64 (__fastcall*)(i64 game, u64 building, int transferMode, i64 resource,
                                             float amount, float* residual, float param7,
                                             char exactBuilding, i64 param9);
// FUN_14040BCA0 is a runtime-function entry in 1.1.1.13. Its native callers pass
// game/building in RCX/RDX, mode in R8D, colour selector in R9D, and an enable byte
// in the fifth stack slot. Native colour selector 0 is green.
using BuildingHighlightFn = void (__fastcall*)(u64 game, i64 building, int mode,
                                               int colorSelector, int enabled);
struct C3DVector3 { float x; float y; float z; };
using Collision2DBBoxFn = bool (__fastcall*)(void* manager, C3DVector3* point,
                                             u32* outType, u32* outId,
                                             u32 ignoreType, u32 ignoreId,
                                             char filterFlag, float tolerance);
using GetRotationStepFn = int (__fastcall*)(void* manager, float rotation);
using Vec3TransformCoordFn = C3DVector3* (__fastcall*)(C3DVector3* out,
                                                       const C3DVector3* in,
                                                       const void* matrix);

struct LocalBox2D {
    float minX;
    float minZ;
    float maxX;
    float maxZ;
    u8 valid;
};

struct Point2D {
    float x;
    float z;
};

struct PreviewFootprint {
    C3DVector3 origin;
    const void* forwardRotation;
    const void* inverseRotation;
    LocalBox2D primary;
    LocalBox2D secondary;
};

enum HostRootClass : u8 {
    HOST_RESOURCE_ROOT = 0,
    HOST_STORAGE_DELIVERY_ONLY = 1,
};

struct HostLane {
    u8* building;
    u8* hostStore;
    i32 transport;
    AnnexMode mode;
    u8 rootClass;
    u8 reserved0;
    u8 reserved1;
    u8 reserved2;
};

struct StorageCandidate {
    u8* building;
    u8* store;
    i32 transport;
    u8 modeMask;             // bit 0 = root INPUT lane, bit 1 = root OUTPUT lane
    u8 cargoGateway;
    u8 blueprintOnly;        // template descriptor used for classification only; never mutated
    u8 reserved;
};

struct StorageTemplateObservation {
    u8* templ;
    u32 transportMask;
};

struct MinedResource {
    u8* resource;
    i32 transport;
};

struct PoolResource {
    u8* resource;
    u8* hostRec;
    u8* annexRec;
    float coeff;
    float totalUnits;
    float hostMaxUnits;
    float annexMaxUnits;
    float hostUnits;
};

struct AnnexState {
    u8* host;
    u8* storage;
    u8* hostStore;
    u8* annexStore;
    i32 transport;
    AnnexMode mode;
    float originalHostCapacity;
    float annexCapacity;
    float pooledCapacity;
    float lastSimulationShadowCapacity;
    u8 active;
    u8 conflictWarned;
    u8 contaminationWarned;
    u8 explicitSelection;
};

static const TsmHostCompat* g_host = nullptr;
static u8* g_exeBase = nullptr;
static SimulationDispatchFn g_originalDispatch = nullptr;
static ConstructionToolFrameFn g_originalConstructionFrame = nullptr;
static GenericStorageUiFn g_originalGenericStorageUi = nullptr;
static FactoryConnectionQueryFn g_originalFactoryConnectionQuery = nullptr;
static ResourceTransferFn g_originalResourceTransfer = nullptr;
static VectorPushBackU64Fn g_vectorPushBackU64 = nullptr;
static AggregateStorageTouchFn g_aggregateStorageTouch = nullptr;
static BuildingHighlightFn g_buildingHighlight = nullptr;
static Collision2DBBoxFn g_collision2DBBox = nullptr;
static GetRotationStepFn g_getRotationStep = nullptr;
static Vec3TransformCoordFn g_vec3TransformCoord = nullptr;

static MinedResource g_minedResources[MAX_MINED_RESOURCES] = {};
static u32 g_minedResourceCount = 0;
static u64 g_catalogToolsBegin = 0;
static u64 g_catalogToolsEnd = 0;
static int g_catalogReady = 0;
static int g_catalogOverflowLogged = 0;

static int g_linkGapMeters = 5;
static int g_showPlacementOverlay = 1;
static float g_linkGapSq = 25.0f;
static float g_coarseCandidateRadiusSq = 22500.0f; // 150 m centre prefilter used only by geometry fallbacks
static float g_verticalLimit = 10.0f;

static HostLane g_hostLanes[MAX_HOST_LANES] = {};
static StorageCandidate g_storages[MAX_STORAGES] = {};
static StorageTemplateObservation g_storageTemplateObservations[MAX_STORAGE_TEMPLATE_OBSERVATIONS] = {};
static u32 g_storageTemplateObservationCount = 0;
static AnnexState g_annexState[MAX_ANNEX_STATES] = {};
static u32 g_scanCounter = 0;
static u32 g_currentHostLaneCount = 0;
static u32 g_currentStorageCount = 0;
static int g_reentrant = 0;
static int g_frameReentrant = 0;
static int g_transferReentrant = 0;
static int g_simulationWindowActive = 0;
static int g_transferAmbiguousLogged = 0;
static int g_uiReentrant = 0;
static int g_connectionQueryReentrant = 0;
static int g_freightConnectionUiAmbiguousLogged = 0;
static int g_overflowLogged = 0;
static int g_invalidWorldLogged = 0;
static int g_placementTopologyPrimed = 0;
static int g_ambiguousSelectionLogged = 0;
static int g_gatewayBlueprintOnlyLogged = 0;
static int g_footprintProbeFailureLogged = 0;
static u8* g_cachedOverlayTarget = nullptr;
static u8* g_cachedOverlayToolTemplate = nullptr;

static inline int readable(const void* p, usize n) {
    return g_host && g_host->readablePtr && p && g_host->readablePtr(p, n) != 0;
}

static inline u8 read_u8(const u8* p) { return *p; }
static inline u64 read_u64(const u8* p) { return *(const u64*)p; }
static inline void write_u64(u8* p, u64 v) { *(u64*)p = v; }

static int bytes_equal(const u8* a, const u8* b, usize n) {
    if (!a || !b) return 0;
    for (usize i = 0; i < n; ++i) if (a[i] != b[i]) return 0;
    return 1;
}

// RML and several established native plugins publish a standard 14-byte
// FF25 absolute-entry detour. Accept either the exact WRSR 1.1.1.9 prologue or
// an already-published readable FF25 destination. Passing the live bridge bytes
// back to installInlineHook is intentional: when ESM loads after another owner,
// RML can preserve that owner as ESM's continuation instead of overwriting it.
// All other entry states fail closed rather than guessing instruction boundaries.
// Return: 0=unsupported, 1=vanilla entry, 2=recognized pre-existing detour.
static int hook_expected_bytes(u8* target, const u8* vanillaExpected, usize vanillaSpan,
                               u8 bridgeExpected[ABSOLUTE_ENTRY_BRIDGE_SPAN],
                               const u8** outExpected, usize* outSpan) {
    if (!target || !vanillaExpected || !outExpected || !outSpan) return 0;
    if (readable(target, vanillaSpan) && bytes_equal(target, vanillaExpected, vanillaSpan)) {
        *outExpected = vanillaExpected;
        *outSpan = vanillaSpan;
        return 1;
    }
    if (!readable(target, ABSOLUTE_ENTRY_BRIDGE_SPAN)) return 0;
    if (target[0] != 0xFF || target[1] != 0x25 ||
        target[2] != 0x00 || target[3] != 0x00 ||
        target[4] != 0x00 || target[5] != 0x00) return 0;
    u64 destination = read_u64(target + 6);
    if (!destination || destination == (u64)target || !readable((const void*)destination, 1)) return 0;
    memcpy(bridgeExpected, target, ABSOLUTE_ENTRY_BRIDGE_SPAN);
    *outExpected = bridgeExpected;
    *outSpan = ABSOLUTE_ENTRY_BRIDGE_SPAN;
    return 2;
}

static inline i32 read_i32(const u8* p) { return *(const i32*)p; }
static inline float read_f32(const u8* p) { return *(const float*)p; }
static inline void write_f32(u8* p, float v) { *(float*)p = v; }

static inline int finite_reasonable(float v) {
    return (v == v) && v > -1.0e20f && v < 1.0e20f;
}

static inline float abs_f(float v) { return v < 0.0f ? -v : v; }
static inline float clamp_f(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int valid_vector(u64 begin, u64 end, usize stride, usize maxCount, usize* outCount) {
    if (!outCount || end < begin || stride == 0) return 0;
    u64 bytes = end - begin;
    if ((bytes % stride) != 0) return 0;
    u64 count = bytes / stride;
    if (count > maxCount) return 0;
    if (count != 0 && (!begin || !readable((const void*)begin, (usize)bytes))) return 0;
    *outCount = (usize)count;
    return 1;
}

static inline int capacity_close(float a, float b) {
    if (!finite_reasonable(a) || !finite_reasonable(b)) return 0;
    float scale = abs_f(a);
    if (abs_f(b) > scale) scale = abs_f(b);
    float tolerance = 0.01f + scale * 0.00001f;
    return abs_f(a - b) <= tolerance;
}

static inline u32 transport_bit(i32 transport) {
    return (transport >= 0 && transport < 32) ? (1u << (u32)transport) : 0u;
}

static int supported_annex_transport(i32 transport) {
    return transport == TRANSPORT_COVERED || transport == TRANSPORT_OPEN ||
           transport == TRANSPORT_AGGREGATE || transport == TRANSPORT_OIL ||
           transport == TRANSPORT_DRY_BULK || transport == TRANSPORT_COOLER ||
           transport == TRANSPORT_LIVESTOCK || transport == TRANSPORT_WATER ||
           transport == TRANSPORT_WASTE;
}

static int supported_mine_transport(i32 transport) {
    return transport == TRANSPORT_AGGREGATE || transport == TRANSPORT_OIL;
}

static const char* transport_label(i32 transport) {
    switch (transport) {
        case TRANSPORT_COVERED: return "covered/warehouse";
        case TRANSPORT_OPEN: return "open";
        case TRANSPORT_AGGREGATE: return "aggregate";
        case TRANSPORT_OIL: return "oil/liquid";
        case TRANSPORT_DRY_BULK: return "dry-bulk";
        case TRANSPORT_COOLER: return "refrigerated/cold";
        case TRANSPORT_LIVESTOCK: return "livestock";
        case TRANSPORT_WATER: return "water";
        case TRANSPORT_WASTE: return "waste";
        default: return "unsupported";
    }
}

static int store_direction_matches(u8* store, AnnexMode mode) {
    if (!store || !readable(store, STORE_STRIDE)) return 0;
    i32 role = read_i32(store + STORE_ROLE);
    u8 blockOut = read_u8(store + STORE_BLOCK_OUTPUT);
    u8 blockIn = read_u8(store + STORE_BLOCK_INPUT);
    if (mode == ANNEX_OUTPUT) {
        return role == STORE_ROLE_EXPORT || role == STORE_ROLE_EXPORT_SPECIAL ||
               role == STORE_ROLE_TRASH_SMALL || role == STORE_ROLE_TRASH_BIG ||
               (blockOut == 0 && blockIn != 0);
    }
    if (mode == ANNEX_INPUT) {
        return role == STORE_ROLE_DEMAND_BASIC || role == STORE_ROLE_DEMAND_MEDIUM ||
               role == STORE_ROLE_DEMAND_ADV || role == STORE_ROLE_DEMAND_MIDADV ||
               role == STORE_ROLE_IMPORT || role == STORE_ROLE_IMPORT_SPECIAL ||
               role == STORE_ROLE_DEMAND_HOTEL || role == STORE_ROLE_FUEL ||
               role == STORE_ROLE_IMPORT_CAR || role == STORE_ROLE_IMPORT_SPECIAL2 ||
               role == STORE_ROLE_IMPORT_SPECIAL_TRASH_BIG ||
               (blockOut != 0 && blockIn == 0);
    }
    return 0;
}

static float resource_coefficient(u8* resource, i32 transport);
static int template_resource_vector(u8* templ, usize beginOffset, usize endOffset,
                                    u64* outBegin, usize* outCount);
static u8* recipe_resource_at(u64 begin, usize index, usize rateOffset, float* outRate);

static int building_store_vector(u8* building, u64* outBegin, usize* outCount) {
    if (!building || !outBegin || !outCount ||
        !readable(building, BUILDING_STORES_END + sizeof(u64))) return 0;
    u64 begin = read_u64(building + BUILDING_STORES_BEGIN);
    u64 end = read_u64(building + BUILDING_STORES_END);
    usize count = 0;
    if (!valid_vector(begin, end, STORE_STRIDE, 64, &count)) return 0;
    *outBegin = begin;
    *outCount = count;
    return 1;
}

static int template_store_vector(u8* templ, u64* outBegin, usize* outCount) {
    if (!templ || !outBegin || !outCount ||
        !readable(templ + TEMPLATE_STORES_END, sizeof(u64))) return 0;
    u64 begin = read_u64(templ + TEMPLATE_STORES_BEGIN);
    u64 end = read_u64(templ + TEMPLATE_STORES_END);
    usize count = 0;
    if (!valid_vector(begin, end, STORE_STRIDE, 64, &count)) return 0;
    *outBegin = begin;
    *outCount = count;
    return 1;
}

static u32 template_storage_transport_mask(u8* templ) {
    u64 begin = 0;
    usize count = 0;
    if (!template_store_vector(templ, &begin, &count)) return 0;
    u32 mask = 0;
    for (usize i = 0; i < count; ++i) {
        u8* store = (u8*)(begin + i * STORE_STRIDE);
        i32 transport = read_i32(store + STORE_TRANSPORT);
        float cap = read_f32(store + STORE_CAPACITY);
        if (!supported_annex_transport(transport) || !finite_reasonable(cap)) continue;
        // Special trash descriptors are authored with zero template capacity and
        // receive live container-slot capacity later; they are still real WASTE
        // storage blueprints for placement/gateway classification.
        if (cap > 0.000001f || transport == TRANSPORT_WASTE) mask |= transport_bit(transport);
    }
    return mask;
}

static int template_store_has_recipe_resource(u8* store, u8* templ, AnnexMode mode) {
    if (!store || !templ || !readable(store, STORE_STRIDE)) return 0;
    i32 transport = read_i32(store + STORE_TRANSPORT);
    if (!supported_annex_transport(transport)) return 0;
    usize beginOff = mode == ANNEX_INPUT ? TEMPLATE_CONSUME_BEGIN : TEMPLATE_PRODUCE_BEGIN;
    usize endOff = mode == ANNEX_INPUT ? TEMPLATE_CONSUME_END : TEMPLATE_PRODUCE_END;
    u64 rb = 0;
    usize rc = 0;
    if (!template_resource_vector(templ, beginOff, endOff, &rb, &rc)) return 0;
    for (usize i = 0; i < rc; ++i) {
        u8* resource = recipe_resource_at(rb, i, mode == ANNEX_INPUT ? RECIPE_INPUT_RATE : RECIPE_OUTPUT_RATE, nullptr);
        if (resource && resource_coefficient(resource, transport) > 0.0f) return 1;
    }
    return 0;
}

static int store_belongs_to_building(u8* building, u8* store) {
    u64 begin = 0;
    usize count = 0;
    if (!store || !building_store_vector(building, &begin, &count)) return 0;
    u64 p = (u64)store;
    return p >= begin && p < begin + count * STORE_STRIDE && ((p - begin) % STORE_STRIDE) == 0;
}

static u8* find_resource_record(u8* store, u8* resource) {
    if (!store || !resource || !readable(store, STORE_STRIDE)) return nullptr;
    u64 begin = read_u64(store + STORE_RECORDS_BEGIN);
    u64 end = read_u64(store + STORE_RECORDS_END);
    usize count = 0;
    if (!valid_vector(begin, end, RECORD_STRIDE, 256, &count)) return nullptr;
    for (usize i = 0; i < count; ++i) {
        u8* rec = (u8*)(begin + i * RECORD_STRIDE);
        if (read_u64(rec + RECORD_RESOURCE) == (u64)resource) return rec;
    }
    return nullptr;
}

static int resource_record_index(u8* store, u8* resource, usize* outIndex) {
    if (!store || !resource || !outIndex || !readable(store, STORE_STRIDE)) return 0;
    u64 begin = read_u64(store + STORE_RECORDS_BEGIN);
    u64 end = read_u64(store + STORE_RECORDS_END);
    usize count = 0;
    if (!valid_vector(begin, end, RECORD_STRIDE, 256, &count)) return 0;
    for (usize i = 0; i < count; ++i) {
        u8* rec = (u8*)(begin + i * RECORD_STRIDE);
        if (read_u64(rec + RECORD_RESOURCE) == (u64)resource) {
            *outIndex = i;
            return 1;
        }
    }
    return 0;
}

// WRSR 1.1.1.9 FUN_140428C70 / FUN_140428DC0 pair the resource and
// allocation vectors BY INDEX: resource identity comes from store+0x00, while
// weight/active come from store+0x18 at the same index. The allocation record's
// first qword is therefore not used as a generic resource key here.
static u8* allocation_entry_for_index(u8* store, usize index) {
    if (!store || !readable(store, STORE_STRIDE)) return nullptr;
    u64 begin = read_u64(store + STORE_ALLOC_BEGIN);
    u64 end = read_u64(store + STORE_ALLOC_END);
    usize count = 0;
    if (!valid_vector(begin, end, ALLOC_STRIDE, 256, &count) || index >= count) return nullptr;
    return (u8*)(begin + index * ALLOC_STRIDE);
}

static u8* find_allocation_entry(u8* store, u8* resource) {
    usize index = 0;
    return resource_record_index(store, resource, &index) ? allocation_entry_for_index(store, index) : nullptr;
}

static float resource_coefficient(u8* resource, i32 transport) {
    if (!resource || transport < 0 || transport > 31) return 0.0f;
    usize off = RESOURCE_COEFF_BASE + (usize)transport * RESOURCE_COEFF_STRIDE;
    if (!readable(resource + off, sizeof(float))) return 0.0f;
    float v = read_f32(resource + off);
    if (!finite_reasonable(v) || v <= 0.000001f || v > 1000000.0f) return 0.0f;
    return v;
}

static int template_building_type(u8* templ, i32* outType) {
    if (!templ || !outType || !readable(templ + TEMPLATE_RUNTIME_TYPE, sizeof(i32))) return 0;
    i32 type = read_i32(templ + TEMPLATE_RUNTIME_TYPE);
    if (type < 0 || type > 300) return 0;
    *outType = type;
    return 1;
}

static int template_resource_vector(u8* templ, usize beginOffset, usize endOffset,
                                    u64* outBegin, usize* outCount) {
    if (!templ || !outBegin || !outCount || !readable(templ + endOffset, sizeof(u64))) return 0;
    u64 begin = read_u64(templ + beginOffset);
    u64 end = read_u64(templ + endOffset);
    usize count = 0;
    if (!valid_vector(begin, end, RECIPE_STRIDE, 64, &count)) return 0;
    *outBegin = begin;
    *outCount = count;
    return 1;
}

static u8* recipe_resource_at(u64 begin, usize index, usize rateOffset, float* outRate) {
    u8* rec = (u8*)(begin + index * RECIPE_STRIDE);
    if (!readable(rec, RECIPE_STRIDE)) return nullptr;
    u8* resource = (u8*)read_u64(rec + RECIPE_RESOURCE);
    if (!resource || !readable(resource, 1)) return nullptr;
    // Input and output recipe records store their rate at different offsets.
    // Most catalog/compatibility paths only need the resource pointer and must
    // not reject a valid record based on an unrelated +0x08 field.
    if (outRate) {
        if (rateOffset > RECIPE_STRIDE - sizeof(float)) return nullptr;
        float rate = read_f32(rec + rateOffset);
        if (!finite_reasonable(rate) || rate <= 0.000001f) return nullptr;
        *outRate = rate;
    }
    return resource;
}

static i32 resource_supported_transport(u8* resource, u32 allowedMask) {
    if (!resource) return -1;
    const i32 transports[] = {TRANSPORT_COVERED, TRANSPORT_OPEN, TRANSPORT_AGGREGATE, TRANSPORT_OIL,
                              TRANSPORT_DRY_BULK, TRANSPORT_COOLER, TRANSPORT_LIVESTOCK,
                              TRANSPORT_WATER, TRANSPORT_WASTE, TRANSPORT_CONCRETE};
    for (usize i = 0; i < sizeof(transports)/sizeof(transports[0]); ++i) {
        i32 t = transports[i];
        if ((allowedMask & transport_bit(t)) && resource_coefficient(resource, t) > 0.0f) return t;
    }
    return -1;
}

static int mined_resource_present(u8* resource, i32 transport) {
    if (!resource) return 0;
    for (u32 i = 0; i < g_minedResourceCount; ++i) {
        if (g_minedResources[i].resource == resource && g_minedResources[i].transport == transport) return 1;
    }
    return 0;
}

static void add_mined_resource(u8* resource, i32 transport) {
    if (!resource || transport < 0 || mined_resource_present(resource, transport)) return;
    if (g_minedResourceCount < MAX_MINED_RESOURCES) {
        g_minedResources[g_minedResourceCount].resource = resource;
        g_minedResources[g_minedResourceCount].transport = transport;
        ++g_minedResourceCount;
    } else if (!g_catalogOverflowLogged && g_host && g_host->log) {
        g_host->log("[EnhancedStorageManagement] mined-resource discovery limit reached; additional compatible resources ignored");
        g_catalogOverflowLogged = 1;
    }
}

static int template_has_recipe_transport(u8* templ, usize beginOffset, usize endOffset, i32 transport) {
    u64 begin = 0;
    usize count = 0;
    if (!template_resource_vector(templ, beginOffset, endOffset, &begin, &count)) return 0;
    for (usize i = 0; i < count; ++i) {
        u8* resource = recipe_resource_at(begin, i, beginOffset == TEMPLATE_CONSUME_BEGIN ? RECIPE_INPUT_RATE : RECIPE_OUTPUT_RATE, nullptr);
        if (resource && resource_coefficient(resource, transport) > 0.0f) return 1;
    }
    return 0;
}

// Placement preview must prove that the prospective storage blueprint contains
// an exact resource used by the host recipe. A shared transport family alone is
// not enough: e.g. an OPEN store for chalk must not highlight a liquor factory
// merely because one of its recipe resources can also use OPEN transport.
static int template_storage_store_matches_host_recipe(u8* storageStore, u8* hostTempl,
                                                       AnnexMode mode) {
    if (!storageStore || !hostTempl || !readable(storageStore, STORE_STRIDE)) return 0;
    i32 transport = read_i32(storageStore + STORE_TRANSPORT);
    if (!supported_annex_transport(transport)) return 0;
    usize beginOffset = mode == ANNEX_INPUT ? TEMPLATE_CONSUME_BEGIN : TEMPLATE_PRODUCE_BEGIN;
    usize endOffset = mode == ANNEX_INPUT ? TEMPLATE_CONSUME_END : TEMPLATE_PRODUCE_END;
    u64 begin = 0;
    usize count = 0;
    if (!template_resource_vector(hostTempl, beginOffset, endOffset, &begin, &count) || count == 0) return 0;
    for (usize i = 0; i < count; ++i) {
        u8* resource = recipe_resource_at(begin, i,
            mode == ANNEX_INPUT ? RECIPE_INPUT_RATE : RECIPE_OUTPUT_RATE, nullptr);
        if (!resource || resource_coefficient(resource, transport) <= 0.0f) continue;
        if (find_resource_record(storageStore, resource)) return 1;
    }
    return 0;
}

// Completed host lanes are selected from their live descriptors. A storage
// template's resource-record vector is not reliably populated before placement,
// so intersect the host's authoritative resource list with the candidate
// transport's native resource coefficient instead of requiring template records.
static int template_store_matches_live_host_store(u8* storageTempl, u8* hostStore,
                                                    i32 transport) {
    if (!storageTempl || !hostStore || !readable(hostStore, STORE_STRIDE) ||
        !supported_annex_transport(transport)) return 0;
    u64 begin = 0;
    usize count = 0;
    if (!template_store_vector(storageTempl, &begin, &count)) return 0;
    int hasCandidateTransport = 0;
    for (usize i = 0; i < count; ++i) {
        u8* blueprint = (u8*)(begin + i * STORE_STRIDE);
        if (read_i32(blueprint + STORE_TRANSPORT) != transport) continue;
        float capacity = read_f32(blueprint + STORE_CAPACITY);
        if (!finite_reasonable(capacity) || capacity <= 0.000001f) continue;
        hasCandidateTransport = 1;
        break;
    }
    if (!hasCandidateTransport) return 0;

    u64 rb = read_u64(hostStore + STORE_RECORDS_BEGIN);
    u64 re = read_u64(hostStore + STORE_RECORDS_END);
    usize rc = 0;
    if (!valid_vector(rb, re, RECORD_STRIDE, 256, &rc)) return 0;
    for (usize ri = 0; ri < rc; ++ri) {
        u8* resource = (u8*)read_u64((u8*)(rb + ri * RECORD_STRIDE) + RECORD_RESOURCE);
        if (resource && resource_coefficient(resource, transport) > 0.0f) return 1;
    }
    return 0;
}


static int store_matches_recipe(u8* store, u8* templ, usize beginOffset, usize endOffset) {
    if (!store || !templ || !readable(store, STORE_STRIDE)) return 0;
    i32 transport = read_i32(store + STORE_TRANSPORT);
    if (!supported_annex_transport(transport) && transport != TRANSPORT_CONCRETE) return 0;
    u64 begin = 0;
    usize count = 0;
    if (!template_resource_vector(templ, beginOffset, endOffset, &begin, &count)) return 0;
    for (usize i = 0; i < count; ++i) {
        u8* resource = recipe_resource_at(begin, i, beginOffset == TEMPLATE_CONSUME_BEGIN ? RECIPE_INPUT_RATE : RECIPE_OUTPUT_RATE, nullptr);
        // Recipe compatibility is a template property. Do not require the live
        // resource-record vector to have materialised yet: empty water/fuel and
        // several specialised buffers are valid before their first delivery.
        if (resource && resource_coefficient(resource, transport) > 0.0f) return 1;
    }
    return 0;
}

static int store_has_pool_resources(u8* store) {
    if (!store || !readable(store, STORE_STRIDE)) return 0;
    i32 transport = read_i32(store + STORE_TRANSPORT);
    u64 begin = read_u64(store + STORE_RECORDS_BEGIN);
    u64 end = read_u64(store + STORE_RECORDS_END);
    usize count = 0;
    if (!valid_vector(begin, end, RECORD_STRIDE, MAX_POOL_RESOURCES, &count) || count == 0) return 0;
    for (usize i = 0; i < count; ++i) {
        u8* res = (u8*)read_u64((u8*)begin + i * RECORD_STRIDE + RECORD_RESOURCE);
        if (res && resource_coefficient(res, transport) > 0.0f) return 1;
    }
    return 0;
}

static int template_engine_idle(u8* templ) {
    if (!templ || !readable(templ + TEMPLATE_ENGINE_SPEED, sizeof(float))) return 0;
    float speed = read_f32(templ + TEMPLATE_ENGINE_SPEED);
    return finite_reasonable(speed) && abs_f(speed) <= 0.001f;
}

static u8* building_template(u8* building);
static int building_is_runtime_type(u8* building, i32 wantedType);
static u8* template_store_for_live_store(u8* building, u8* liveStore, i32 expectedTransport);
static int authored_capacity_for_live_store(u8* building, u8* liveStore,
                                            i32 expectedTransport, float* outCapacity);
static int fuel_service_type(i32 type);
static float group_resource_capacity_for_mechanics(u8* hostStore, u8* resource);
static int template_has_native_fuel_store(u8* templ);
static u32 collect_unique_active_pool_roots_for_member(u8* member, u8** outRoots, u32 outCapacity);
static u8* active_pool_root_for_member(u8* member);
static u32 cargo_template_transport_mask(void* game, u8* templ);
static u8 cargo_template_mode_mask(void* game, u8* templ, i32 transport);

static int annex_building_store_eligible(u8* building, u8* store) {
    if (!building || !store || !readable(store, STORE_STRIDE)) return 0;
    if (!readable(building + BUILDING_TEMPLATE, sizeof(u64))) return 0;
    u8* templ = (u8*)read_u64(building + BUILDING_TEMPLATE);
    i32 type = -1;
    if (!templ || !template_building_type(templ, &type)) return 0;
    i32 transport = read_i32(store + STORE_TRANSPORT);
    float cap = read_f32(store + STORE_CAPACITY);
    if (!finite_reasonable(cap) || cap <= 0.01f || !supported_annex_transport(transport)) return 0;

    // Cargo stations are genuine transfer gateways even when their tiny native
    // 1 t routing descriptor has not materialised a resource-record vector yet.
    // WRSR's stock rail/road aggregate loaders use exactly this topology. Keep
    // them discoverable from type + transport + geometry. Their live resource
    // records are routing state, not structural compatibility evidence.
    if (type == BUILDING_TYPE_CARGO_STATION) {
        i32 role = read_i32(store + STORE_ROLE);
        return role != STORE_ROLE_FUEL;
    }

    // Ordinary storage annexes must expose a real live resource pool. This is
    // deliberately transport-generic across the supported WRSR storage classes,
    // including refrigerated (5) and livestock (6), so Workshop storage variants
    // do not need a name/ID allow-list.
    if (type == BUILDING_TYPE_STORAGE) return store_has_pool_resources(store);

    if (type == BUILDING_TYPE_WATER_PUMP && transport == TRANSPORT_WATER &&
        cap >= WATER_TANK_MIN_CAPACITY && template_engine_idle(templ)) return store_has_pool_resources(store);
    if (type == BUILDING_TYPE_TRASH_CONTAINER && transport == TRANSPORT_WASTE) {
        i32 role = read_i32(store + STORE_ROLE);
        return role == STORE_ROLE_TRASH_SMALL || role == STORE_ROLE_TRASH_BIG ||
               role == STORE_ROLE_IMPORT_SPECIAL_TRASH_BIG || store_has_pool_resources(store);
    }
    return 0;
}

static int append_supported_storage_candidates(void* game, u8* building, u32* storageCount, u32* outMask) {
    if (!game || !building || !storageCount) return 0;
    u8* templ = building_template(building);
    i32 type = -1;
    if (!templ || !template_building_type(templ, &type)) return 0;
    int found = 0;
    u32 mask = 0;

    // First use real per-building live storage descriptors. These are the only
    // descriptors Enhanced Storage Management is ever allowed to mutate or stage cargo in.
    u64 begin = 0;
    usize count = 0;
    if (building_store_vector(building, &begin, &count)) {
        for (usize i = 0; i < count; ++i) {
            u8* store = (u8*)(begin + i * STORE_STRIDE);
            if (!annex_building_store_eligible(building, store)) continue;
            i32 transport = read_i32(store + STORE_TRANSPORT);
            found = 1;
            mask |= transport_bit(transport);
            if (*storageCount >= MAX_STORAGES) return -1;
            StorageCandidate* candidate = &g_storages[(*storageCount)++];
            *candidate = StorageCandidate{};
            candidate->building = building;
            candidate->store = store;
            candidate->transport = transport;
            candidate->modeMask = type == BUILDING_TYPE_CARGO_STATION
                ? cargo_template_mode_mask(game, templ, transport) : LANE_MASK_BOTH;
            candidate->cargoGateway = type == BUILDING_TYPE_CARGO_STATION ? 1 : 0;
            candidate->blueprintOnly = 0;
        }
    }

    // Specialised aggregate/oil/bulk/water cargo stations often do not expose
    // their 1 t routing descriptor in building+0x970 until vanilla network state
    // exists. Their template still has the exact 0xE0 descriptor at +0x778. Add
    // classification-only candidates for missing transports so placement and
    // gateway classification does not incorrectly call them unsupported. The
    // blueprint is NEVER written to and can never become an AnnexState.
    if (type == BUILDING_TYPE_CARGO_STATION) {
        u64 tb = 0;
        usize tc = 0;
        if (template_store_vector(templ, &tb, &tc)) {
            for (usize i = 0; i < tc; ++i) {
                u8* tstore = (u8*)(tb + i * STORE_STRIDE);
                i32 transport = read_i32(tstore + STORE_TRANSPORT);
                if (!supported_annex_transport(transport)) continue;
                float cap = read_f32(tstore + STORE_CAPACITY);
                if (!finite_reasonable(cap) || cap <= 0.000001f) continue;
                u32 bit = transport_bit(transport);
                if (mask & bit) continue;
                found = 1;
                mask |= bit;
                if (*storageCount >= MAX_STORAGES) return -1;
                StorageCandidate* candidate = &g_storages[(*storageCount)++];
                *candidate = StorageCandidate{};
                candidate->building = building;
                candidate->store = tstore;
                candidate->transport = transport;
                candidate->modeMask = cargo_template_mode_mask(game, templ, transport);
                candidate->cargoGateway = 1;
                candidate->blueprintOnly = 1;
            }
        }
    }

    if (outMask) *outMask = mask;
    return found;
}

static u32 observed_storage_template_mask(u8* templ) {
    if (!templ) return 0;
    for (u32 i = 0; i < g_storageTemplateObservationCount; ++i) {
        if (g_storageTemplateObservations[i].templ == templ) return g_storageTemplateObservations[i].transportMask;
    }
    return 0;
}

static int storage_template_observed(u8* templ) {
    if (!templ) return 0;
    for (u32 i = 0; i < g_storageTemplateObservationCount; ++i) if (g_storageTemplateObservations[i].templ == templ) return 1;
    return 0;
}

static void observe_storage_template(u8* templ, u32 transportMask) {
    if (!templ) return;
    for (u32 i = 0; i < g_storageTemplateObservationCount; ++i) {
        StorageTemplateObservation* o = &g_storageTemplateObservations[i];
        if (o->templ != templ) continue;
        o->transportMask |= transportMask;
        return;
    }
    if (g_storageTemplateObservationCount >= MAX_STORAGE_TEMPLATE_OBSERVATIONS) return;
    StorageTemplateObservation* o = &g_storageTemplateObservations[g_storageTemplateObservationCount++];
    o->templ = templ;
    o->transportMask = transportMask;
}

static int loaded_tool_vector(void* game, u64* outBegin, usize* outCount, u64* outEnd) {
    if (!game || !outBegin || !outCount || !outEnd ||
        !readable((u8*)game + GAME_TOOLS_BEGIN, 16)) return 0;
    u64 begin = read_u64((u8*)game + GAME_TOOLS_BEGIN);
    u64 end = read_u64((u8*)game + GAME_TOOLS_END);
    usize count = 0;
    if (!valid_vector(begin, end, TOOL_STRIDE, 20000, &count) || count == 0) return 0;
    *outBegin = begin;
    *outCount = count;
    *outEnd = end;
    return 1;
}

static int ensure_type_catalog(void* game) {
    if (!game) return 0;
    u64 toolsBegin = 0, toolsEnd = 0;
    usize toolCount = 0;
    if (!loaded_tool_vector(game, &toolsBegin, &toolCount, &toolsEnd)) return 0;
    if (g_catalogReady && toolsBegin == g_catalogToolsBegin && toolsEnd == g_catalogToolsEnd) return 1;

    g_minedResourceCount = 0;
    for (u32 i = 0; i < MAX_MINED_RESOURCES; ++i) {
        g_minedResources[i].resource = nullptr;
        g_minedResources[i].transport = -1;
    }

    int foundDefinition = 0;
    for (usize i = 0; i < toolCount; ++i) {
        u8* tool = (u8*)(toolsBegin + i * TOOL_STRIDE);
        if (!readable(tool + TOOL_TEMPLATE, sizeof(u64))) continue;
        u8* templ = (u8*)read_u64(tool + TOOL_TEMPLATE);
        i32 type = -1;
        if (!templ || !template_building_type(templ, &type)) continue;
        foundDefinition = 1;
        if (type != BUILDING_TYPE_MINE) continue;

        u64 produced = 0;
        usize producedCount = 0;
        if (!template_resource_vector(templ, TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END,
                                      &produced, &producedCount)) continue;
        for (usize r = 0; r < producedCount; ++r) {
            u8* resource = recipe_resource_at(produced, r, RECIPE_OUTPUT_RATE, nullptr);
            if (!resource) continue;
            i32 transport = resource_supported_transport(
                resource, transport_bit(TRANSPORT_AGGREGATE) | transport_bit(TRANSPORT_OIL));
            if (supported_mine_transport(transport)) add_mined_resource(resource, transport);
        }
    }

    g_catalogToolsBegin = toolsBegin;
    g_catalogToolsEnd = toolsEnd;
    g_catalogReady = foundDefinition;
    return g_catalogReady;
}

static int world_building_vector(void* game, u64* outBegin, usize* outCount) {
    if (!game || !outBegin || !outCount || !readable((u8*)game + GAME_BUILDINGS_BEGIN, 16)) return 0;
    u64 begin = read_u64((u8*)game + GAME_BUILDINGS_BEGIN);
    u64 end = read_u64((u8*)game + GAME_BUILDINGS_END);
    usize count = 0;
    if (!valid_vector(begin, end, sizeof(u64), 200000, &count)) return 0;
    *outBegin = begin;
    *outCount = count;
    return 1;
}

static int completed_building(u8* building) {
    if (!building || !readable(building, BUILDING_STORES_END + sizeof(u64))) return 0;
    float complete = read_f32(building + BUILDING_COMPLETION);
    return finite_reasonable(complete) && complete >= 0.999f;
}

static u8* building_template(u8* building) {
    if (!building || !readable(building + BUILDING_TEMPLATE, sizeof(u64))) return nullptr;
    return (u8*)read_u64(building + BUILDING_TEMPLATE);
}

static int template_recipe_contains_resource(u8* templ, AnnexMode mode, u8* resource) {
    if (!templ || !resource) return 0;
    usize beginOffset = mode == ANNEX_INPUT ? TEMPLATE_CONSUME_BEGIN : TEMPLATE_PRODUCE_BEGIN;
    usize endOffset   = mode == ANNEX_INPUT ? TEMPLATE_CONSUME_END   : TEMPLATE_PRODUCE_END;
    u64 begin = 0;
    usize count = 0;
    if (!template_resource_vector(templ, beginOffset, endOffset, &begin, &count)) return 0;
    for (usize i = 0; i < count; ++i) {
        if (recipe_resource_at(begin, i, beginOffset == TEMPLATE_CONSUME_BEGIN ? RECIPE_INPUT_RATE : RECIPE_OUTPUT_RATE, nullptr) == resource) return 1;
    }
    return 0;
}

// A host descriptor can expose more records than the production lane actually
// consumes/produces. That is especially common with covered/open/cold Workshop
// stores. Recipe membership therefore wins over raw descriptor overlap for
// production buildings; shops/pubs and waste lanes fall back to the live store.
static int lane_accepts_resource(const HostLane* lane, u8* resource) {
    if (!lane || !lane->building || !lane->hostStore || !resource) return 0;
    if (read_i32(lane->hostStore + STORE_TRANSPORT) != lane->transport) return 0;
    if (resource_coefficient(resource, lane->transport) <= 0.0f) return 0;

    // Prefer exact identity from the LIVE descriptor. If the live vector is
    // populated, absence is authoritative: a same-transport resource belongs to
    // some other buffer and must not bleed into this one.
    u8* exactLive = find_resource_record(lane->hostStore, resource);
    if (!exactLive) {
        u64 liveBegin = read_u64(lane->hostStore + STORE_RECORDS_BEGIN);
        u64 liveEnd = read_u64(lane->hostStore + STORE_RECORDS_END);
        usize liveCount = 0;
        if (!valid_vector(liveBegin, liveEnd, RECORD_STRIDE, MAX_POOL_RESOURCES, &liveCount)) return 0;
        if (liveCount != 0) return 0;

        // Some valid empty buffers (notably water and fuel/service stores) do
        // not materialise their live resource vector until first use. Recover
        // the corresponding immutable template descriptor and require the same
        // exact resource there. This also supports empty compatible buffers
        // without reopening its wrong-same-transport descriptor regression.
        u8* blueprint = template_store_for_live_store(lane->building, lane->hostStore, lane->transport);
        if (blueprint) {
            u64 bpBegin = read_u64(blueprint + STORE_RECORDS_BEGIN);
            u64 bpEnd = read_u64(blueprint + STORE_RECORDS_END);
            usize bpCount = 0;
            if (!valid_vector(bpBegin, bpEnd, RECORD_STRIDE, MAX_POOL_RESOURCES, &bpCount)) return 0;
            if (bpCount != 0) {
                if (!find_resource_record(blueprint, resource)) return 0;
            } else {
                // A genuinely record-less blueprint can only be inferred for a
                // recipe lane that is unique for this root/transport/direction.
                // This covers simple one-water-input factories but refuses an
                // ambiguous pair of empty same-transport input stores.
                if (lane->transport == TRANSPORT_WASTE) return 0;
                u8* templ = building_template(lane->building);
                if (!templ || !template_recipe_contains_resource(templ, lane->mode, resource)) return 0;
                u32 same = 0;
                for (u32 i = 0; i < g_currentHostLaneCount; ++i) {
                    HostLane* other = &g_hostLanes[i];
                    if (other->building == lane->building && other->transport == lane->transport &&
                        other->mode == lane->mode) ++same;
                }
                if (same != 1) return 0;
            }
        } else {
            return 0;
        }
    }

    // Waste is intentionally exact-match only. A construction-waste container
    // stand must not redefine a factory whose native waste record is mixed waste.
    if (lane->transport == TRANSPORT_WASTE) return 1;

    u8* templ = building_template(lane->building);
    if (!templ) return 0;
    // Any building class with a real recipe (factories, farms, heating plants,
    // vehicle/rail/air/ship production lines, mines, etc.) uses that recipe to
    // scope a lane. Buildings whose storage is operational rather than a recipe
    // input (fuel stores in DO/CO/technical/repair offices, shops, etc.) have no
    // matching recipe vector and fall back to the exact live/blueprint descriptor.
    usize beginOff = lane->mode == ANNEX_INPUT ? TEMPLATE_CONSUME_BEGIN : TEMPLATE_PRODUCE_BEGIN;
    usize endOff   = lane->mode == ANNEX_INPUT ? TEMPLATE_CONSUME_END   : TEMPLATE_PRODUCE_END;
    u64 rb = 0;
    usize rc = 0;
    if (template_resource_vector(templ, beginOff, endOff, &rb, &rc) && rc != 0)
        return template_recipe_contains_resource(templ, lane->mode, resource);
    return 1;
}

static int lane_annex_store_overlap(const HostLane* lane, u8* annexStore) {
    if (!lane || !annexStore || !readable(annexStore, STORE_STRIDE)) return 0;
    if (read_i32(annexStore + STORE_TRANSPORT) != lane->transport) return 0;
    u64 begin = read_u64(annexStore + STORE_RECORDS_BEGIN);
    u64 end = read_u64(annexStore + STORE_RECORDS_END);
    usize count = 0;
    if (!valid_vector(begin, end, RECORD_STRIDE, MAX_POOL_RESOURCES, &count) || count == 0) return 0;
    for (usize i = 0; i < count; ++i) {
        u8* resource = (u8*)read_u64((u8*)begin + i * RECORD_STRIDE + RECORD_RESOURCE);
        if (resource && lane_accepts_resource(lane, resource)) return 1;
    }
    return 0;
}

static int state_accepts_resource(const AnnexState* s, u8* resource) {
    if (!s || !resource || !s->active) return 0;
    HostLane lane = {s->host, s->hostStore, s->transport, s->mode, HOST_RESOURCE_ROOT, 0, 0, 0};
    return lane_accepts_resource(&lane, resource);
}

static int valid_capacity_store(u8* store, i32 expectedTransport, float* outCapacity) {
    if (!store || !outCapacity || !readable(store, STORE_STRIDE)) return 0;
    i32 transport = read_i32(store + STORE_TRANSPORT);
    if (expectedTransport >= 0 && transport != expectedTransport) return 0;
    if (!supported_annex_transport(transport)) return 0;
    float capacity = read_f32(store + STORE_CAPACITY);
    if (!finite_reasonable(capacity) || capacity <= 0.01f || capacity > 100000000.0f) return 0;
    *outCapacity = capacity;
    return 1;
}

static int building_center(u8* building, C3DVector3* out);
 
static int position_distance_sq(u8* a, u8* b, float* outSq) {
    if (!a || !b || !outSq) return 0;
    C3DVector3 pa{}, pb{};
    if (!building_center(a, &pa) || !building_center(b, &pb)) return 0;
    if (abs_f(pa.y - pb.y) > g_verticalLimit) return 0;
    float dx = pa.x - pb.x;
    float dz = pa.z - pb.z;
    float sq = dx * dx + dz * dz;
    if (!finite_reasonable(sq) || sq < 0.0f) return 0;
    *outSq = sq;
    return 1;
}

static int cursor_distance_sq(void* game, u8* building, float* outSq) {
    if (!game || !building || !outSq) return 0;
    u8* world = (u8*)game;
    C3DVector3 buildingPos{};
    if (!readable(world + GAME_CURSOR_X, 12) || !building_center(building, &buildingPos)) return 0;
    float cx = read_f32(world + GAME_CURSOR_X);
    float cy = read_f32(world + GAME_CURSOR_Y);
    float cz = read_f32(world + GAME_CURSOR_Z);
    if (!finite_reasonable(cx) || !finite_reasonable(cy) || !finite_reasonable(cz)) return 0;
    if (abs_f(cy - buildingPos.y) > g_verticalLimit) return 0;
    float dx = cx - buildingPos.x;
    float dz = cz - buildingPos.z;
    float sq = dx * dx + dz * dz;
    if (!finite_reasonable(sq) || sq < 0.0f) return 0;
    *outSq = sq;
    return 1;
}

static u8* current_tool_template(void* game) {
    if (!game || !readable((u8*)game + GAME_CURRENT_TOOL, sizeof(u64))) return nullptr;
    u8* tool = (u8*)read_u64((u8*)game + GAME_CURRENT_TOOL);
    if (!tool || !readable(tool + TOOL_TEMPLATE, sizeof(u64))) return nullptr;
    u8* templ = (u8*)read_u64(tool + TOOL_TEMPLATE);
    return templ && readable(templ + TEMPLATE_RUNTIME_TYPE, sizeof(i32)) ? templ : nullptr;
}

// Derive the root-lane direction that an unbuilt template will expose once its
// live 0xE0 stores materialise. This is used only for construction feedback;
// completed mechanics still require the real building+0x970 descriptors.
static u8 template_host_mode_mask_for_transport(u8* templ, i32 transport) {
    if (!templ || !supported_annex_transport(transport)) return 0;
    i32 type = -1;
    if (!template_building_type(templ, &type)) return 0;
    const int recipeFactory = type == BUILDING_TYPE_FACTORY ||
                              type == BUILDING_TYPE_WATER_TREATMENT ||
                              type == BUILDING_TYPE_PRODUCTION_LINE;
    const int inputOnlyRecipe = type == BUILDING_TYPE_POWERPLANT ||
                                type == BUILDING_TYPE_HEATING_PLANT;
    const int mine = type == BUILDING_TYPE_MINE;
    const int shopPub = type == BUILDING_TYPE_SHOP || type == BUILDING_TYPE_PUB;
    const int farm = type == BUILDING_TYPE_FARM;
    const int repairMaterial = type == BUILDING_TYPE_REPAIR_OFFICE;
    // Fuel compatibility is descriptor-driven, not type-list-driven.  End
    // stations, passenger heliports, airport terminals and Workshop service
    // buildings can all legitimately own $STORAGE_FUEL without belonging to one
    // of the validated office/service runtime classes.
    const int fuelService = template_has_native_fuel_store(templ);
    if (!recipeFactory && !inputOnlyRecipe && !mine && !shopPub && !farm && !repairMaterial && !fuelService) return 0;

    u8 modes = 0;
    u64 sb = 0;
    usize sc = 0;
    const int haveStoreBlueprints = template_store_vector(templ, &sb, &sc);
    for (usize i = 0; haveStoreBlueprints && i < sc; ++i) {
        u8* store = (u8*)(sb + i * STORE_STRIDE);
        if (read_i32(store + STORE_TRANSPORT) != transport) continue;
        float cap = read_f32(store + STORE_CAPACITY);
        if (!finite_reasonable(cap) || (cap <= 0.000001f && transport != TRANSPORT_WASTE)) continue;
        const int explicitInput = store_direction_matches(store, ANNEX_INPUT);
        const int explicitOutput = store_direction_matches(store, ANNEX_OUTPUT);
        // Template descriptors use the same 0xE0 layout as live stores. Prefer
        // exact descriptor/resource-to-recipe matching; transport-only matching
        // is a fallback for rare blueprints whose resource vector is deferred.
        int recipeInput = store_matches_recipe(store, templ, TEMPLATE_CONSUME_BEGIN, TEMPLATE_CONSUME_END);
        int recipeOutput = store_matches_recipe(store, templ, TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END);
        if (!recipeInput) recipeInput = template_store_has_recipe_resource(store, templ, ANNEX_INPUT);
        if (!recipeOutput) recipeOutput = template_store_has_recipe_resource(store, templ, ANNEX_OUTPUT);

        if (repairMaterial && (transport == TRANSPORT_COVERED || transport == TRANSPORT_OPEN)) {
            if (explicitInput || recipeInput) modes |= LANE_MASK_INPUT;
            continue;
        }
        if (fuelService && transport == TRANSPORT_OIL) {
            modes |= LANE_MASK_INPUT;
            continue;
        }
        if (shopPub) {
            modes |= LANE_MASK_INPUT;
            continue;
        }
        if (mine) {
            if (recipeOutput) modes |= LANE_MASK_OUTPUT;
            if (transport == TRANSPORT_WASTE && explicitOutput) modes |= LANE_MASK_OUTPUT;
            continue;
        }
        if (inputOnlyRecipe) {
            if (recipeInput) modes |= LANE_MASK_INPUT;
            if (transport == TRANSPORT_WASTE && explicitOutput) modes |= LANE_MASK_OUTPUT;
            continue;
        }
        if (farm) {
            if (recipeOutput) modes |= LANE_MASK_OUTPUT;
            if (explicitInput) modes |= LANE_MASK_INPUT;
            if (transport == TRANSPORT_WASTE && explicitOutput) modes |= LANE_MASK_OUTPUT;
            continue;
        }
        if (recipeFactory) {
            if (recipeInput && !recipeOutput) modes |= LANE_MASK_INPUT;
            if (recipeOutput && !recipeInput) modes |= LANE_MASK_OUTPUT;
            // Vanilla $STORAGE_IMPORT/$STORAGE_EXPORT is itself an authoritative
            // direction contract. Use it as a preview fallback when a template's
            // resource records are deferred or a Workshop recipe vector cannot be
            // matched yet. This is especially important for aggregate processors.
            if (!recipeInput && !recipeOutput) {
                if (explicitInput) modes |= LANE_MASK_INPUT;
                if (explicitOutput) modes |= LANE_MASK_OUTPUT;
            }
            if (transport == TRANSPORT_WASTE && explicitOutput) modes |= LANE_MASK_OUTPUT;
        }
    }

    // Some production templates expose their consume/produce recipe before a
    // matching +0x778 storage blueprint is fully populated. Placement feedback
    // must still recognise those buildings: the recipe/resource coefficient is
    // authoritative for the transport family, while completed mechanics remain
    // gated by the real live descriptor.
    if (recipeFactory) {
        if (template_has_recipe_transport(templ, TEMPLATE_CONSUME_BEGIN, TEMPLATE_CONSUME_END, transport))
            modes |= LANE_MASK_INPUT;
        if (template_has_recipe_transport(templ, TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END, transport))
            modes |= LANE_MASK_OUTPUT;
    } else if (inputOnlyRecipe) {
        if (template_has_recipe_transport(templ, TEMPLATE_CONSUME_BEGIN, TEMPLATE_CONSUME_END, transport))
            modes |= LANE_MASK_INPUT;
    } else if (mine) {
        if (template_has_recipe_transport(templ, TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END, transport))
            modes |= LANE_MASK_OUTPUT;
    } else if (farm) {
        if (template_has_recipe_transport(templ, TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END, transport))
            modes |= LANE_MASK_OUTPUT;
    }
    return modes;
}

static int current_tool_host_transport_masks(void* game, u32* outInputMask, u32* outOutputMask) {
    if (outInputMask) *outInputMask = 0;
    if (outOutputMask) *outOutputMask = 0;
    u8* templ = current_tool_template(game);
    i32 type = -1;
    if (!templ || !template_building_type(templ, &type)) return 0;
    if (type == BUILDING_TYPE_STORAGE || type == BUILDING_TYPE_CARGO_STATION ||
        type == BUILDING_TYPE_TRASH_CONTAINER || type == BUILDING_TYPE_FIELD ||
        type == BUILDING_TYPE_WATER_PUMP) return 0;

    const i32 transports[] = {TRANSPORT_COVERED, TRANSPORT_OPEN, TRANSPORT_AGGREGATE,
                              TRANSPORT_OIL, TRANSPORT_DRY_BULK, TRANSPORT_COOLER,
                              TRANSPORT_LIVESTOCK, TRANSPORT_WATER, TRANSPORT_WASTE};
    u32 inMask = 0, outMask = 0;
    for (usize i = 0; i < sizeof(transports)/sizeof(transports[0]); ++i) {
        i32 t = transports[i];
        u8 modes = template_host_mode_mask_for_transport(templ, t);
        if (modes & LANE_MASK_INPUT) inMask |= transport_bit(t);
        if (modes & LANE_MASK_OUTPUT) outMask |= transport_bit(t);
    }
    if (outInputMask) *outInputMask = inMask;
    if (outOutputMask) *outOutputMask = outMask;
    return (inMask | outMask) != 0;
}

static int current_tool_storage_transport_mask(void* game, u32* outMask) {
    if (outMask) *outMask = 0;
    if (!game) return 0;
    u8* templ = current_tool_template(game);
    i32 type = -1;
    if (!templ || !template_building_type(templ, &type)) return 0;

    // Completed live buildings are the authority for actual storage descriptors.
    // Placement ghosts do not expose that vector reliably, so an unseen native
    // storage template is provisionally previewed. Once observed in-world, its
    // real transport mask becomes the preview filter. Final annex creation always
    // validates the completed building's live 0xE0 descriptor and can never be
    // created from this provisional classification alone.
    if (type == BUILDING_TYPE_STORAGE) {
        u32 mask = template_storage_transport_mask(templ);
        if (!mask) mask = observed_storage_template_mask(templ);
        if (!mask && !storage_template_observed(templ)) {
            mask = transport_bit(TRANSPORT_COVERED) | transport_bit(TRANSPORT_OPEN) |
                   transport_bit(TRANSPORT_AGGREGATE) | transport_bit(TRANSPORT_OIL) |
                   transport_bit(TRANSPORT_DRY_BULK) | transport_bit(TRANSPORT_COOLER) |
                   transport_bit(TRANSPORT_LIVESTOCK) | transport_bit(TRANSPORT_WATER);
        }
        if (outMask) *outMask = mask;
        return mask != 0;
    }

    if (type == BUILDING_TYPE_CARGO_STATION) {
        u32 mask = cargo_template_transport_mask(game, templ);
        if (outMask) *outMask = mask;
        return mask != 0;
    }

    // Native water reservoirs share $TYPE_WATER_PUMP with powered pumps and
    // switches. Engine speed zero is the stable template-side discriminator;
    // the completed building must additionally expose a >=10-capacity water
    // descriptor before it is accepted as an annex.
    if (type == BUILDING_TYPE_WATER_PUMP && template_engine_idle(templ)) {
        u32 mask = template_storage_transport_mask(templ) & transport_bit(TRANSPORT_WATER);
        if (!mask) mask = transport_bit(TRANSPORT_WATER);
        if (outMask) *outMask = mask;
        return mask != 0;
    }
    if (type == BUILDING_TYPE_TRASH_CONTAINER) {
        u32 mask = template_storage_transport_mask(templ) & transport_bit(TRANSPORT_WASTE);
        if (!mask) mask = transport_bit(TRANSPORT_WASTE);
        if (outMask) *outMask = mask;
        return mask != 0;
    }
    return 0;
}

static u8 current_tool_lane_mode_mask(void* game, i32 transport) {
    if (!game || !readable((u8*)game + GAME_CURRENT_TOOL, sizeof(u64))) return LANE_MASK_BOTH;
    u8* tool = (u8*)read_u64((u8*)game + GAME_CURRENT_TOOL);
    if (!tool || !readable(tool + TOOL_TEMPLATE, sizeof(u64))) return LANE_MASK_BOTH;
    u8* templ = (u8*)read_u64(tool + TOOL_TEMPLATE);
    i32 type = -1;
    if (!templ || !template_building_type(templ, &type)) return LANE_MASK_BOTH;
    return type == BUILDING_TYPE_CARGO_STATION
        ? cargo_template_mode_mask(game, templ, transport) : LANE_MASK_BOTH;
}

static int current_tool_is_building_type(void* game, i32 wantedType) {
    if (!game || !readable((u8*)game + GAME_CURRENT_TOOL, sizeof(u64))) return 0;
    u8* tool = (u8*)read_u64((u8*)game + GAME_CURRENT_TOOL);
    if (!tool || !readable(tool + TOOL_TEMPLATE, sizeof(u64))) return 0;
    u8* templ = (u8*)read_u64(tool + TOOL_TEMPLATE);
    i32 type = -1;
    return templ && template_building_type(templ, &type) && type == wantedType;
}

static int building_collision_identity(u8* building, u32* outType, u32* outId) {
    if (!building || !outType || !outId ||
        !readable(building + BUILDING_INSTANCE_ID, sizeof(i32))) return 0;
    u8* templ = building_template(building);
    if (!templ || !readable(templ + TEMPLATE_COLLISION_TYPE, sizeof(i32))) return 0;
    i32 type = read_i32(templ + TEMPLATE_COLLISION_TYPE);
    i32 id = read_i32(building + BUILDING_INSTANCE_ID);
    if (type < 0 || id < 0) return 0;
    *outType = (u32)type;
    *outId = (u32)id;
    return 1;
}

static void* c3d_building_manager(void* game) {
    if (!g_exeBase) return nullptr;
    u8* root = nullptr;
    u8* slot = g_exeBase + RVA_GAME_ROOT_SLOT;
    if (readable(slot, sizeof(u64))) root = (u8*)read_u64(slot);
    if ((!root || !readable(root + GAME_C3D_BUILDING_MANAGER, sizeof(u64))) && game) {
        root = (u8*)game;
    }
    if (!root || !readable(root + GAME_C3D_BUILDING_MANAGER, sizeof(u64))) return nullptr;
    void* manager = (void*)read_u64(root + GAME_C3D_BUILDING_MANAGER);
    return manager && readable(manager, 1) ? manager : nullptr;
}

static u8* c3d_type_for_template(void* game, u8* templ) {
    if (!templ || !readable(templ + TEMPLATE_COLLISION_TYPE, sizeof(i32))) return nullptr;
    i32 typeIndex = read_i32(templ + TEMPLATE_COLLISION_TYPE);
    if (typeIndex < 0) return nullptr;
    u8* manager = (u8*)c3d_building_manager(game);
    if (!manager || !readable(manager + C3D_TYPES_END, sizeof(u64))) return nullptr;
    u64 begin = read_u64(manager + C3D_TYPES_BEGIN);
    u64 end = read_u64(manager + C3D_TYPES_END);
    usize count = 0;
    if (!valid_vector(begin, end, C3D_TYPE_STRIDE, 4096, &count) || (usize)typeIndex >= count) return nullptr;
    u8* type = (u8*)(begin + (usize)typeIndex * C3D_TYPE_STRIDE);
    return readable(type, C3D_TYPE_NODES_END + sizeof(u64)) ? type : nullptr;
}

static u32 c3d_connection_kind_flags(void* game, u8* templ) {
    u8* type = c3d_type_for_template(game, templ);
    if (!type) return 0;
    u64 begin = read_u64(type + C3D_TYPE_NODES_BEGIN);
    u64 end = read_u64(type + C3D_TYPE_NODES_END);
    usize count = 0;
    if (!valid_vector(begin, end, sizeof(u64), 1024, &count)) return 0;
    u32 flags = 0;
    for (usize i = 0; i < count; ++i) {
        u8* node = (u8*)read_u64((u8*)begin + i * sizeof(u64));
        if (!node || !readable(node + C3D_NODE_CONNECTION_META, sizeof(u64))) continue;
        u8* meta = (u8*)read_u64(node + C3D_NODE_CONNECTION_META);
        if (!meta || !readable(meta + C3D_CONNECTION_KIND, sizeof(i32))) continue;
        i32 kind = read_i32(meta + C3D_CONNECTION_KIND);
        if (kind >= 0 && kind < 32) flags |= (1u << (u32)kind);
    }
    return flags;
}

static u32 cargo_template_transport_mask(void* game, u8* templ) {
    if (!templ) return 0;
    // 1.1.1.13 function RVA 0x0DEB70 confirms template+0x778/+0x780 as the storage-blueprint vector.
    // Prefer it over mutable live routing state so specialised aggregate/pipe
    // stations remain classifiable even when their runtime routing store is empty.
    u32 blueprint = template_storage_transport_mask(templ);
    if (blueprint) return blueprint;
    u32 observed = observed_storage_template_mask(templ);
    if (storage_template_observed(templ) && observed) return observed;

    u32 flags = c3d_connection_kind_flags(game, templ);
    u32 mask = 0;
    if (flags & ((1u << 3) | (1u << 4))) mask |= transport_bit(TRANSPORT_AGGREGATE);
    if (flags & ((1u << 5) | (1u << 6))) mask |= transport_bit(TRANSPORT_OIL);
    if (flags & ((1u << 0x13) | (1u << 0x14))) mask |= transport_bit(TRANSPORT_DRY_BULK);
    if (flags & ((1u << 0x19) | (1u << 0x1A))) mask |= transport_bit(TRANSPORT_WATER);

    // Ordinary road/rail cargo stations do not need factory-resource nodes;
    // their covered/open routing buffers are bidirectional. If no specialised
    // resource-network signature exists, preview them provisionally as general
    // cargo. Completed live descriptors remain authoritative after placement.
    if (!mask) mask = transport_bit(TRANSPORT_COVERED) | transport_bit(TRANSPORT_OPEN) |
                      transport_bit(TRANSPORT_DRY_BULK) | transport_bit(TRANSPORT_COOLER) |
                      transport_bit(TRANSPORT_LIVESTOCK);
    return mask;
}

static u8 cargo_template_mode_mask(void* game, u8* templ, i32 transport) {
    if (transport == TRANSPORT_COVERED || transport == TRANSPORT_OPEN ||
        transport == TRANSPORT_COOLER || transport == TRANSPORT_LIVESTOCK) return LANE_MASK_BOTH;
    u32 flags = c3d_connection_kind_flags(game, templ);
    u8 modes = 0;
    // A station INPUT connection takes material FROM a root output (loader).
    // A station OUTPUT connection delivers material TO a root input (unloader).
    if (transport == TRANSPORT_AGGREGATE) {
        if (flags & (1u << 3)) modes |= LANE_MASK_OUTPUT;
        if (flags & (1u << 4)) modes |= LANE_MASK_INPUT;
    } else if (transport == TRANSPORT_OIL) {
        if (flags & (1u << 5)) modes |= LANE_MASK_OUTPUT;
        if (flags & (1u << 6)) modes |= LANE_MASK_INPUT;
    } else if (transport == TRANSPORT_DRY_BULK) {
        if (flags & (1u << 0x13)) modes |= LANE_MASK_OUTPUT;
        if (flags & (1u << 0x14)) modes |= LANE_MASK_INPUT;
    } else if (transport == TRANSPORT_WATER) {
        if (flags & (1u << 0x19)) modes |= LANE_MASK_OUTPUT;
        if (flags & (1u << 0x1A)) modes |= LANE_MASK_INPUT;
    }
    return modes ? modes : LANE_MASK_BOTH;
}

static u32 building_annex_transport_mask(void* game, u8* building) {
    if (!building) return 0;
    u8* templ = building_template(building);
    i32 type = -1;
    if (!templ || !template_building_type(templ, &type)) return 0;
    if (type == BUILDING_TYPE_STORAGE) {
        u32 mask = template_storage_transport_mask(templ);
        if (!mask) mask = observed_storage_template_mask(templ);
        if (!mask) {
            u64 sb = 0; usize sc = 0;
            if (building_store_vector(building, &sb, &sc)) {
                for (usize i = 0; i < sc; ++i) {
                    u8* store = (u8*)(sb + i * STORE_STRIDE);
                    if (!annex_building_store_eligible(building, store)) continue;
                    i32 transport = read_i32(store + STORE_TRANSPORT);
                    if (supported_annex_transport(transport)) mask |= transport_bit(transport);
                }
            }
        }
        return mask;
    }
    if (type == BUILDING_TYPE_CARGO_STATION) return cargo_template_transport_mask(game, templ);
    if (type == BUILDING_TYPE_WATER_PUMP && template_engine_idle(templ))
        return template_storage_transport_mask(templ) & transport_bit(TRANSPORT_WATER);
    if (type == BUILDING_TYPE_TRASH_CONTAINER)
        return transport_bit(TRANSPORT_WASTE);
    return 0;
}

static u8 building_annex_mode_mask(void* game, u8* building, i32 transport) {
    if (!building) return 0;
    u8* templ = building_template(building);
    i32 type = -1;
    if (!templ || !template_building_type(templ, &type)) return 0;
    if (type == BUILDING_TYPE_CARGO_STATION) return cargo_template_mode_mask(game, templ, transport);
    if (type == BUILDING_TYPE_TRASH_CONTAINER) return LANE_MASK_OUTPUT;
    if (type == BUILDING_TYPE_STORAGE ||
        (type == BUILDING_TYPE_WATER_PUMP && template_engine_idle(templ))) return LANE_MASK_BOTH;
    return 0;
}

static int point_hits_building(void* game, u8* target, const C3DVector3* point, u8* ignore) {
    if (!game || !target || !point || !g_collision2DBBox) return 0;
    void* manager = c3d_building_manager(game);
    if (!manager) return 0;
    u32 targetType = 0, targetId = 0;
    if (!building_collision_identity(target, &targetType, &targetId)) return 0;
    u32 ignoreType = 0xFFFFFFFFu, ignoreId = 0xFFFFFFFFu;
    if (ignore) building_collision_identity(ignore, &ignoreType, &ignoreId);
    u32 hitType = 0xFFFFFFFFu, hitId = 0xFFFFFFFFu;
    C3DVector3 copy = *point;
    bool hit = g_collision2DBBox(manager, &copy, &hitType, &hitId,
                                 ignoreType, ignoreId, 0, 0.0f);
    return hit && hitType == targetType && hitId == targetId;
}

static int building_center(u8* building, C3DVector3* out) {
    if (!building || !out || !readable(building + BUILDING_INSTANCE_ID, sizeof(i32))) return 0;
    u8* templ = building_template(building);
    u8* type = templ ? c3d_type_for_template(nullptr, templ) : nullptr;
    if (!type || !readable(type + C3D_TYPE_INSTANCES_END, sizeof(u64))) return 0;

    i32 instanceId = read_i32(building + BUILDING_INSTANCE_ID);
    if (instanceId < 0) return 0;
    u64 begin = read_u64(type + C3D_TYPE_INSTANCES_BEGIN);
    u64 end = read_u64(type + C3D_TYPE_INSTANCES_END);
    usize count = 0;
    if (!valid_vector(begin, end, C3D_INSTANCE_STRIDE, 100000, &count) ||
        (usize)instanceId >= count) return 0;

    u8* instance = (u8*)(begin + (usize)instanceId * C3D_INSTANCE_STRIDE);
    if (!readable(instance + C3D_INSTANCE_POS_Z, sizeof(float))) return 0;
    out->x = read_f32(instance + C3D_INSTANCE_POS_X);
    out->y = read_f32(instance + C3D_INSTANCE_POS_Y);
    out->z = read_f32(instance + C3D_INSTANCE_POS_Z);
    return finite_reasonable(out->x) && finite_reasonable(out->y) && finite_reasonable(out->z);
}

static C3DVector3 lerp_point(const C3DVector3& a, const C3DVector3& b, float t) {
    C3DVector3 p;
    p.x = a.x + (b.x - a.x) * t;
    p.y = a.y + (b.y - a.y) * t;
    p.z = a.z + (b.z - a.z) * t;
    return p;
}

static float point_distance_sq_2d(const C3DVector3& a, const C3DVector3& b) {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return dx * dx + dz * dz;
}

// Find the final point still inside target while travelling from a known
// inside point towards an outside point. Collision2DBBox is an oriented 2D
// bounding-box test, so this segment transition is monotonic for a building.
static int boundary_from_inside(void* game, u8* target, u8* ignore,
                                const C3DVector3& insidePoint,
                                const C3DVector3& outsidePoint,
                                C3DVector3* outBoundary) {
    if (!outBoundary) return 0;
    if (!point_hits_building(game, target, &insidePoint, ignore)) {
        if (!g_footprintProbeFailureLogged && g_host && g_host->log) {
            g_host->log("[EnhancedStorageManagement] native footprint probe could not resolve building %p at its own centre; link discovery skipped for that candidate", target);
            g_footprintProbeFailureLogged = 1;
        }
        return 0;
    }
    if (point_hits_building(game, target, &outsidePoint, ignore)) {
        *outBoundary = outsidePoint;
        return 2; // segment endpoint is still inside: overlap/zero-gap case
    }
    float insideT = 0.0f;
    float outsideT = 1.0f;
    for (int i = 0; i < 12; ++i) {
        float mid = (insideT + outsideT) * 0.5f;
        C3DVector3 p = lerp_point(insidePoint, outsidePoint, mid);
        if (point_hits_building(game, target, &p, ignore)) insideT = mid;
        else outsideT = mid;
    }
    *outBoundary = lerp_point(insidePoint, outsidePoint, insideT);
    return 1;
}

static int pair_footprint_gap_sq_collision_ray_fallback(void* game, u8* host, u8* storage, float* outSq) {
    if (!game || !host || !storage || !outSq) return 0;
    float centreSq = 0.0f;
    if (!position_distance_sq(host, storage, &centreSq) || centreSq > g_coarseCandidateRadiusSq) return 0;
    C3DVector3 hostCenter = {}, storageCenter = {};
    if (!building_center(host, &hostCenter) || !building_center(storage, &storageCenter)) return 0;
    if (abs_f(hostCenter.y - storageCenter.y) > g_verticalLimit) return 0;

    C3DVector3 hostEdge = {}, storageEdge = {};
    int hostResult = boundary_from_inside(game, host, storage, hostCenter, storageCenter, &hostEdge);
    int storageResult = boundary_from_inside(game, storage, host, storageCenter, hostCenter, &storageEdge);
    if (!hostResult || !storageResult) return 0;
    if (hostResult == 2 || storageResult == 2) {
        *outSq = 0.0f;
        return 1;
    }
    float gapSq = point_distance_sq_2d(hostEdge, storageEdge);
    if (!finite_reasonable(gapSq) || gapSq < 0.0f) return 0;
    *outSq = gapSq;
    return 1;
}

static int load_local_box_from_mesh(u8* mesh, int flipZ, LocalBox2D* out) {
    if (!mesh || !out || !readable(mesh + C3D_MESH_BBOX_MAX_Z, sizeof(float))) return 0;
    float minX = read_f32(mesh + C3D_MESH_BBOX_MIN_X);
    float minZ = read_f32(mesh + C3D_MESH_BBOX_MIN_Z);
    float maxX = read_f32(mesh + C3D_MESH_BBOX_MAX_X);
    float maxZ = read_f32(mesh + C3D_MESH_BBOX_MAX_Z);
    if (!finite_reasonable(minX) || !finite_reasonable(minZ) ||
        !finite_reasonable(maxX) || !finite_reasonable(maxZ)) return 0;
    if (flipZ) {
        float oldMinZ = minZ;
        minZ = -maxZ;
        maxZ = -oldMinZ;
    }
    if (!(minX < maxX) || !(minZ < maxZ)) return 0;
    out->minX = minX;
    out->minZ = minZ;
    out->maxX = maxX;
    out->maxZ = maxZ;
    out->valid = 1;
    return 1;
}

// Resolve the exact prospective native storage footprint used by WRSR's C3D
// cursor-building path.  EditorSetCursorUpdateCopy and Collision2DBBox in
// The supplied C3DDLL64 1.1.1.13 Collision2DBBox uses the same TYPE geometry
// and rotation-step matrices.
static int current_tool_preview_footprint(void* game, PreviewFootprint* out) {
    if (!game || !out || !g_getRotationStep || !g_vec3TransformCoord) return 0;
    u8* templ = current_tool_template(game);
    if (!templ || !readable(templ + TEMPLATE_COLLISION_TYPE, sizeof(i32))) return 0;
    i32 typeIndex = read_i32(templ + TEMPLATE_COLLISION_TYPE);
    if (typeIndex < 0) return 0;

    u8* manager = (u8*)c3d_building_manager(game);
    if (!manager || !readable(manager + C3D_TYPES_END, sizeof(u64))) return 0;
    u64 typesBegin = read_u64(manager + C3D_TYPES_BEGIN);
    u64 typesEnd = read_u64(manager + C3D_TYPES_END);
    usize typeCount = 0;
    if (!valid_vector(typesBegin, typesEnd, C3D_TYPE_STRIDE, 4096, &typeCount) ||
        (usize)typeIndex >= typeCount) return 0;
    u8* type = (u8*)(typesBegin + (usize)typeIndex * C3D_TYPE_STRIDE);
    if (!readable(type, C3D_TYPE_FLIP_Z + 1)) return 0;

    u8* primaryMesh = (u8*)read_u64(type + C3D_TYPE_PRIMARY_MESH);
    u8* secondaryMesh = (u8*)read_u64(type + C3D_TYPE_SECONDARY_MESH);
    int flipZ = read_u8(type + C3D_TYPE_FLIP_Z) != 0;

    PreviewFootprint pf = {};
    const int havePrimary = load_local_box_from_mesh(primaryMesh, flipZ, &pf.primary);
    const int haveSecondary = secondaryMesh ? load_local_box_from_mesh(secondaryMesh, flipZ, &pf.secondary) : 0;
    // Some station/construction templates expose only a secondary collision
    // mesh. Either authored footprint is enough; refusing secondary-only types
    // would otherwise make the configured edge-gap result rotation-dependent.
    if (!havePrimary && !haveSecondary) return 0;

    u8* world = (u8*)game;
    if (!readable(world + GAME_CURSOR_X, 12) || !readable(world + GAME_CURSOR_ROTATION, sizeof(float)) ||
        !readable(manager + C3D_ROTATION_STEP_COUNT, sizeof(i32))) return 0;
    pf.origin.x = read_f32(world + GAME_CURSOR_X);
    pf.origin.y = read_f32(world + GAME_CURSOR_Y);
    pf.origin.z = read_f32(world + GAME_CURSOR_Z);
    float rotation = read_f32(world + GAME_CURSOR_ROTATION);
    i32 stepCount = read_i32(manager + C3D_ROTATION_STEP_COUNT);
    if (!finite_reasonable(pf.origin.x) || !finite_reasonable(pf.origin.y) ||
        !finite_reasonable(pf.origin.z) || !finite_reasonable(rotation) ||
        stepCount <= 0 || stepCount > 4096) return 0;

    int step = g_getRotationStep(manager, rotation);
    if (step < 0 || step >= stepCount) return 0;
    const void* fwd = manager + C3D_ROT_MATRIX_FWD_BASE + (usize)step * C3D_ROT_MATRIX_STRIDE;
    const void* inv = manager + C3D_ROT_MATRIX_INV_BASE + (usize)step * C3D_ROT_MATRIX_STRIDE;
    if (!readable(fwd, 64) || !readable(inv, 64)) return 0;
    pf.forwardRotation = fwd;
    pf.inverseRotation = inv;


    *out = pf;
    return 1;
}

// Resolve the same oriented C3D footprint for an already-built WRSR building.
// Collision2DBBox indexes TYPE instances by (collision type, building instance id):
// TYPE+0x2C0 is a vector of 0x118-byte live instances; +0x10..+0x18 is
// position and +0x1C is the discrete rotation step. Reusing those exact fields
// avoids treating the WRSR gameplay object's logical centre as the footprint.
static int completed_building_footprint(void* game, u8* building, PreviewFootprint* out) {
    if (!game || !building || !out || !g_vec3TransformCoord) return 0;
    u32 typeIndex = 0, instanceId = 0;
    if (!building_collision_identity(building, &typeIndex, &instanceId)) return 0;
    u8* manager = (u8*)c3d_building_manager(game);
    if (!manager || !readable(manager + C3D_TYPES_END, sizeof(u64)) ||
        !readable(manager + C3D_ROTATION_STEP_COUNT, sizeof(i32))) return 0;

    u64 typesBegin = read_u64(manager + C3D_TYPES_BEGIN);
    u64 typesEnd = read_u64(manager + C3D_TYPES_END);
    usize typeCount = 0;
    if (!valid_vector(typesBegin, typesEnd, C3D_TYPE_STRIDE, 4096, &typeCount) ||
        (usize)typeIndex >= typeCount) return 0;
    u8* type = (u8*)(typesBegin + (usize)typeIndex * C3D_TYPE_STRIDE);
    if (!readable(type, C3D_TYPE_FLIP_Z + 1) ||
        !readable(type + C3D_TYPE_INSTANCES_END, sizeof(u64))) return 0;

    u64 instancesBegin = read_u64(type + C3D_TYPE_INSTANCES_BEGIN);
    u64 instancesEnd = read_u64(type + C3D_TYPE_INSTANCES_END);
    usize instanceCount = 0;
    if (!valid_vector(instancesBegin, instancesEnd, C3D_INSTANCE_STRIDE, 200000, &instanceCount) ||
        (usize)instanceId >= instanceCount) return 0;
    u8* instance = (u8*)(instancesBegin + (usize)instanceId * C3D_INSTANCE_STRIDE);
    if (!readable(instance + C3D_INSTANCE_ROT_STEP, 1)) return 0;

    PreviewFootprint pf = {};
    u8* primaryMesh = (u8*)read_u64(type + C3D_TYPE_PRIMARY_MESH);
    u8* secondaryMesh = (u8*)read_u64(type + C3D_TYPE_SECONDARY_MESH);
    int flipZ = read_u8(type + C3D_TYPE_FLIP_Z) != 0;
    const int havePrimary = load_local_box_from_mesh(primaryMesh, flipZ, &pf.primary);
    const int haveSecondary = secondaryMesh ? load_local_box_from_mesh(secondaryMesh, flipZ, &pf.secondary) : 0;
    if (!havePrimary && !haveSecondary) return 0;

    pf.origin.x = read_f32(instance + C3D_INSTANCE_POS_X);
    pf.origin.y = read_f32(instance + C3D_INSTANCE_POS_Y);
    pf.origin.z = read_f32(instance + C3D_INSTANCE_POS_Z);
    i32 stepCount = read_i32(manager + C3D_ROTATION_STEP_COUNT);
    u32 step = (u32)read_u8(instance + C3D_INSTANCE_ROT_STEP);
    if (!finite_reasonable(pf.origin.x) || !finite_reasonable(pf.origin.y) ||
        !finite_reasonable(pf.origin.z) || stepCount <= 0 || stepCount > 4096 ||
        step >= (u32)stepCount) return 0;

    const void* fwd = manager + C3D_ROT_MATRIX_FWD_BASE + (usize)step * C3D_ROT_MATRIX_STRIDE;
    const void* inv = manager + C3D_ROT_MATRIX_INV_BASE + (usize)step * C3D_ROT_MATRIX_STRIDE;
    if (!readable(fwd, 64) || !readable(inv, 64)) return 0;
    pf.forwardRotation = fwd;
    pf.inverseRotation = inv;
    *out = pf;
    return 1;
}

static int local_box_world_quad(const PreviewFootprint& pf, const LocalBox2D& box,
                                Point2D outQuad[4]) {
    if (!box.valid || !pf.forwardRotation || !g_vec3TransformCoord || !outQuad) return 0;
    const float xs[4] = {box.minX, box.maxX, box.maxX, box.minX};
    const float zs[4] = {box.minZ, box.minZ, box.maxZ, box.maxZ};
    for (int i = 0; i < 4; ++i) {
        C3DVector3 local = {xs[i], 0.0f, zs[i]};
        C3DVector3 rotated = {};
        if (!g_vec3TransformCoord(&rotated, &local, pf.forwardRotation)) return 0;
        float wx = rotated.x + pf.origin.x;
        float wz = rotated.z + pf.origin.z;
        if (!finite_reasonable(wx) || !finite_reasonable(wz)) return 0;
        outQuad[i].x = wx;
        outQuad[i].z = wz;
    }
    return 1;
}

static float point_segment_distance_sq_2d(const Point2D& p, const Point2D& a, const Point2D& b) {
    float vx = b.x - a.x;
    float vz = b.z - a.z;
    float lenSq = vx * vx + vz * vz;
    if (lenSq <= 0.0000001f) {
        float dx = p.x - a.x;
        float dz = p.z - a.z;
        return dx * dx + dz * dz;
    }
    float t = ((p.x - a.x) * vx + (p.z - a.z) * vz) / lenSq;
    t = clamp_f(t, 0.0f, 1.0f);
    float qx = a.x + vx * t;
    float qz = a.z + vz * t;
    float dx = p.x - qx;
    float dz = p.z - qz;
    return dx * dx + dz * dz;
}

static int quads_overlap_sat(const Point2D a[4], const Point2D b[4]) {
    const Point2D* polys[2] = {a, b};
    for (int poly = 0; poly < 2; ++poly) {
        const Point2D* q = polys[poly];
        for (int edge = 0; edge < 4; ++edge) {
            const Point2D& p0 = q[edge];
            const Point2D& p1 = q[(edge + 1) & 3];
            float ex = p1.x - p0.x;
            float ez = p1.z - p0.z;
            float ax = -ez;
            float az = ex;
            float axisLenSq = ax * ax + az * az;
            if (axisLenSq <= 0.0000001f) continue;
            float amin = a[0].x * ax + a[0].z * az;
            float amax = amin;
            float bmin = b[0].x * ax + b[0].z * az;
            float bmax = bmin;
            for (int i = 1; i < 4; ++i) {
                float ap = a[i].x * ax + a[i].z * az;
                float bp = b[i].x * ax + b[i].z * az;
                if (ap < amin) amin = ap;
                if (ap > amax) amax = ap;
                if (bp < bmin) bmin = bp;
                if (bp > bmax) bmax = bp;
            }
            const float epsilon = 0.0001f;
            if (amax < bmin - epsilon || bmax < amin - epsilon) return 0;
        }
    }
    return 1;
}

static float quad_gap_sq(const Point2D a[4], const Point2D b[4]) {
    if (quads_overlap_sat(a, b)) return 0.0f;
    float best = 1.0e30f;
    for (int i = 0; i < 4; ++i) {
        for (int e = 0; e < 4; ++e) {
            float d1 = point_segment_distance_sq_2d(a[i], b[e], b[(e + 1) & 3]);
            float d2 = point_segment_distance_sq_2d(b[i], a[e], a[(e + 1) & 3]);
            if (d1 < best) best = d1;
            if (d2 < best) best = d2;
        }
    }
    return best;
}

static int oriented_footprint_gap_sq(const PreviewFootprint& a, const PreviewFootprint& b, float* outSq) {
    if (!outSq || !a.forwardRotation || !b.forwardRotation) return 0;
    const LocalBox2D* abox[2] = {&a.primary, &a.secondary};
    const LocalBox2D* bbox[2] = {&b.primary, &b.secondary};
    float best = 1.0e30f;
    int found = 0;
    for (int ai = 0; ai < 2; ++ai) {
        if (!abox[ai]->valid) continue;
        Point2D aq[4] = {};
        if (!local_box_world_quad(a, *abox[ai], aq)) continue;
        for (int bi = 0; bi < 2; ++bi) {
            if (!bbox[bi]->valid) continue;
            Point2D bq[4] = {};
            if (!local_box_world_quad(b, *bbox[bi], bq)) continue;
            float gapSq = quad_gap_sq(aq, bq);
            if (!finite_reasonable(gapSq) || gapSq < 0.0f) continue;
            if (gapSq < best) best = gapSq;
            found = 1;
        }
    }
    if (!found) return 0;
    *outSq = best;
    return 1;
}

static int point_in_local_box_2d(const LocalBox2D& box, float x, float z) {
    if (!box.valid) return 0;
    const float e = 0.01f;
    return x >= box.minX - e && x <= box.maxX + e &&
           z >= box.minZ - e && z <= box.maxZ + e;
}

static int point_hits_preview(const PreviewFootprint& pf, const C3DVector3& point) {
    if (!pf.inverseRotation || !g_vec3TransformCoord) return 0;
    C3DVector3 rel = {point.x - pf.origin.x, point.y - pf.origin.y, point.z - pf.origin.z};
    C3DVector3 local = {};
    if (!g_vec3TransformCoord(&local, &rel, pf.inverseRotation)) return 0;
    if (!finite_reasonable(local.x) || !finite_reasonable(local.z)) return 0;
    return point_in_local_box_2d(pf.primary, local.x, local.z) ||
           point_in_local_box_2d(pf.secondary, local.x, local.z);
}

static int preview_boundary_from_inside(const PreviewFootprint& pf,
                                        const C3DVector3& insidePoint,
                                        const C3DVector3& outsidePoint,
                                        C3DVector3* outBoundary) {
    if (!outBoundary || !point_hits_preview(pf, insidePoint)) return 0;
    if (point_hits_preview(pf, outsidePoint)) {
        *outBoundary = outsidePoint;
        return 2;
    }
    float insideT = 0.0f;
    float outsideT = 1.0f;
    for (int i = 0; i < 12; ++i) {
        float mid = (insideT + outsideT) * 0.5f;
        C3DVector3 p = lerp_point(insidePoint, outsidePoint, mid);
        if (point_hits_preview(pf, p)) insideT = mid;
        else outsideT = mid;
    }
    *outBoundary = lerp_point(insidePoint, outsidePoint, insideT);
    return 1;
}

static int preview_pair_footprint_gap_sq_collision_ray_fallback(void* game, const PreviewFootprint& preview,
                                                                u8* host, float* outSq) {
    if (!game || !host || !outSq) return 0;
    float centreSq = 0.0f;
    if (!cursor_distance_sq(game, host, &centreSq) || centreSq > g_coarseCandidateRadiusSq) return 0;
    C3DVector3 hostCenter = {};
    if (!building_center(host, &hostCenter)) return 0;
    if (abs_f(hostCenter.y - preview.origin.y) > g_verticalLimit) return 0;

    C3DVector3 hostEdge = {}, previewEdge = {};
    int hostResult = boundary_from_inside(game, host, nullptr, hostCenter, preview.origin, &hostEdge);
    int previewResult = preview_boundary_from_inside(preview, preview.origin, hostCenter, &previewEdge);
    if (!hostResult || !previewResult) return 0;
    if (hostResult == 2 || previewResult == 2) {
        *outSq = 0.0f;
        return 1;
    }
    float gapSq = point_distance_sq_2d(hostEdge, previewEdge);
    if (!finite_reasonable(gapSq) || gapSq < 0.0f) return 0;
    *outSq = gapSq;
    return 1;
}

// The link rule uses the minimum separation between every edge of the real rotated
// C3D collision footprints. A centre-to-centre collision ray is retained only as a
// conservative fallback if a live C3D instance cannot be reconstructed.
static int pair_footprint_gap_sq(void* game, u8* host, u8* storage, float* outSq) {
    if (!game || !host || !storage || !outSq) return 0;
    PreviewFootprint a = {}, b = {};
    if (completed_building_footprint(game, host, &a) &&
        completed_building_footprint(game, storage, &b) &&
        abs_f(a.origin.y - b.origin.y) <= g_verticalLimit &&
        oriented_footprint_gap_sq(a, b, outSq)) return 1;
    return pair_footprint_gap_sq_collision_ray_fallback(game, host, storage, outSq);
}

static int preview_pair_footprint_gap_sq(void* game, const PreviewFootprint& preview,
                                         u8* host, float* outSq) {
    if (!game || !host || !outSq) return 0;
    PreviewFootprint built = {};
    if (completed_building_footprint(game, host, &built) &&
        abs_f(built.origin.y - preview.origin.y) <= g_verticalLimit &&
        oriented_footprint_gap_sq(preview, built, outSq)) return 1;
    return preview_pair_footprint_gap_sq_collision_ray_fallback(game, preview, host, outSq);
}

// Cursor-origin test retained only as a rendering fail-safe if C3D's preview
// TYPE/matrix data cannot be read. Normal placement feedback uses the
// prospective storage-annex footprint above.
static int cursor_footprint_gap_sq_fallback(void* game, u8* host, float* outSq) {
    if (!game || !host || !outSq) return 0;
    float centreSq = 0.0f;
    if (!cursor_distance_sq(game, host, &centreSq) || centreSq > g_coarseCandidateRadiusSq) return 0;
    C3DVector3 hostCenter = {}, cursor = {};
    if (!building_center(host, &hostCenter) || !readable((u8*)game + GAME_CURSOR_X, 12)) return 0;
    cursor.x = read_f32((u8*)game + GAME_CURSOR_X);
    cursor.y = read_f32((u8*)game + GAME_CURSOR_Y);
    cursor.z = read_f32((u8*)game + GAME_CURSOR_Z);
    if (!finite_reasonable(cursor.x) || !finite_reasonable(cursor.y) || !finite_reasonable(cursor.z) ||
        abs_f(cursor.y - hostCenter.y) > g_verticalLimit) return 0;

    C3DVector3 edge = {};
    int result = boundary_from_inside(game, host, nullptr, hostCenter, cursor, &edge);
    if (!result) return 0;
    if (result == 2) {
        *outSq = 0.0f;
        return 1;
    }
    float gapSq = point_distance_sq_2d(edge, cursor);
    if (!finite_reasonable(gapSq) || gapSq < 0.0f) return 0;
    *outSq = gapSq;
    return 1;
}

static int current_preview_gap_to_building(void* game, int havePreview,
                                           const PreviewFootprint& preview,
                                           u8* building, float* outSq) {
    if (!game || !building || !outSq) return 0;
    return havePreview
        ? preview_pair_footprint_gap_sq(game, preview, building, outSq)
        : cursor_footprint_gap_sq_fallback(game, building, outSq);
}

// Cargo gateway preview is rendering-only and must fail closed. A prior crash
// occurred immediately after a valid cargo preview was decoded. Avoid the direct
// built-root C3D instance-vector reconstruction here and use WRSR's collision
// boundary query for the completed root instead.
static int current_cargo_preview_gap_to_building(void* game, int havePreview,
                                                 const PreviewFootprint& preview,
                                                 u8* building, float* outSq) {
    if (!game || !building || !outSq || !completed_building(building)) return 0;
    return havePreview
        ? preview_pair_footprint_gap_sq_collision_ray_fallback(game, preview, building, outSq)
        : cursor_footprint_gap_sq_fallback(game, building, outSq);
}

static u8* nearest_attachment_to_preview(void* game, u32 transportMask, u8** outRoot, float* outGapSq, int cargoSafe) {
    if (outRoot) *outRoot = nullptr;
    if (outGapSq) *outGapSq = 0.0f;
    PreviewFootprint preview = {};
    int havePreview = current_tool_preview_footprint(game, &preview);

    u8* bestSurface = nullptr;
    u8* bestRoot = nullptr;
    float bestSq = g_linkGapSq + 1.0f;

    // Current STORAGE or DELIVERY tool against completed RESOURCE roots. DELIVERY
    // tools may additionally target completed standalone STORAGE roots. Ordinary
    // storage tools deliberately skip those storage-root lanes so storage-storage
    // chaining can never be promised by the overlay.
    u8* previewStorageTemplate = cargoSafe ? nullptr : current_tool_template(game);
    for (u32 i = 0; i < g_currentHostLaneCount; ++i) {
        HostLane* lane = &g_hostLanes[i];
        if (!lane->building) continue;
        if ((transportMask & transport_bit(lane->transport)) == 0) continue;
        if (!cargoSafe && lane->rootClass == HOST_STORAGE_DELIVERY_ONLY) continue;
        if (lane->rootClass == HOST_RESOURCE_ROOT &&
            (current_tool_lane_mode_mask(game, lane->transport) & lane_mode_mask(lane->mode)) == 0) continue;
        if (!cargoSafe && lane->rootClass == HOST_RESOURCE_ROOT &&
            !template_store_matches_live_host_store(previewStorageTemplate, lane->hostStore, lane->transport)) continue;
        float sq = 0.0f;
        int haveGap = cargoSafe
            ? current_cargo_preview_gap_to_building(game, havePreview, preview, lane->building, &sq)
            : current_preview_gap_to_building(game, havePreview, preview, lane->building, &sq);
        if (haveGap && sq <= g_linkGapSq && sq < bestSq) {
            bestSurface = lane->building;
            if (lane->rootClass == HOST_STORAGE_DELIVERY_ONLY) {
                u8* pooledRoot = active_pool_root_for_member(lane->building);
                bestRoot = pooledRoot ? pooledRoot : lane->building;
            } else {
                bestRoot = lane->building;
            }
            bestSq = sq;
        }
    }

    // STORAGE tool against an already-built DELIVERY building. The completed
    // storage will become the delivery-only stock root unless it subsequently
    // joins a compatible production/resource root. This closes the reverse
    // construction order for storage<->delivery.
    if (!cargoSafe) {
        for (u32 i = 0; i < g_currentStorageCount; ++i) {
            StorageCandidate* candidate = &g_storages[i];
            if (!candidate->cargoGateway || !candidate->building || !completed_building(candidate->building)) continue;
            if ((transportMask & transport_bit(candidate->transport)) == 0) continue;
            float sq = 0.0f;
            if (!current_preview_gap_to_building(game, havePreview, preview, candidate->building, &sq) ||
                sq > g_linkGapSq || sq >= bestSq) continue;
            bestSurface = candidate->building;
            bestRoot = candidate->building;
            bestSq = sq;
        }
    }

    // Cargo-safe DELIVERY placement deliberately skips unfinished targets. Their
    // render/collision lifetime is less stable during construction-menu changes;
    // the requested UI contract concerns already placed buildings, and mechanics
    // activate once construction completes.
    if (cargoSafe) {
        if (bestRoot && outRoot) *outRoot = bestRoot;
        if (bestSurface && outGapSq) *outGapSq = bestSq;
        return bestSurface;
    }

    // STORAGE placement beside an unfinished RESOURCE building remains a visual
    // promise derived from the template recipe/descriptor. No state is created
    // until both buildings are complete.
    u64 wb = 0;
    usize wc = 0;
    if (world_building_vector(game, &wb, &wc)) {
        for (usize i = 0; i < wc; ++i) {
            u8* building = (u8*)read_u64((u8*)wb + i * sizeof(u64));
            if (!building || completed_building(building)) continue;
            u8* templ = building_template(building);
            if (!templ) continue;
            int compatible = 0;
            u8* storageTempl = current_tool_template(game);
            u64 storageBegin = 0;
            usize storageCount = 0;
            if (!storageTempl || !template_store_vector(storageTempl, &storageBegin, &storageCount)) continue;
            const i32 transports[] = {TRANSPORT_COVERED, TRANSPORT_OPEN, TRANSPORT_AGGREGATE,
                                      TRANSPORT_OIL, TRANSPORT_DRY_BULK, TRANSPORT_COOLER,
                                      TRANSPORT_LIVESTOCK, TRANSPORT_WATER, TRANSPORT_WASTE};
            for (usize t = 0; t < sizeof(transports)/sizeof(transports[0]) && !compatible; ++t) {
                i32 transport = transports[t];
                if ((transportMask & transport_bit(transport)) == 0) continue;
                u8 hostModes = template_host_mode_mask_for_transport(templ, transport);
                for (usize si = 0; si < storageCount && !compatible; ++si) {
                    u8* storageStore = (u8*)(storageBegin + si * STORE_STRIDE);
                    if (read_i32(storageStore + STORE_TRANSPORT) != transport) continue;
                    float capacity = read_f32(storageStore + STORE_CAPACITY);
                    if (!finite_reasonable(capacity) || capacity <= 0.000001f) continue;
                    if ((hostModes & LANE_MASK_INPUT) &&
                        template_storage_store_matches_host_recipe(storageStore, templ, ANNEX_INPUT))
                        compatible = 1;
                    if (!compatible && (hostModes & LANE_MASK_OUTPUT) &&
                        template_storage_store_matches_host_recipe(storageStore, templ, ANNEX_OUTPUT))
                        compatible = 1;
                }
            }
            if (!compatible) continue;
            float sq = 0.0f;
            if (current_preview_gap_to_building(game, havePreview, preview, building, &sq) &&
                sq <= g_linkGapSq && sq < bestSq) {
                bestSurface = building;
                bestRoot = building;
                bestSq = sq;
            }
        }
    }

    if (bestRoot && outRoot) *outRoot = bestRoot;
    if (bestSurface && outGapSq) *outGapSq = bestSq;
    return bestSurface;
}

static u8* nearest_existing_annex_to_host_preview(void* game, u32 inputMask, u32 outputMask,
                                                  float* outGapSq) {
    if (outGapSq) *outGapSq = 0.0f;
    PreviewFootprint preview = {};
    int havePreview = current_tool_preview_footprint(game, &preview);
    u64 wb = 0;
    usize wc = 0;
    if (!world_building_vector(game, &wb, &wc)) return nullptr;

    u8* best = nullptr;
    float bestSq = g_linkGapSq + 1.0f;
    const i32 transports[] = {TRANSPORT_COVERED, TRANSPORT_OPEN, TRANSPORT_AGGREGATE,
                              TRANSPORT_OIL, TRANSPORT_DRY_BULK, TRANSPORT_COOLER,
                              TRANSPORT_LIVESTOCK, TRANSPORT_WATER, TRANSPORT_WASTE};
    for (usize i = 0; i < wc; ++i) {
        u8* building = (u8*)read_u64((u8*)wb + i * sizeof(u64));
        if (!building) continue;
        u32 annexMask = building_annex_transport_mask(game, building);
        if (!annexMask) continue;
        int compatible = 0;
        for (usize t = 0; t < sizeof(transports)/sizeof(transports[0]); ++t) {
            i32 transport = transports[t];
            u32 bit = transport_bit(transport);
            if ((annexMask & bit) == 0) continue;
            u8 annexModes = building_annex_mode_mask(game, building, transport);
            if (((inputMask & bit) && (annexModes & LANE_MASK_INPUT)) ||
                ((outputMask & bit) && (annexModes & LANE_MASK_OUTPUT))) {
                compatible = 1;
                break;
            }
        }
        if (!compatible) continue;
        float sq = 0.0f;
        if (current_preview_gap_to_building(game, havePreview, preview, building, &sq) &&
            sq <= g_linkGapSq && sq < bestSq) {
            best = building;
            bestSq = sq;
        }
    }
    if (best && outGapSq) *outGapSq = bestSq;
    return best;
}

static int queue_native_green_highlight(void* game, u8* building) {
    if (!game || !building || !g_buildingHighlight || !readable(building, 1)) return 0;
    // Match WRSR's own green-highlight call shape: mode=0, colour selector=0,
    // and the fifth (enable) parameter set to 1.
    g_buildingHighlight((u64)game, (i64)building, 0, 0, 1);
    return 1;
}

static int same_lane(const HostLane* lane, const AnnexState* s) {
    return lane && s && s->active &&
           s->host == lane->building && s->hostStore == lane->hostStore &&
           s->transport == lane->transport && s->mode == lane->mode;
}

static AnnexState* allocate_state_for_lane(const HostLane* lane) {
    // Allocate one state per physical annex, not one state
    // per host lane. Multiple states may therefore share the same hostStore.
    if (!lane) return nullptr;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        if (!g_annexState[i].active && !g_annexState[i].host) {
            AnnexState* s = &g_annexState[i];
            *s = AnnexState{};
            s->host = lane->building;
            s->hostStore = lane->hostStore;
            s->transport = lane->transport;
            s->mode = lane->mode;
            return s;
        }
    }
    if (!g_overflowLogged && g_host && g_host->log) {
        g_host->log("[EnhancedStorageManagement] storage-link state limit reached; additional relationships ignored");
        g_overflowLogged = 1;
    }
    return nullptr;
}

static AnnexState* find_state_for_storage_lane(u8* building, u8* store, const HostLane* lane) {
    if (!building || !store || !lane) return nullptr;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (s->active && s->storage == building && s->annexStore == store && same_lane(lane, s)) return s;
    }
    return nullptr;
}

static u32 collect_unique_active_pool_roots_for_member(u8* member, u8** outRoots, u32 outCapacity) {
    if (!member) return 0;
    u32 count = 0;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* state = &g_annexState[i];
        if (!state->active || state->storage != member || !state->host || state->host == member) continue;
        if (!completed_building(state->host) || !readable(state->host, 1)) continue;
        int duplicate = 0;
        for (u32 r = 0; r < count && r < outCapacity; ++r) {
            if (outRoots && outRoots[r] == state->host) { duplicate = 1; break; }
        }
        if (duplicate) continue;
        if (outRoots && count < outCapacity) outRoots[count] = state->host;
        ++count;
    }
    return count;
}

static u8* active_pool_root_for_member(u8* member) {
    u8* root = nullptr;
    return collect_unique_active_pool_roots_for_member(member, &root, 1) ? root : nullptr;
}

static int storage_building_bound_to_other_root(u8* building, u8* root) {
    if (!building || !root) return 1;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (s->active && s->storage == building && s->host && s->host != root) return 1;
    }
    return 0;
}

static int storage_host_store_is_active_annex(u8* building, u8* store) {
    if (!building || !store) return 0;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active) continue;
        if (s->storage == building && s->annexStore == store && s->host && s->host != building) return 1;
    }
    return 0;
}

static int host_is_storage_delivery_root(u8* building) {
    if (!building) return 0;
    u8* templ = building_template(building);
    i32 type = -1;
    if (!templ || !template_building_type(templ, &type)) return 0;
    return type == BUILDING_TYPE_STORAGE ||
           (type == BUILDING_TYPE_WATER_PUMP && template_engine_idle(templ));
}

static AnnexState* first_state_for_host_store(u8* hostStore) {
    if (!hostStore) return nullptr;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (s->active && s->hostStore == hostStore) return s;
    }
    return nullptr;
}

static void zero_state(AnnexState* s) {
    if (s) *s = AnnexState{};
}

static int host_lane_present(const AnnexState* s, u32 laneCount) {
    if (!s) return 0;
    for (u32 i = 0; i < laneCount; ++i) {
        const HostLane* lane = &g_hostLanes[i];
        if (lane->building == s->host && lane->hostStore == s->hostStore &&
            lane->transport == s->transport && lane->mode == s->mode) return 1;
    }
    return 0;
}

static int storage_present(u8* storage, u8* annexStore, u32 storageCount) {
    for (u32 i = 0; i < storageCount; ++i) {
        if (g_storages[i].building == storage && g_storages[i].store == annexStore) return 1;
    }
    return 0;
}

static int state_layout_valid(AnnexState* s) {
    if (!s || !s->active || !s->host || !s->storage || !s->hostStore || !s->annexStore) return 0;
    if (!supported_annex_transport(s->transport)) return 0;
    if (!completed_building(s->host) || !completed_building(s->storage)) return 0;
    if (!store_belongs_to_building(s->host, s->hostStore) || !store_belongs_to_building(s->storage, s->annexStore)) return 0;
    if (!readable(s->hostStore, STORE_STRIDE) || !readable(s->annexStore, STORE_STRIDE)) return 0;
    if (read_i32(s->hostStore + STORE_TRANSPORT) != s->transport ||
        read_i32(s->annexStore + STORE_TRANSPORT) != s->transport) return 0;
    if (!annex_building_store_eligible(s->storage, s->annexStore)) return 0;
    // Freight delivery descriptors are transient routing buffers. Their resource
    // record vector changes as WRSR routes cargo, so transport + ownership is the
    // stable validity boundary for an established delivery relationship.
    if (building_is_runtime_type(s->storage, BUILDING_TYPE_CARGO_STATION)) return 1;
    HostLane lane = {s->host, s->hostStore, s->transport, s->mode, HOST_RESOURCE_ROOT, 0, 0, 0};
    return lane_annex_store_overlap(&lane, s->annexStore);
}

static int validate_allocation_policy(u8* store) {
    if (!store || !readable(store, STORE_STRIDE)) return 0;
    u64 rb = read_u64(store + STORE_RECORDS_BEGIN);
    u64 re = read_u64(store + STORE_RECORDS_END);
    u64 ab = read_u64(store + STORE_ALLOC_BEGIN);
    u64 ae = read_u64(store + STORE_ALLOC_END);
    usize rc = 0, ac = 0;
    if (!valid_vector(rb, re, RECORD_STRIDE, MAX_POOL_RESOURCES, &rc) ||
        !valid_vector(ab, ae, ALLOC_STRIDE, MAX_POOL_RESOURCES, &ac)) return 0;
    usize count = rc < ac ? rc : ac;
    for (usize i = 0; i < count; ++i) {
        u8* rec = (u8*)rb + i * RECORD_STRIDE;
        u8* e = (u8*)ab + i * ALLOC_STRIDE;
        u8* res = (u8*)read_u64(rec + RECORD_RESOURCE);
        float weight = read_f32(e + ALLOC_WEIGHT);
        float active = read_f32(e + ALLOC_ACTIVE);
        if (!res || !finite_reasonable(weight) || !finite_reasonable(active)) return 0;
    }
    return 1;
}

static int store_has_explicit_positive_allocation(u8* store) {
    if (!store || !readable(store, STORE_STRIDE)) return 0;
    u64 begin = read_u64(store + STORE_ALLOC_BEGIN);
    u64 end = read_u64(store + STORE_ALLOC_END);
    usize count = 0;
    if (!valid_vector(begin, end, ALLOC_STRIDE, 256, &count)) return 0;
    for (usize i = 0; i < count; ++i) {
        u8* e = (u8*)begin + i * ALLOC_STRIDE;
        float weight = read_f32(e + ALLOC_WEIGHT);
        float active = read_f32(e + ALLOC_ACTIVE);
        if (!finite_reasonable(weight) || !finite_reasonable(active)) continue;
        if (active > 0.5f && weight > 0.000001f) return 1;
    }
    return 0;
}

static float store_policy_weight(u8* store, u8* resource, int* outActive) {
    if (outActive) *outActive = 0;
    u8* e = find_allocation_entry(store, resource);
    if (!e) return find_resource_record(store, resource) ? 1.0f : 0.0f;
    float active = read_f32(e + ALLOC_ACTIVE);
    float weight = read_f32(e + ALLOC_WEIGHT);
    if (!finite_reasonable(active) || !finite_reasonable(weight)) return -1.0f;
    if (active <= 0.5f) return 1.0f;
    if (outActive) *outActive = 1;
    return clamp_f(weight, 0.0f, 1.0f);
}

static int annex_resource_allowed(u8* annexStore, u8* resource) {
    if (!annexStore || !resource || !find_resource_record(annexStore, resource)) return 0;
    int active = 0;
    float weight = store_policy_weight(annexStore, resource, &active);
    if (weight < 0.0f) return 0;
    return !active || weight > 0.000001f;
}

static int annex_selection_matches_lane(const HostLane* lane, u8* annexStore,
                                        int trashContainer, int* outHasEvidence) {
    if (outHasEvidence) *outHasEvidence = 0;
    if (!lane || !annexStore || !lane_annex_store_overlap(lane, annexStore)) return 0;

    if (trashContainer) {
        // Trash-container descriptors represent concrete waste classes. Their
        // selector bookkeeping is not a warehouse percentage vector, so do not
        // interpret it here. lane_annex_store_overlap() above already proved an
        // exact resource record shared with the host; that exact identity is the
        // only selection evidence required for waste annexing.
        if (outHasEvidence) *outHasEvidence = 1;
        return 1;
    }

    // Native allocation and existing cargo are evidence for THIS resource slice,
    // not a veto against other selected resources in the same warehouse.
    // One descriptor may contribute independently to every matching
    // lane on the same production root.
    int anyEvidence = 0;
    int laneEvidence = 0;
    u64 rbAlloc = read_u64(annexStore + STORE_RECORDS_BEGIN);
    u64 reAlloc = read_u64(annexStore + STORE_RECORDS_END);
    u64 ab = read_u64(annexStore + STORE_ALLOC_BEGIN);
    u64 ae = read_u64(annexStore + STORE_ALLOC_END);
    usize rcAlloc = 0, ac = 0;
    if (!valid_vector(rbAlloc, reAlloc, RECORD_STRIDE, 256, &rcAlloc) ||
        !valid_vector(ab, ae, ALLOC_STRIDE, 256, &ac)) return 0;
    usize paired = rcAlloc < ac ? rcAlloc : ac;
    for (usize i = 0; i < paired; ++i) {
        u8* rec = (u8*)rbAlloc + i * RECORD_STRIDE;
        u8* e = (u8*)ab + i * ALLOC_STRIDE;
        u8* resource = (u8*)read_u64(rec + RECORD_RESOURCE);
        float weight = read_f32(e + ALLOC_WEIGHT);
        float active = read_f32(e + ALLOC_ACTIVE);
        if (!resource || !finite_reasonable(weight) || !finite_reasonable(active)) return 0;
        if (active <= 0.5f || weight <= 0.000001f ||
            resource_coefficient(resource, lane->transport) <= 0.0f) continue;
        anyEvidence = 1;
        if (lane_accepts_resource(lane, resource)) laneEvidence = 1;
    }

    u64 rb = read_u64(annexStore + STORE_RECORDS_BEGIN);
    u64 re = read_u64(annexStore + STORE_RECORDS_END);
    usize rc = 0;
    if (!valid_vector(rb, re, RECORD_STRIDE, 256, &rc)) return 0;
    for (usize i = 0; i < rc; ++i) {
        u8* rec = (u8*)rb + i * RECORD_STRIDE;
        u8* resource = (u8*)read_u64(rec + RECORD_RESOURCE);
        float amount = read_f32(rec + RECORD_CURRENT);
        if (!resource || !finite_reasonable(amount) || amount < -0.01f) return 0;
        if (amount <= 0.01f || resource_coefficient(resource, lane->transport) <= 0.0f) continue;
        anyEvidence = 1;
        if (lane_accepts_resource(lane, resource)) laneEvidence = 1;
    }

    if (outHasEvidence) *outHasEvidence = anyEvidence;
    return anyEvidence ? laneEvidence : 1;
}

static int explicit_allocation_matches_lane(const HostLane* lane, u8* store, int* outHasExplicit) {
    if (outHasExplicit) *outHasExplicit = 0;
    if (!lane || !store || !readable(store, STORE_STRIDE)) return 0;
    u64 rb = read_u64(store + STORE_RECORDS_BEGIN);
    u64 re = read_u64(store + STORE_RECORDS_END);
    u64 ab = read_u64(store + STORE_ALLOC_BEGIN);
    u64 ae = read_u64(store + STORE_ALLOC_END);
    usize rc = 0, ac = 0;
    if (!valid_vector(rb, re, RECORD_STRIDE, 256, &rc) ||
        !valid_vector(ab, ae, ALLOC_STRIDE, 256, &ac)) return 0;
    usize count = rc < ac ? rc : ac;
    int any = 0;
    int match = 0;
    for (usize i = 0; i < count; ++i) {
        u8* rec = (u8*)rb + i * RECORD_STRIDE;
        u8* alloc = (u8*)ab + i * ALLOC_STRIDE;
        u8* resource = (u8*)read_u64(rec + RECORD_RESOURCE);
        float weight = read_f32(alloc + ALLOC_WEIGHT);
        float active = read_f32(alloc + ALLOC_ACTIVE);
        if (!resource || !finite_reasonable(weight) || !finite_reasonable(active)) return 0;
        if (active <= 0.5f || weight <= 0.000001f ||
            resource_coefficient(resource, lane->transport) <= 0.0f) continue;
        any = 1;
        if (lane_accepts_resource(lane, resource)) match = 1;
    }
    if (outHasExplicit) *outHasExplicit = any;
    return !any || match;
}

static float annex_effective_capacity_for_host(AnnexState* s) {
    if (!s || !state_layout_valid(s)) return 0.0f;
    // Delivery buildings are transfer ports, not storage expansion. Their tiny
    // native routing buffers remain live for WRSR, but contribute zero logical
    // pool capacity. This keeps mechanics aligned with the storage UI contract.
    if (building_is_runtime_type(s->storage, BUILDING_TYPE_CARGO_STATION)) return 0.0f;
    // Trash-container live descriptors are already the native container-slot
    // partition (e.g. 10 large construction-waste containers = 65 t). Their
    // allocation metadata is not a second percentage of that partition.
    if (building_is_runtime_type(s->storage, BUILDING_TYPE_TRASH_CONTAINER)) return s->annexCapacity;
    if (!store_has_explicit_positive_allocation(s->annexStore)) return s->annexCapacity;

    u64 rb = read_u64(s->annexStore + STORE_RECORDS_BEGIN);
    u64 re = read_u64(s->annexStore + STORE_RECORDS_END);
    usize rc = 0;
    if (!valid_vector(rb, re, RECORD_STRIDE, MAX_POOL_RESOURCES, &rc)) return 0.0f;
    float fraction = 0.0f;
    for (usize i = 0; i < rc; ++i) {
        u8* res = (u8*)read_u64((u8*)rb + i * RECORD_STRIDE + RECORD_RESOURCE);
        if (!res || !state_accepts_resource(s, res)) continue;
        int active = 0;
        float w = store_policy_weight(s->annexStore, res, &active);
        if (w < 0.0f) return 0.0f;
        if (active && w > 0.0f) fraction += w;
    }
    fraction = clamp_f(fraction, 0.0f, 1.0f);
    return s->annexCapacity * fraction;
}

// Native storage direction/allocation policy is never modified by this plugin.

static void touch_storage_visual(void* game, u8* building, u8*, i32 transport, u8* resource) {
    // Aggregate stores have a positively traced native visual/material refresh
    // path. Other storage families read their live records directly and are
    // deliberately not poked through an unverified generic "dirty" field.
    if (transport == TRANSPORT_AGGREGATE && g_aggregateStorageTouch && game && building && resource) {
        g_aggregateStorageTouch(game, building, resource);
    }
}

static float store_total_used_units(u8* store, i32 transport) {
    if (!store || !readable(store, STORE_STRIDE)) return -1.0f;
    u64 begin = read_u64(store + STORE_RECORDS_BEGIN);
    u64 end = read_u64(store + STORE_RECORDS_END);
    usize count = 0;
    if (!valid_vector(begin, end, RECORD_STRIDE, 256, &count)) return -1.0f;
    float total = 0.0f;
    for (usize i = 0; i < count; ++i) {
        u8* rec = (u8*)begin + i * RECORD_STRIDE;
        u8* resource = (u8*)read_u64(rec + RECORD_RESOURCE);
        float amount = read_f32(rec + RECORD_CURRENT);
        if (!resource || !finite_reasonable(amount) || amount < -0.01f) return -1.0f;
        if (amount <= 0.0f) continue;
        float coeff = resource_coefficient(resource, transport);
        if (coeff <= 0.0f) continue;
        total += amount / coeff;
    }
    return finite_reasonable(total) ? total : -1.0f;
}

static float resource_units_in_store(u8* store, u8* resource, i32 transport) {
    u8* rec = find_resource_record(store, resource);
    if (!rec) return 0.0f;
    float coeff = resource_coefficient(resource, transport);
    float amount = read_f32(rec + RECORD_CURRENT);
    if (coeff <= 0.0f || !finite_reasonable(amount) || amount <= 0.0f) return 0.0f;
    return amount / coeff;
}

static float resource_policy_max_units(u8* store, float physicalCapacity, u8* resource) {
    if (!store || !resource || physicalCapacity <= 0.0f) return 0.0f;
    int active = 0;
    float weight = store_policy_weight(store, resource, &active);
    if (weight < 0.0f) return 0.0f;
    return active ? physicalCapacity * weight : physicalCapacity;
}

static float resource_free_units(u8* store, float physicalCapacity, i32 transport, u8* resource) {
    if (!store || !resource || !annex_resource_allowed(store, resource)) return 0.0f;
    float totalUsed = store_total_used_units(store, transport);
    if (totalUsed < 0.0f) return 0.0f;
    float totalFree = physicalCapacity - totalUsed;
    if (totalFree <= 0.000001f) return 0.0f;
    float current = resource_units_in_store(store, resource, transport);
    float perMax = resource_policy_max_units(store, physicalCapacity, resource);
    float perFree = perMax - current;
    if (perFree <= 0.000001f) return 0.0f;
    return perFree < totalFree ? perFree : totalFree;
}

static float move_resource_units(void* game, u8* fromBuilding, u8* fromStore,
                                 u8* toBuilding, u8* toStore, i32 transport,
                                 u8* resource, float wantedUnits, float toPhysicalCapacity) {
    if (!fromStore || !toStore || !resource || wantedUnits <= 0.000001f) return 0.0f;
    u8* fromRec = find_resource_record(fromStore, resource);
    u8* toRec = find_resource_record(toStore, resource);
    float coeff = resource_coefficient(resource, transport);
    if (!fromRec || !toRec || coeff <= 0.0f) return 0.0f;
    float fromAmount = read_f32(fromRec + RECORD_CURRENT);
    float toAmount = read_f32(toRec + RECORD_CURRENT);
    if (!finite_reasonable(fromAmount) || !finite_reasonable(toAmount) || fromAmount < -0.01f || toAmount < -0.01f) return 0.0f;
    float available = fromAmount > 0.0f ? fromAmount / coeff : 0.0f;
    float free = resource_free_units(toStore, toPhysicalCapacity, transport, resource);
    float moved = wantedUnits;
    if (moved > available) moved = available;
    if (moved > free) moved = free;
    if (moved <= 0.000001f) return 0.0f;
    float amount = moved * coeff;
    float newFrom = fromAmount - amount;
    if (newFrom < 0.0f && newFrom > -0.001f) newFrom = 0.0f;
    write_f32(fromRec + RECORD_CURRENT, newFrom);
    write_f32(toRec + RECORD_CURRENT, toAmount + amount);
    touch_storage_visual(game, fromBuilding, fromStore, transport, resource);
    touch_storage_visual(game, toBuilding, toStore, transport, resource);
    return moved;
}

static int annex_contents_compatible(AnnexState* s) {
    if (!s || !s->annexStore || !readable(s->annexStore, STORE_STRIDE)) return 0;
    // Unrelated resources are allowed to coexist in a multi-resource annex. They
    // simply occupy physical capacity and are ignored by this particular lane
    // slice. Reject only corrupt/negative live records; never convert cargo.
    u64 begin = read_u64(s->annexStore + STORE_RECORDS_BEGIN);
    u64 end = read_u64(s->annexStore + STORE_RECORDS_END);
    usize count = 0;
    if (!valid_vector(begin, end, RECORD_STRIDE, 256, &count)) return 0;
    for (usize i = 0; i < count; ++i) {
        u8* rec = (u8*)begin + i * RECORD_STRIDE;
        u8* resource = (u8*)read_u64(rec + RECORD_RESOURCE);
        float amount = read_f32(rec + RECORD_CURRENT);
        if (!resource || !finite_reasonable(amount) || amount < -0.01f) return 0;
    }
    return 1;
}

static float group_native_capacity(u8* hostStore) {
    AnnexState* first = first_state_for_host_store(hostStore);
    return first ? first->originalHostCapacity : 0.0f;
}

static float group_pooled_capacity(u8* hostStore) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first) return 0.0f;
    float pooled = first->originalHostCapacity;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active || s->hostStore != hostStore || !state_layout_valid(s)) continue;
        float contribution = annex_effective_capacity_for_host(s);
        if (!finite_reasonable(contribution) || contribution < 0.0f) return 0.0f;
        pooled += contribution;
    }
    if (!finite_reasonable(pooled) || pooled <= 0.0f || pooled > 100000000.0f) return 0.0f;
    return pooled;
}

static float state_effective_used_for_host(AnnexState* s) {
    if (!s || !state_layout_valid(s)) return 0.0f;
    u64 rb = read_u64(s->annexStore + STORE_RECORDS_BEGIN);
    u64 re = read_u64(s->annexStore + STORE_RECORDS_END);
    usize rc = 0;
    if (!valid_vector(rb, re, RECORD_STRIDE, MAX_POOL_RESOURCES, &rc)) return 0.0f;
    const int explicitAllocation = store_has_explicit_positive_allocation(s->annexStore);
    float total = 0.0f;
    for (usize i = 0; i < rc; ++i) {
        u8* res = (u8*)read_u64((u8*)rb + i * RECORD_STRIDE + RECORD_RESOURCE);
        if (!res || !state_accepts_resource(s, res)) continue;
        if (explicitAllocation) {
            int active = 0;
            float w = store_policy_weight(s->annexStore, res, &active);
            if (w < 0.0f || !active || w <= 0.0f) continue;
        }
        total += resource_units_in_store(s->annexStore, res, s->transport);
    }
    return finite_reasonable(total) ? total : 0.0f;
}

static float group_free_annex_capacity(u8* hostStore) {
    float freeTotal = 0.0f;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active || s->hostStore != hostStore || !state_layout_valid(s)) continue;
        float effective = annex_effective_capacity_for_host(s);
        float used = state_effective_used_for_host(s);
        float free = effective - used;
        if (free > 0.0f) freeTotal += free;
    }
    return freeTotal;
}

static void update_group_capacity_metadata(u8* hostStore, float pooled, float shadow) {
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active || s->hostStore != hostStore) continue;
        s->pooledCapacity = pooled;
        if (shadow >= 0.0f) s->lastSimulationShadowCapacity = shadow;
    }
}

static int group_capacity_owned(u8* hostStore, float nativeCap, float newPooled, float* outLive) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || !hostStore || !readable(hostStore + STORE_CAPACITY, sizeof(float))) return 0;
    float live = read_f32(hostStore + STORE_CAPACITY);
    if (outLive) *outLive = live;
    if (!finite_reasonable(live)) return 0;
    if (capacity_close(live, nativeCap) || capacity_close(live, newPooled)) return 1;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active || s->hostStore != hostStore) continue;
        if (capacity_close(live, s->pooledCapacity) || capacity_close(live, s->lastSimulationShadowCapacity)) return 1;
    }
    if (!first->conflictWarned && g_host && g_host->log) {
        g_host->log("[EnhancedStorageManagement] group capacity ownership conflict on host %p/%s: live %.3f, native %.3f, recalculated pooled %.3f; writes suspended",
                    first->host, transport_label(first->transport), live, nativeCap, newPooled);
        first->conflictWarned = 1;
    }
    return 0;
}

static float move_specific_host_resource_to_annexes(void* game, u8* hostStore, u8* resource, float wanted) {
    if (!hostStore || !resource || wanted <= 0.000001f) return 0.0f;
    float moved = 0.0f;
    for (u32 i = 0; i < MAX_ANNEX_STATES && moved + 0.000001f < wanted; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active || s->hostStore != hostStore || !state_layout_valid(s) ||
            building_is_runtime_type(s->storage, BUILDING_TYPE_CARGO_STATION) ||
            !state_accepts_resource(s, resource) || !annex_resource_allowed(s->annexStore, resource)) continue;
        moved += move_resource_units(game, s->host, hostStore, s->storage, s->annexStore,
                                     s->transport, resource, wanted - moved, s->annexCapacity);
    }
    return moved;
}

static float move_host_composition_to_one_annex(void* game, AnnexState* s, float wanted) {
    if (!s || !state_layout_valid(s) || wanted <= 0.000001f) return 0.0f;
    float movedTotal = 0.0f;
    for (u32 pass = 0; pass < MAX_POOL_RESOURCES + 2 && movedTotal + 0.00001f < wanted; ++pass) {
        u64 hb = read_u64(s->hostStore + STORE_RECORDS_BEGIN);
        u64 he = read_u64(s->hostStore + STORE_RECORDS_END);
        usize hc = 0;
        if (!valid_vector(hb, he, RECORD_STRIDE, MAX_POOL_RESOURCES, &hc)) break;
        float weightSum = 0.0f;
        for (usize i = 0; i < hc; ++i) {
            u8* rec = (u8*)hb + i * RECORD_STRIDE;
            u8* res = (u8*)read_u64(rec + RECORD_RESOURCE);
            if (!res || !state_accepts_resource(s, res) || !find_resource_record(s->annexStore, res) ||
                !annex_resource_allowed(s->annexStore, res)) continue;
            float units = resource_units_in_store(s->hostStore, res, s->transport);
            if (units <= 0.000001f || resource_free_units(s->annexStore, s->annexCapacity, s->transport, res) <= 0.000001f) continue;
            weightSum += units;
        }
        if (weightSum <= 0.000001f) break;
        float remainingAtStart = wanted - movedTotal;
        float thisPass = 0.0f;
        for (usize i = 0; i < hc; ++i) {
            u8* rec = (u8*)hb + i * RECORD_STRIDE;
            u8* res = (u8*)read_u64(rec + RECORD_RESOURCE);
            if (!res || !state_accepts_resource(s, res) || !find_resource_record(s->annexStore, res) ||
                !annex_resource_allowed(s->annexStore, res)) continue;
            float units = resource_units_in_store(s->hostStore, res, s->transport);
            if (units <= 0.000001f) continue;
            float share = remainingAtStart * (units / weightSum);
            float moved = move_resource_units(game, s->host, s->hostStore, s->storage, s->annexStore,
                                              s->transport, res, share, s->annexCapacity);
            thisPass += moved;
            movedTotal += moved;
        }
        if (thisPass <= 0.000001f) break;
    }
    return movedTotal;
}

static float move_annex_composition_to_host(void* game, AnnexState* s, float wanted) {
    if (!s || !state_layout_valid(s) || wanted <= 0.000001f) return 0.0f;
    float nativeCap = group_native_capacity(s->hostStore);
    if (nativeCap <= 0.0f) return 0.0f;
    float movedTotal = 0.0f;
    for (u32 pass = 0; pass < MAX_POOL_RESOURCES + 2 && movedTotal + 0.00001f < wanted; ++pass) {
        u64 hb = read_u64(s->hostStore + STORE_RECORDS_BEGIN);
        u64 he = read_u64(s->hostStore + STORE_RECORDS_END);
        usize hc = 0;
        if (!valid_vector(hb, he, RECORD_STRIDE, MAX_POOL_RESOURCES, &hc)) break;
        float weightSum = 0.0f;
        for (usize i = 0; i < hc; ++i) {
            u8* res = (u8*)read_u64((u8*)hb + i * RECORD_STRIDE + RECORD_RESOURCE);
            if (!res || !state_accepts_resource(s, res) || !find_resource_record(s->annexStore, res) ||
                !annex_resource_allowed(s->annexStore, res)) continue;
            float units = resource_units_in_store(s->annexStore, res, s->transport);
            if (units <= 0.000001f) continue;
            float totalHostUsed = store_total_used_units(s->hostStore, s->transport);
            float hostTotalFree = nativeCap - totalHostUsed;
            float hostResMax = resource_policy_max_units(s->hostStore, nativeCap, res);
            float hostResFree = hostResMax - resource_units_in_store(s->hostStore, res, s->transport);
            if (hostTotalFree <= 0.000001f || hostResFree <= 0.000001f) continue;
            weightSum += units;
        }
        if (weightSum <= 0.000001f) break;
        float remainingAtStart = wanted - movedTotal;
        float thisPass = 0.0f;
        for (usize i = 0; i < hc; ++i) {
            u8* res = (u8*)read_u64((u8*)hb + i * RECORD_STRIDE + RECORD_RESOURCE);
            if (!res || !state_accepts_resource(s, res) || !find_resource_record(s->annexStore, res) ||
                !annex_resource_allowed(s->annexStore, res)) continue;
            float units = resource_units_in_store(s->annexStore, res, s->transport);
            if (units <= 0.000001f) continue;
            float share = remainingAtStart * (units / weightSum);
            // move_resource_units asks resource_free_units() on the target. The
            // host is currently published with a pooled/shadow capacity, so use
            // an explicit native-capacity equivalent by temporarily capping the
            // request to the host's native total/per-resource room.
            float totalHostUsed = store_total_used_units(s->hostStore, s->transport);
            float totalRoom = nativeCap - totalHostUsed;
            float resRoom = resource_policy_max_units(s->hostStore, nativeCap, res) -
                            resource_units_in_store(s->hostStore, res, s->transport);
            float room = totalRoom < resRoom ? totalRoom : resRoom;
            if (room <= 0.000001f) continue;
            if (share > room) share = room;
            u8* fromRec = find_resource_record(s->annexStore, res);
            u8* toRec = find_resource_record(s->hostStore, res);
            float coeff = resource_coefficient(res, s->transport);
            if (!fromRec || !toRec || coeff <= 0.0f) continue;
            float fromAmount = read_f32(fromRec + RECORD_CURRENT);
            float toAmount = read_f32(toRec + RECORD_CURRENT);
            if (!finite_reasonable(fromAmount) || !finite_reasonable(toAmount)) continue;
            float available = fromAmount > 0.0f ? fromAmount / coeff : 0.0f;
            float moved = share < available ? share : available;
            if (moved <= 0.000001f) continue;
            write_f32(fromRec + RECORD_CURRENT, fromAmount - moved * coeff);
            write_f32(toRec + RECORD_CURRENT, toAmount + moved * coeff);
            touch_storage_visual(game, s->storage, s->annexStore, s->transport, res);
            touch_storage_visual(game, s->host, s->hostStore, s->transport, res);
            thisPass += moved;
            movedTotal += moved;
        }
        if (thisPass <= 0.000001f) break;
    }
    return movedTotal;
}

// Cargo-station output gateways are access points, not passive overflow annexes.
// WRSR road/rail vehicles only see cargo present in the station's own live routing
// descriptor. After the root simulation has run, keep any real live gateway
// descriptor topped up from the root buffer so normal manual/DO loading can
// consume it. Input gateways need no special path: the ordinary ANNEX_INPUT
// rebalance already drains their unloaded cargo into the root. Template-only
// specialised stations are intentionally excluded here; they require a separate
// transfer-path hook rather than synthesising persistent live descriptors.
static int stage_live_output_gateways(void* game, u8* hostStore) {
    if (!game || !hostStore) return 1;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        const int storageDeliveryRoot = host_is_storage_delivery_root(s->host);
        if (!s->active || s->hostStore != hostStore ||
            (s->mode != ANNEX_OUTPUT && !storageDeliveryRoot) ||
            !building_is_runtime_type(s->storage, BUILDING_TYPE_CARGO_STATION) ||
            !state_layout_valid(s)) continue;
        float used = store_total_used_units(s->annexStore, s->transport);
        if (used < 0.0f) return 0;
        float room = s->annexCapacity - used;
        if (room <= 0.00001f) continue;
        move_host_composition_to_one_annex(game, s, room);
    }
    return 1;
}

static int rebalance_group(void* game, u8* hostStore) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || !state_layout_valid(first)) return 0;
    float nativeCap = first->originalHostCapacity;
    if (!finite_reasonable(nativeCap) || nativeCap <= 0.0f) return 0;

    // Validate every physical annex before moving any cargo.
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active || s->hostStore != hostStore) continue;
        if (!state_layout_valid(s) || !annex_contents_compatible(s)) {
            if (!s->contaminationWarned && g_host && g_host->log) {
                g_host->log("[EnhancedStorageManagement] linked %s storage %p has incompatible live cargo/layout; pool rebalance suspended rather than converting cargo",
                            transport_label(s->transport), s->storage);
                s->contaminationWarned = 1;
            }
            return 0;
        }
        s->contaminationWarned = 0;
    }

    // First enforce any explicit native host per-resource allocation caps.
    u64 hb = read_u64(hostStore + STORE_RECORDS_BEGIN);
    u64 he = read_u64(hostStore + STORE_RECORDS_END);
    usize hc = 0;
    if (!valid_vector(hb, he, RECORD_STRIDE, MAX_POOL_RESOURCES, &hc)) return 0;
    for (usize i = 0; i < hc; ++i) {
        u8* res = (u8*)read_u64((u8*)hb + i * RECORD_STRIDE + RECORD_RESOURCE);
        if (!res) continue;
        float current = resource_units_in_store(hostStore, res, first->transport);
        float maxUnits = resource_policy_max_units(hostStore, nativeCap, res);
        float excess = current - maxUnits;
        if (excess > 0.00001f) move_specific_host_resource_to_annexes(game, hostStore, res, excess);
    }

    // The native descriptor itself must be back within its original physical
    // capacity before the building simulation executes. Distribute any total
    // overflow across all annexes, preserving composition where possible.
    float hostUsed = store_total_used_units(hostStore, first->transport);
    if (hostUsed < 0.0f) return 0;
    float overflow = hostUsed - nativeCap;
    if (overflow > 0.00001f) {
        for (u32 i = 0; i < MAX_ANNEX_STATES && overflow > 0.00001f; ++i) {
            AnnexState* s = &g_annexState[i];
            if (!s->active || s->hostStore != hostStore || !state_layout_valid(s) ||
                building_is_runtime_type(s->storage, BUILDING_TYPE_CARGO_STATION)) continue;
            float moved = move_host_composition_to_one_annex(game, s, overflow);
            overflow -= moved;
        }
    }
    hostUsed = store_total_used_units(hostStore, first->transport);
    if (hostUsed < 0.0f || hostUsed > nativeCap + 0.001f) return 0;

    // Input annexes stage stock into the native input buffer so WRSR consumes
    // from its own descriptor exactly as before. Output annexes remain overflow
    // storage and are not pulled back into the factory.
    if (first->mode == ANNEX_INPUT) {
        hostUsed = store_total_used_units(hostStore, first->transport);
        float room = nativeCap - hostUsed;
        if (room > 0.00001f) {
            for (u32 i = 0; i < MAX_ANNEX_STATES && room > 0.00001f; ++i) {
                AnnexState* s = &g_annexState[i];
                if (!s->active || s->hostStore != hostStore || !state_layout_valid(s)) continue;
                float moved = move_annex_composition_to_host(game, s, room);
                room -= moved;
            }
        }
    }

    float pooled = group_pooled_capacity(hostStore);
    if (pooled <= 0.0f) return 0;
    update_group_capacity_metadata(hostStore, pooled, -1.0f);
    first->conflictWarned = 0;
    return 1;
}

static int redistribute_output_water_group(void* game, u8* hostStore) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || first->mode != ANNEX_OUTPUT || first->transport != TRANSPORT_WATER ||
        !state_layout_valid(first)) return 1;

    u64 hb = read_u64(hostStore + STORE_RECORDS_BEGIN);
    u64 he = read_u64(hostStore + STORE_RECORDS_END);
    usize hc = 0;
    if (!valid_vector(hb, he, RECORD_STRIDE, MAX_POOL_RESOURCES, &hc)) return 0;
    const float hostCap = first->originalHostCapacity;
    if (!finite_reasonable(hostCap) || hostCap <= 0.0f) return 0;

    for (usize r = 0; r < hc; ++r) {
        u8* hostRec = (u8*)hb + r * RECORD_STRIDE;
        u8* resource = (u8*)read_u64(hostRec + RECORD_RESOURCE);
        float coeff = resource_coefficient(resource, TRANSPORT_WATER);
        if (!resource || coeff <= 0.0f) continue;

        u8* stores[MAX_POOL_RESOURCES] = {};
        u8* buildings[MAX_POOL_RESOURCES] = {};
        float caps[MAX_POOL_RESOURCES] = {};
        u32 count = 0;
        float totalCap = hostCap;
        float totalUnits = resource_units_in_store(hostStore, resource, TRANSPORT_WATER);

        for (u32 i = 0; i < MAX_ANNEX_STATES && count < MAX_POOL_RESOURCES; ++i) {
            AnnexState* st = &g_annexState[i];
            if (!st->active || st->hostStore != hostStore || st->transport != TRANSPORT_WATER ||
                st->mode != ANNEX_OUTPUT || !state_layout_valid(st) ||
                !find_resource_record(st->annexStore, resource)) continue;
            int duplicate = 0;
            for (u32 j = 0; j < count; ++j) if (stores[j] == st->annexStore) { duplicate = 1; break; }
            if (duplicate) continue;
            float cap = st->annexCapacity;
            if (!finite_reasonable(cap) || cap <= 0.0f) continue;
            stores[count] = st->annexStore;
            buildings[count] = st->storage;
            caps[count] = cap;
            totalCap += cap;
            totalUnits += resource_units_in_store(st->annexStore, resource, TRANSPORT_WATER);
            ++count;
        }
        if (count == 0 || totalCap <= 0.0f || totalUnits < 0.0f) continue;
        if (totalUnits > totalCap + 0.01f) return 0;

        // Treat every physical output connection as an outlet on one collective
        // tank: maintain the same fill ratio in the native plant buffer and all
        // directly attached reservoirs. When one reservoir is drained through a
        // pipe, stock in the other reservoirs therefore replenishes it on the next
        // simulation dispatch instead of marooning water behind another outlet.
        const float fill = clamp_f(totalUnits / totalCap, 0.0f, 1.0f);
        float hostTarget = hostCap * fill;
        write_f32(hostRec + RECORD_CURRENT, hostTarget * coeff);
        touch_storage_visual(game, first->host, hostStore, TRANSPORT_WATER, resource);
        for (u32 i = 0; i < count; ++i) {
            u8* rec = find_resource_record(stores[i], resource);
            if (!rec) continue;
            float target = caps[i] * fill;
            write_f32(rec + RECORD_CURRENT, target * coeff);
            touch_storage_visual(game, buildings[i], stores[i], TRANSPORT_WATER, resource);
        }
    }
    return 1;
}

// Temporarily gather an OUTPUT group's real cargo into the root descriptor while
// WRSR performs its native simulation/logistics work.  Capacity virtualization by
// itself is insufficient: vehicle loaders stop when the root's own resource record
// reaches zero even when annex records still contain stock.  The root is therefore
// used as the transient access window for the whole physical pool.  publish_group_
// after_simulation() immediately spills any amount above native capacity back into
// the annexes, so save-visible physical ownership remains distributed.
static int stage_output_pool_into_host(void* game, u8* hostStore) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || first->mode != ANNEX_OUTPUT || !state_layout_valid(first)) return 1;
    const float pooled = group_pooled_capacity(hostStore);
    if (!finite_reasonable(pooled) || pooled <= 0.0f) return 0;

    float hostUsed = store_total_used_units(hostStore, first->transport);
    if (hostUsed < 0.0f) return 0;
    float totalRoom = pooled - hostUsed;
    if (totalRoom <= 0.00001f) return 1;

    u64 hb = read_u64(hostStore + STORE_RECORDS_BEGIN);
    u64 he = read_u64(hostStore + STORE_RECORDS_END);
    usize hc = 0;
    if (!valid_vector(hb, he, RECORD_STRIDE, MAX_POOL_RESOURCES, &hc)) return 0;

    for (usize r = 0; r < hc && totalRoom > 0.00001f; ++r) {
        u8* hostRec = (u8*)hb + r * RECORD_STRIDE;
        u8* resource = (u8*)read_u64(hostRec + RECORD_RESOURCE);
        if (!resource) continue;
        const float resourceCap = group_resource_capacity_for_mechanics(hostStore, resource);
        float resourceUsed = resource_units_in_store(hostStore, resource, first->transport);
        float resourceRoom = resourceCap - resourceUsed;
        if (resourceRoom <= 0.00001f) continue;
        if (resourceRoom > totalRoom) resourceRoom = totalRoom;

        for (u32 i = 0; i < MAX_ANNEX_STATES && resourceRoom > 0.00001f; ++i) {
            AnnexState* s = &g_annexState[i];
            if (!s->active || s->hostStore != hostStore || !state_layout_valid(s) ||
                !state_accepts_resource(s, resource) || !annex_resource_allowed(s->annexStore, resource)) continue;
            u8* fromRec = find_resource_record(s->annexStore, resource);
            u8* toRec = find_resource_record(hostStore, resource);
            float coeff = resource_coefficient(resource, first->transport);
            if (!fromRec || !toRec || coeff <= 0.0f) continue;
            float fromAmount = read_f32(fromRec + RECORD_CURRENT);
            float toAmount = read_f32(toRec + RECORD_CURRENT);
            if (!finite_reasonable(fromAmount) || !finite_reasonable(toAmount) || fromAmount <= 0.0f) continue;
            float available = fromAmount / coeff;
            float moved = available < resourceRoom ? available : resourceRoom;
            if (moved <= 0.000001f) continue;
            write_f32(fromRec + RECORD_CURRENT, fromAmount - moved * coeff);
            write_f32(toRec + RECORD_CURRENT, toAmount + moved * coeff);
            touch_storage_visual(game, s->storage, s->annexStore, s->transport, resource);
            touch_storage_visual(game, s->host, hostStore, s->transport, resource);
            resourceRoom -= moved;
            totalRoom -= moved;
        }
    }
    return 1;
}

static __declspec(noinline) int prepare_group_for_simulation(void* game, u8* hostStore) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first) return 0;
    float nativeCap = first->originalHostCapacity;
    float pooled = group_pooled_capacity(hostStore);
    float live = 0.0f;
    if (pooled <= 0.0f || !group_capacity_owned(hostStore, nativeCap, pooled, &live)) return 0;
    if (!rebalance_group(game, hostStore)) return 0;
    pooled = group_pooled_capacity(hostStore);
    // Publish the full physical capacity before gathering output stock. Native
    // vehicle/logistics code can then consume beyond the root's authored buffer
    // during this dispatch without tripping its ordinary capacity gates.
    write_f32(hostStore + STORE_CAPACITY, pooled);
    if (!stage_output_pool_into_host(game, hostStore)) {
        write_f32(hostStore + STORE_CAPACITY, nativeCap);
        update_group_capacity_metadata(hostStore, pooled, nativeCap);
        return 0;
    }
    if (!redistribute_output_water_group(game, hostStore)) {
        write_f32(hostStore + STORE_CAPACITY, nativeCap);
        update_group_capacity_metadata(hostStore, pooled, nativeCap);
        return 0;
    }
    pooled = group_pooled_capacity(hostStore);
    float freeAnnex = group_free_annex_capacity(hostStore);
    float shadow = nativeCap + freeAnnex;
    if (first->mode == ANNEX_OUTPUT) shadow = pooled; // stock is deliberately consolidated for access
    if (shadow < nativeCap) shadow = nativeCap;
    if (shadow > pooled) shadow = pooled;
    write_f32(hostStore + STORE_CAPACITY, shadow);
    update_group_capacity_metadata(hostStore, pooled, shadow);
    return 1;
}

static __declspec(noinline) void publish_group_after_simulation(void* game, u8* hostStore) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || !hostStore || !readable(hostStore + STORE_CAPACITY, sizeof(float))) return;
    float pooled = group_pooled_capacity(hostStore);
    float nativeCap = first->originalHostCapacity;
    float live = 0.0f;
    // If our transient shadow is still the live value, it is ours to clean up
    // even when a later validation/rebalance step fails. If another plugin has
    // changed the capacity to an unrelated value, group_capacity_owned() fails
    // and we deliberately do not overwrite that external state.
    if (pooled <= 0.0f || !group_capacity_owned(hostStore, nativeCap, pooled, &live)) return;

    int ok = state_layout_valid(first);
    if (ok) ok = rebalance_group(game, hostStore);
    if (ok) ok = stage_live_output_gateways(game, hostStore);
    if (ok) ok = redistribute_output_water_group(game, hostStore);
    pooled = group_pooled_capacity(hostStore);

    // SAVE-SAFETY INVARIANT: pooled capacity is transient only. Restore the
    // authored/native descriptor on EVERY owned exit path, including a failed
    // rebalance/gateway validation, so no simulation error can strand the
    // synthetic capacity in save-visible state.
    write_f32(hostStore + STORE_CAPACITY, nativeCap);
    if (finite_reasonable(pooled) && pooled > 0.0f)
        update_group_capacity_metadata(hostStore, pooled, nativeCap);
    if (ok) first->conflictWarned = 0;
}

static int world_vector_quick(void* game, u64* outBegin, usize* outCount, u64* outSignature) {
    if (!game || !outBegin || !outCount || !outSignature || !readable((u8*)game + GAME_BUILDINGS_BEGIN, 16)) return 0;
    u64 begin = read_u64((u8*)game + GAME_BUILDINGS_BEGIN);
    u64 end = read_u64((u8*)game + GAME_BUILDINGS_END);
    if (end < begin || ((end - begin) % sizeof(u64)) != 0) return 0;
    usize count = (usize)((end - begin) / sizeof(u64));
    if (count > 200000) return 0;
    u64 first = 0, middle = 0, last = 0;
    if (count) {
        if (!readable((void*)begin, sizeof(u64)) || !readable((void*)(begin + (count - 1) * sizeof(u64)), sizeof(u64))) return 0;
        first = read_u64((u8*)begin);
        last = read_u64((u8*)begin + (count - 1) * sizeof(u64));
        usize mid = count / 2;
        if (readable((void*)(begin + mid * sizeof(u64)), sizeof(u64))) middle = read_u64((u8*)begin + mid * sizeof(u64));
    }
    u64 sig = begin ^ (end << 7) ^ ((u64)count * 0x9E3779B185EBCA87ull) ^
              (first >> 3) ^ (middle << 11) ^ (last << 23);
    *outBegin = begin;
    *outCount = count;
    *outSignature = sig;
    return 1;
}

static int building_in_world_vector(u64 begin, usize count, u8* target) {
    if (!target) return 0;
    for (usize i = 0; i < count; ++i) {
        u8* slot = (u8*)begin + i * sizeof(u64);
        if (!readable(slot, sizeof(u64))) return 0;
        if ((u8*)read_u64(slot) == target) return 1;
    }
    return 0;
}

static u64 g_worldGuardSignature = 0;

static void guard_active_states_against_world_transition(void* game) {
    u64 begin = 0, signature = 0;
    usize count = 0;
    if (!world_vector_quick(game, &begin, &count, &signature)) return;
    if (g_worldGuardSignature == 0) {
        g_worldGuardSignature = signature;
        return;
    }
    if (signature == g_worldGuardSignature) return;

    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active) continue;
        int hostPresent = building_in_world_vector(begin, count, s->host);
        int storagePresent = building_in_world_vector(begin, count, s->storage);
        if (hostPresent && storagePresent) continue;

        u8* hostStore = s->hostStore;
        float nativeCap = s->originalHostCapacity;
        float oldPooled = s->pooledCapacity;
        float oldShadow = s->lastSimulationShadowCapacity;
        if (storagePresent && s->annexStore && store_belongs_to_building(s->storage, s->annexStore))         zero_state(s);

        if (hostPresent && hostStore && readable(hostStore + STORE_CAPACITY, sizeof(float))) {
            float live = read_f32(hostStore + STORE_CAPACITY);
            if (capacity_close(live, oldPooled) || capacity_close(live, oldShadow) || capacity_close(live, nativeCap)) {
                AnnexState* remaining = first_state_for_host_store(hostStore);
                float target = nativeCap;
                if (finite_reasonable(target) && target > 0.0f) {
                    write_f32(hostStore + STORE_CAPACITY, target);
                    if (remaining) update_group_capacity_metadata(hostStore, group_pooled_capacity(hostStore), target);
                }
            }
        }
    }
    g_worldGuardSignature = signature;
}

static void prepare_all_active_pairs(void* game) {
    guard_active_states_against_world_transition(game);
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active || !s->hostStore) continue;
        int earlier = 0;
        for (u32 j = 0; j < i; ++j) {
            if (g_annexState[j].active && g_annexState[j].hostStore == s->hostStore) { earlier = 1; break; }
        }
        if (!earlier) prepare_group_for_simulation(game, s->hostStore);
    }
}

static void publish_all_active_pairs(void* game) {
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active || !s->hostStore) continue;
        int earlier = 0;
        for (u32 j = 0; j < i; ++j) {
            if (g_annexState[j].active && g_annexState[j].hostStore == s->hostStore) { earlier = 1; break; }
        }
        if (!earlier) publish_group_after_simulation(game, s->hostStore);
    }
}

static void detach_state(void* game, AnnexState* s, const char*) {
    if (!s || !s->host) return;
    u8* hostStore = s->hostStore;
    float nativeCap = s->originalHostCapacity;

    if (s->active && state_layout_valid(s)) {
        // Best-effort settle the whole lane before removing one physical annex.
        // Failure is non-destructive: cargo remains wherever WRSR currently has it.
        rebalance_group(game, hostStore);
            }
    zero_state(s);

    AnnexState* remaining = first_state_for_host_store(hostStore);
    float target = nativeCap;
    if (hostStore && readable(hostStore + STORE_CAPACITY, sizeof(float)) && finite_reasonable(target) && target > 0.0f) {
        write_f32(hostStore + STORE_CAPACITY, target);
        if (remaining) update_group_capacity_metadata(hostStore, group_pooled_capacity(hostStore), target);
    }
}

static void expire_annex_states(void* game, u32 laneCount, u32 storageCount) {
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->host) continue;
        int lanePresent = host_lane_present(s, laneCount);
        int annexPresent = storage_present(s->storage, s->annexStore, storageCount);
        if (lanePresent && annexPresent) continue;
        if (s->active && state_layout_valid(s)) {
            detach_state(game, s, !lanePresent ? " (host lane no longer eligible)" : " (annex no longer eligible)");
            continue;
        }

        u8* hostStore = s->hostStore;
        float nativeCap = s->originalHostCapacity;
        if (annexPresent)         zero_state(s);
        AnnexState* remaining = first_state_for_host_store(hostStore);
        float target = nativeCap;
        if (lanePresent && hostStore && readable(hostStore + STORE_CAPACITY, sizeof(float)) && finite_reasonable(target) && target > 0.0f) {
            write_f32(hostStore + STORE_CAPACITY, target);
            if (remaining) update_group_capacity_metadata(hostStore, group_pooled_capacity(hostStore), target);
        }
    }
}

static void detach_gateway_states_hosted_by_storage_store(void* game, u8* storageBuilding, u8* storageStore) {
    if (!storageBuilding || !storageStore) return;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active || s->host != storageBuilding || s->hostStore != storageStore) continue;
        if (!building_is_runtime_type(s->storage, BUILDING_TYPE_CARGO_STATION)) continue;
        detach_state(game, s, " (storage joined a production/resource root; delivery gateway will migrate to the shared root pool)");
    }
}

static void apply_pool_pair(void* game, HostLane* lane, StorageCandidate* storage) {
    if (!lane || !storage || !lane->building || !storage->building || !lane->hostStore || !storage->store) return;
    if (lane->transport != storage->transport || !supported_annex_transport(lane->transport)) return;
    if (lane->rootClass == HOST_STORAGE_DELIVERY_ONLY) {
        if (!storage->cargoGateway || storage_host_store_is_active_annex(lane->building, lane->hostStore)) return;
    } else if ((storage->modeMask & lane_mode_mask(lane->mode)) == 0) {
        return;
    }
    if (!storage->cargoGateway && !lane_annex_store_overlap(lane, storage->store)) return;

    // If an ordinary storage that was previously acting as a standalone freight
    // root is now annexed to a production/resource root, collapse that topology
    // before creating the production relationship. The gateway will immediately
    // re-resolve through the storage contact surface on the gateway pass below.
    if (lane->rootClass == HOST_RESOURCE_ROOT && !storage->cargoGateway &&
        host_is_storage_delivery_root(storage->building)) {
        detach_gateway_states_hosted_by_storage_store(game, storage->building, storage->store);
    }

    // One physical annex building may expose several independent WRSR storage
    // descriptors (for example covered + cooler) and one descriptor may be split
    // across several resource lanes by the native percentage selector. All of
    // those slices may serve the SAME production root. What remains forbidden is
    // one physical building silently serving two unrelated roots at once.
    if (storage_building_bound_to_other_root(storage->building, lane->building)) return;

    AnnexState* existing = find_state_for_storage_lane(storage->building, storage->store, lane);
    if (existing && existing->active) {
        float pooled = group_pooled_capacity(lane->hostStore);
        if (pooled > 0.0f) {
            update_group_capacity_metadata(lane->hostStore, pooled, existing->originalHostCapacity);
            float live = 0.0f;
            if (group_capacity_owned(lane->hostStore, existing->originalHostCapacity, pooled, &live) &&
                !capacity_close(live, existing->originalHostCapacity)) {
                write_f32(lane->hostStore + STORE_CAPACITY, existing->originalHostCapacity);
            }
        }
        return;
    }

    float annexCapacity = 0.0f;
    if (!valid_capacity_store(storage->store, lane->transport, &annexCapacity)) return;
    AnnexState* group = first_state_for_host_store(lane->hostStore);
    float hostCapacity = 0.0f;
    if (group) {
        hostCapacity = group->originalHostCapacity;
    } else {
        float liveCapacity = 0.0f;
        if (!valid_capacity_store(lane->hostStore, lane->transport, &liveCapacity)) return;
        // Prefer the immutable authored capacity for the root descriptor rather
        // than any transient capacity shadow that may be active in the live store.
        if (!authored_capacity_for_live_store(lane->building, lane->hostStore,
                                              lane->transport, &hostCapacity)) {
            hostCapacity = liveCapacity;
        }
    }
    if (!finite_reasonable(hostCapacity) || hostCapacity <= 0.01f) return;

    i32 storageType = -1;
    u8* storageTempl = building_template(storage->building);
    if (!storageTempl || !template_building_type(storageTempl, &storageType)) return;
    int trashContainer = storageType == BUILDING_TYPE_TRASH_CONTAINER;
    if (!storage->cargoGateway && !annex_selection_matches_lane(lane, storage->store, trashContainer, nullptr)) return;

    AnnexState* s = allocate_state_for_lane(lane);
    if (!s) return;
    s->storage = storage->building;
    s->annexStore = storage->store;
    s->transport = lane->transport;
    s->originalHostCapacity = hostCapacity;
    s->annexCapacity = annexCapacity;
    s->pooledCapacity = hostCapacity;
    s->lastSimulationShadowCapacity = hostCapacity;
    s->active = 1;
    s->explicitSelection = (!trashContainer && !storage->cargoGateway &&
                            store_has_explicit_positive_allocation(storage->store)) ? 1 : 0;

    // Native allocation, direction and external-logistics policy remain WRSR-owned
    // and are never rewritten here.
    if (!validate_allocation_policy(s->annexStore) || !state_layout_valid(s) || !annex_contents_compatible(s)) {
        zero_state(s);
        return;
    }

    const int deliveryGateway = storage->cargoGateway ||
                                building_is_runtime_type(storage->building, BUILDING_TYPE_CARGO_STATION);
    float contribution = annex_effective_capacity_for_host(s);
    float pooled = group_pooled_capacity(lane->hostStore);
    // A delivery gateway is a zero-capacity relationship by design. Keep the
    // state so routing/UI/transfer code can resolve through it, but do not make
    // its 1 t native buffer a prerequisite for or contribution to the pool.
    if (!finite_reasonable(contribution) || (!deliveryGateway && contribution <= 0.00001f) ||
        !finite_reasonable(pooled) || pooled < hostCapacity ||
        (!deliveryGateway && pooled <= hostCapacity) || pooled > 100000000.0f) {
        zero_state(s);
        return;
    }
    update_group_capacity_metadata(lane->hostStore, pooled, hostCapacity);
    if (!rebalance_group(game, lane->hostStore)) {
        zero_state(s);
        AnnexState* remaining = first_state_for_host_store(lane->hostStore);
        float target = hostCapacity;
        if (finite_reasonable(target) && target > 0.0f) {
            write_f32(lane->hostStore + STORE_CAPACITY, target);
            if (remaining) update_group_capacity_metadata(lane->hostStore, group_pooled_capacity(lane->hostStore), target);
        }
        return;
    }
    pooled = group_pooled_capacity(lane->hostStore);
    write_f32(lane->hostStore + STORE_CAPACITY, hostCapacity);
    update_group_capacity_metadata(lane->hostStore, pooled, hostCapacity);
}

static int add_host_lane_class(u8* building, u8* hostStore, i32 transport, AnnexMode mode,
                               u8 rootClass, u32* laneCount) {
    if (!building || !hostStore || !laneCount || !supported_annex_transport(transport)) return 0;
    for (u32 i = 0; i < *laneCount; ++i) {
        // Keep one semantic lane per physical
        // host descriptor. Storage-delivery roots also use one canonical lane.
        if (g_hostLanes[i].building == building && g_hostLanes[i].hostStore == hostStore) return 1;
    }
    if (*laneCount >= MAX_HOST_LANES) return 0;
    HostLane* lane = &g_hostLanes[(*laneCount)++];
    *lane = HostLane{};
    lane->building = building;
    lane->hostStore = hostStore;
    lane->transport = transport;
    lane->mode = mode;
    lane->rootClass = rootClass;
    return 1;
}

static int add_host_lane(u8* building, u8* hostStore, i32 transport, AnnexMode mode, u32* laneCount) {
    return add_host_lane_class(building, hostStore, transport, mode, HOST_RESOURCE_ROOT, laneCount);
}

// A standalone storage can be the physical stock root for freight delivery
// buildings. Keep one canonical INPUT-mode group per descriptor: gateway cargo
// is drained into the storage before simulation and the same gateway is topped
// back up afterwards for vehicle loading. This makes the relation bidirectional
// without creating two opposite-mode groups on the same native store.
static int add_storage_delivery_host_lane(u8* building, u8* store, i32 transport, u32* laneCount) {
    return add_host_lane_class(building, store, transport, ANNEX_INPUT,
                               HOST_STORAGE_DELIVERY_ONLY, laneCount);
}

static void add_recipe_store_lanes(u8* building, u8* templ, AnnexMode mode,
                                   usize recipeBegin, usize recipeEnd, u32* laneCount,
                                   u32 allowedTransportMask) {
    if (!building || !templ || !laneCount) return;
    u64 sb = 0;
    usize sc = 0;
    if (!building_store_vector(building, &sb, &sc)) return;
    for (usize i = 0; i < sc; ++i) {
        u8* store = (u8*)(sb + i * STORE_STRIDE);
        i32 transport = read_i32(store + STORE_TRANSPORT);
        if (!supported_annex_transport(transport) || (allowedTransportMask & transport_bit(transport)) == 0) continue;
        if (!store_matches_recipe(store, templ, recipeBegin, recipeEnd)) continue;
        if (!store_direction_matches(store, mode)) {
            // Workshop factories sometimes use a neutral $STORAGE descriptor
            // instead of $STORAGE_IMPORT/$STORAGE_EXPORT. When the descriptor's
            // resources occur on only one side of the recipe, the recipe itself
            // is authoritative enough to infer direction. If the same descriptor
            // participates on both sides, remain conservative and do not guess.
            usize otherBegin = mode == ANNEX_INPUT ? TEMPLATE_PRODUCE_BEGIN : TEMPLATE_CONSUME_BEGIN;
            usize otherEnd   = mode == ANNEX_INPUT ? TEMPLATE_PRODUCE_END   : TEMPLATE_CONSUME_END;
            if (store_matches_recipe(store, templ, otherBegin, otherEnd)) continue;
        }
        add_host_lane(building, store, transport, mode, laneCount);
    }
}

static void add_live_mine_recipe_lanes(u8* building, u8* templ, u32* laneCount) {
    if (!building || !templ || !laneCount) return;
    u64 pb = 0;
    usize pc = 0;
    if (!template_resource_vector(templ, TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END, &pb, &pc)) return;
    u32 minedMask = 0;
    for (usize r = 0; r < pc; ++r) {
        u8* resource = recipe_resource_at(pb, r, RECIPE_OUTPUT_RATE, nullptr);
        if (!resource) continue;
        i32 t = resource_supported_transport(resource, transport_bit(TRANSPORT_AGGREGATE) | transport_bit(TRANSPORT_OIL));
        if (!supported_mine_transport(t)) continue;
        add_mined_resource(resource, t);
        minedMask |= transport_bit(t);
    }
    if (minedMask) add_recipe_store_lanes(building, templ, ANNEX_OUTPUT,
                                          TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END,
                                          laneCount, minedMask);
}

static void add_production_waste_lanes(u8* building, u32* laneCount) {
    if (!building || !laneCount) return;
    u64 sb = 0;
    usize sc = 0;
    if (!building_store_vector(building, &sb, &sc)) return;
    for (usize i = 0; i < sc; ++i) {
        u8* store = (u8*)(sb + i * STORE_STRIDE);
        if (read_i32(store + STORE_TRANSPORT) != TRANSPORT_WASTE || !store_has_pool_resources(store)) continue;
        if (!store_direction_matches(store, ANNEX_OUTPUT)) continue;
        add_host_lane(building, store, TRANSPORT_WASTE, ANNEX_OUTPUT, laneCount);
    }
}

static u32 general_supported_recipe_mask() {
    return transport_bit(TRANSPORT_COVERED) | transport_bit(TRANSPORT_OPEN) |
           transport_bit(TRANSPORT_AGGREGATE) | transport_bit(TRANSPORT_OIL) |
           transport_bit(TRANSPORT_DRY_BULK) | transport_bit(TRANSPORT_COOLER) |
           transport_bit(TRANSPORT_LIVESTOCK) | transport_bit(TRANSPORT_WATER);
}

static void add_factory_semantic_lanes(u8* building, u8* templ, u32* laneCount) {
    if (!building || !templ || !laneCount) return;
    u32 mask = general_supported_recipe_mask();
    add_recipe_store_lanes(building, templ, ANNEX_OUTPUT,
                           TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END, laneCount, mask);
    add_recipe_store_lanes(building, templ, ANNEX_INPUT,
                           TEMPLATE_CONSUME_BEGIN, TEMPLATE_CONSUME_END, laneCount, mask);
    add_production_waste_lanes(building, laneCount);
}

static void add_water_treatment_lanes(u8* building, u8* templ, u32* laneCount) {
    if (!building || !templ || !laneCount) return;
    u32 mask = general_supported_recipe_mask();
    add_recipe_store_lanes(building, templ, ANNEX_OUTPUT,
                           TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END, laneCount, mask);
    add_recipe_store_lanes(building, templ, ANNEX_INPUT,
                           TEMPLATE_CONSUME_BEGIN, TEMPLATE_CONSUME_END, laneCount, mask);
    add_production_waste_lanes(building, laneCount);
}

static void add_powerplant_lanes(u8* building, u8* templ, u32* laneCount) {
    if (!building || !templ || !laneCount) return;
    add_recipe_store_lanes(building, templ, ANNEX_INPUT,
                           TEMPLATE_CONSUME_BEGIN, TEMPLATE_CONSUME_END,
                           laneCount, general_supported_recipe_mask());
    add_production_waste_lanes(building, laneCount);
}

static void add_shop_pub_lanes(u8* building, u32* laneCount) {
    if (!building || !laneCount) return;
    u64 sb = 0;
    usize sc = 0;
    if (!building_store_vector(building, &sb, &sc)) return;
    for (usize i = 0; i < sc; ++i) {
        u8* store = (u8*)(sb + i * STORE_STRIDE);
        i32 transport = read_i32(store + STORE_TRANSPORT);
        if (!supported_annex_transport(transport) || !store_has_pool_resources(store)) continue;
        add_host_lane(building, store, transport, ANNEX_INPUT, laneCount);
    }
}

static void add_explicit_input_store_lanes(u8* building, u32* laneCount, u32 allowedMask) {
    if (!building || !laneCount) return;
    u64 sb = 0;
    usize sc = 0;
    if (!building_store_vector(building, &sb, &sc)) return;
    for (usize i = 0; i < sc; ++i) {
        u8* store = (u8*)(sb + i * STORE_STRIDE);
        i32 transport = read_i32(store + STORE_TRANSPORT);
        float cap = read_f32(store + STORE_CAPACITY);
        if (!supported_annex_transport(transport) ||
            (allowedMask & transport_bit(transport)) == 0 ||
            !finite_reasonable(cap) || cap <= 0.000001f ||
            !store_has_pool_resources(store) || !store_direction_matches(store, ANNEX_INPUT)) continue;
        add_host_lane(building, store, transport, ANNEX_INPUT, laneCount);
    }
}

static void add_farm_lanes(u8* building, u8* templ, u32* laneCount) {
    if (!building || !templ || !laneCount) return;
    // Native farms produce crops through an EXPORT covered store, while fuel,
    // liquid fertiliser and solid fertiliser are operational input stores and do
    // not all appear in the ordinary recipe consumption vector.
    add_recipe_store_lanes(building, templ, ANNEX_OUTPUT,
                           TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END,
                           laneCount, general_supported_recipe_mask());
    add_explicit_input_store_lanes(building, laneCount,
        transport_bit(TRANSPORT_OIL) | transport_bit(TRANSPORT_WASTE) |
        transport_bit(TRANSPORT_COVERED) | transport_bit(TRANSPORT_OPEN));
    add_production_waste_lanes(building, laneCount);
}

static void add_heating_plant_lanes(u8* building, u8* templ, u32* laneCount) {
    if (!building || !templ || !laneCount) return;
    add_recipe_store_lanes(building, templ, ANNEX_INPUT,
                           TEMPLATE_CONSUME_BEGIN, TEMPLATE_CONSUME_END,
                           laneCount, general_supported_recipe_mask());
    add_production_waste_lanes(building, laneCount);
}

static void add_production_line_lanes(u8* building, u8* templ, u32* laneCount) {
    if (!building || !templ || !laneCount) return;
    // Vehicle/train/aircraft/drydock production lines use car-plant import roles
    // but expose normal covered/open resource records for their material inputs.
    add_recipe_store_lanes(building, templ, ANNEX_INPUT,
                           TEMPLATE_CONSUME_BEGIN, TEMPLATE_CONSUME_END,
                           laneCount, general_supported_recipe_mask());
    add_recipe_store_lanes(building, templ, ANNEX_OUTPUT,
                           TEMPLATE_PRODUCE_BEGIN, TEMPLATE_PRODUCE_END,
                           laneCount, general_supported_recipe_mask());
    add_production_waste_lanes(building, laneCount);
}

static void add_repair_material_lanes(u8* building, u32* laneCount) {
    if (!building || !laneCount) return;
    add_explicit_input_store_lanes(building, laneCount,
        transport_bit(TRANSPORT_COVERED) | transport_bit(TRANSPORT_OPEN));
}

static int fuel_service_type(i32 type) {
    // Some native service families use an oil store that does not carry the
    // ordinary STORE_ROLE_FUEL marker. These validated runtime classes are the
    // narrow fallback; role-driven discovery remains preferred for all others.
    return type == BUILDING_TYPE_GAS_STATION ||
           type == BUILDING_TYPE_DISTRIBUTION_OFFICE ||
           type == BUILDING_TYPE_RAIL_DISTRIBUTION_OFFICE ||
           type == BUILDING_TYPE_CONSTRUCTION_OFFICE ||
           type == BUILDING_TYPE_RAIL_CONSTRUCTION_OFFICE ||
           type == BUILDING_TYPE_TECHNICAL_SERVICES ||
           type == BUILDING_TYPE_REPAIR_OFFICE;
}

static int template_has_native_fuel_store(u8* templ) {
    if (!templ) return 0;
    u64 sb = 0; usize sc = 0;
    if (!template_store_vector(templ, &sb, &sc)) return 0;
    for (usize i = 0; i < sc; ++i) {
        u8* store = (u8*)(sb + i * STORE_STRIDE);
        if (read_i32(store + STORE_TRANSPORT) != TRANSPORT_OIL) continue;
        float cap = read_f32(store + STORE_CAPACITY);
        if (!finite_reasonable(cap) || cap <= 0.000001f) continue;
        if (read_i32(store + STORE_ROLE) == STORE_ROLE_FUEL) return 1;
    }
    return 0;
}

static void add_service_fuel_lanes(u8* building, u32* laneCount) {
    if (!building || !laneCount) return;
    u64 sb = 0;
    usize sc = 0;
    if (!building_store_vector(building, &sb, &sc)) return;
    for (usize i = 0; i < sc; ++i) {
        u8* store = (u8*)(sb + i * STORE_STRIDE);
        if (read_i32(store + STORE_TRANSPORT) != TRANSPORT_OIL || !store_has_pool_resources(store)) continue;
        float cap = read_f32(store + STORE_CAPACITY);
        if (!finite_reasonable(cap) || cap <= 0.000001f) continue;
        const i32 role = read_i32(store + STORE_ROLE);
        u8* templ = building_template(building);
        i32 type = -1;
        if (templ) template_building_type(templ, &type);
        // Prefer the actual $STORAGE_FUEL role. Some service-family definitions
        // encode resident fuel as a special oil store rather than role 11, so the
        // validated service-type fallback remains necessary.
        if (role != STORE_ROLE_FUEL && !fuel_service_type(type)) continue;
        add_host_lane(building, store, TRANSPORT_OIL, ANNEX_INPUT, laneCount);
    }
}

static int building_is_runtime_type(u8* building, i32 wantedType) {
    u8* templ = building_template(building);
    i32 type = -1;
    return templ && template_building_type(templ, &type) && type == wantedType;
}

// Map a live 0xE0 building storage descriptor back to its immutable template
// blueprint. Normal WRSR instances preserve descriptor order. If a dynamic
// family omits entries, accept only a UNIQUE same transport+role descriptor.
static int store_resource_identity_overlap(u8* a, u8* b, usize* outA, usize* outB, usize* outMatches) {
    if (!a || !b || !outA || !outB || !outMatches ||
        !readable(a, STORE_STRIDE) || !readable(b, STORE_STRIDE)) return 0;
    u64 ab = read_u64(a + STORE_RECORDS_BEGIN);
    u64 ae = read_u64(a + STORE_RECORDS_END);
    u64 bb = read_u64(b + STORE_RECORDS_BEGIN);
    u64 be = read_u64(b + STORE_RECORDS_END);
    usize ac = 0, bc = 0;
    if (!valid_vector(ab, ae, RECORD_STRIDE, MAX_POOL_RESOURCES, &ac) ||
        !valid_vector(bb, be, RECORD_STRIDE, MAX_POOL_RESOURCES, &bc)) return 0;
    usize matches = 0;
    for (usize i = 0; i < ac; ++i) {
        u8* res = (u8*)read_u64((u8*)ab + i * RECORD_STRIDE + RECORD_RESOURCE);
        if (!res) continue;
        for (usize j = 0; j < bc; ++j) {
            if (read_u64((u8*)bb + j * RECORD_STRIDE + RECORD_RESOURCE) == (u64)res) {
                ++matches;
                break;
            }
        }
    }
    *outA = ac;
    *outB = bc;
    *outMatches = matches;
    return 1;
}

// Map a live 0xE0 storage descriptor back to the immutable template blueprint.
// WRSR normally preserves descriptor order, but some runtime building families
// alter live role metadata. Prefer the same index when its semantic signature
// still matches; otherwise choose a unique best candidate from transport,
// direction flags and exact resource identity. Capacity is deliberately excluded
// from matching because the template is the authoritative authored value.
static u8* template_store_for_live_store(u8* building, u8* liveStore, i32 expectedTransport) {
    if (!building || !liveStore || !readable(liveStore, STORE_STRIDE)) return nullptr;
    u64 lb = 0;
    usize lc = 0;
    if (!building_store_vector(building, &lb, &lc)) return nullptr;
    usize liveIndex = lc;
    for (usize i = 0; i < lc; ++i) {
        if ((u8*)(lb + i * STORE_STRIDE) == liveStore) { liveIndex = i; break; }
    }
    if (liveIndex >= lc) return nullptr;

    u8* templ = building_template(building);
    u64 tb = 0;
    usize tc = 0;
    if (!templ || !template_store_vector(templ, &tb, &tc)) return nullptr;

    const i32 liveRole = read_i32(liveStore + STORE_ROLE);
    const u8 liveBlockOut = read_u8(liveStore + STORE_BLOCK_OUTPUT);
    const u8 liveBlockIn = read_u8(liveStore + STORE_BLOCK_INPUT);

    u8* best = nullptr;
    int bestScore = -1;
    int tied = 0;
    for (usize i = 0; i < tc; ++i) {
        u8* tstore = (u8*)(tb + i * STORE_STRIDE);
        if (!readable(tstore, STORE_STRIDE) || read_i32(tstore + STORE_TRANSPORT) != expectedTransport) continue;

        usize liveResources = 0, templateResources = 0, matches = 0;
        if (!store_resource_identity_overlap(liveStore, tstore, &liveResources, &templateResources, &matches)) continue;
        // Once a live descriptor exposes exact resource identities, a same-
        // transport blueprint with none of those resources cannot be that lane.
        if (liveResources != 0 && matches == 0) continue;

        int score = 0;
        if (i == liveIndex) score += 32;
        if (read_i32(tstore + STORE_ROLE) == liveRole) score += 16;
        if (read_u8(tstore + STORE_BLOCK_OUTPUT) == liveBlockOut) score += 8;
        if (read_u8(tstore + STORE_BLOCK_INPUT) == liveBlockIn) score += 8;
        if (liveResources == templateResources) score += 4;
        if (liveResources == 0 && templateResources == 0) score += 8;
        if (matches != 0) {
            score += (int)(matches * 16);
            if (matches == liveResources && matches == templateResources) score += 48;
        }

        if (score > bestScore) {
            best = tstore;
            bestScore = score;
            tied = 0;
        } else if (score == bestScore) {
            tied = 1;
        }
    }
    if (!best || tied) return nullptr;
    // A record-less descriptor needs at least index+direction/role evidence;
    // populated descriptors need resource evidence or the exact-index signature.
    if (bestScore < 32) return nullptr;
    return best;
}

// Read the authored/native capacity of a live host descriptor from its
// immutable template storage blueprint. Pooling may temporarily publish a larger
// live value during controlled simulation or transfer windows, but every window
// restores this authored capacity before normal/save-visible state resumes.
static int authored_capacity_for_live_store(u8* building, u8* liveStore,
                                            i32 expectedTransport, float* outCapacity) {
    if (!building || !liveStore || !outCapacity) return 0;
    u8* tstore = template_store_for_live_store(building, liveStore, expectedTransport);
    if (!tstore) return 0;
    float cap = read_f32(tstore + STORE_CAPACITY);
    if (!finite_reasonable(cap) || cap <= 0.01f || cap > 100000000.0f) return 0;
    *outCapacity = cap;
    return 1;
}

static int lane_annex_gap_sq(void* game, const HostLane* lane, const StorageCandidate* candidate, float* outSq) {
    if (!game || !lane || !lane->building || !candidate || !candidate->building || !outSq) return 0;
    float directSq = 0.0f;
    const int haveDirect = pair_footprint_gap_sq(game, lane->building, candidate->building, &directSq);
    if (haveDirect) {
        *outSq = directSq;
        if (directSq <= g_linkGapSq) return 1;
    }

    // A freight delivery building touching a storage that is already annexed to
    // this exact production lane must access the SAME pool. The storage is the
    // physical contact surface; it does not become a new propagating root. This
    // is the crucial storage<->delivery leg of the three-class model.
    if (candidate->cargoGateway && lane->rootClass == HOST_RESOURCE_ROOT) {
        float best = g_linkGapSq + 1.0f;
        int found = 0;
        for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
            AnnexState* st = &g_annexState[i];
            if (!st->active || st->host != lane->building || st->hostStore != lane->hostStore ||
                st->transport != lane->transport || st->mode != lane->mode ||
                !state_layout_valid(st) || building_is_runtime_type(st->storage, BUILDING_TYPE_CARGO_STATION)) continue;
            float sq = 0.0f;
            if (!pair_footprint_gap_sq(game, st->storage, candidate->building, &sq) || sq > g_linkGapSq) continue;
            if (!found || sq < best) { best = sq; found = 1; }
        }
        if (found) {
            *outSq = best;
            return 1;
        }
    }
    return haveDirect;
}

static int scan_world_candidates(void* game, u32* outLaneCount, u32* outStorageCount) {
    if (!game || !outLaneCount || !outStorageCount || !ensure_type_catalog(game)) return 0;
    u8* world = (u8*)game;
    if (!readable(world + GAME_BUILDINGS_BEGIN, 16)) return 0;
    u64 begin = read_u64(world + GAME_BUILDINGS_BEGIN);
    u64 end = read_u64(world + GAME_BUILDINGS_END);
    usize buildingCount = 0;
    if (!valid_vector(begin, end, sizeof(u64), 200000, &buildingCount)) {
        if (!g_invalidWorldLogged && g_host && g_host->log) {
            g_host->log("[EnhancedStorageManagement] invalid building-vector layout; storage-link scan skipped");
            g_invalidWorldLogged = 1;
        }
        return 0;
    }
    g_invalidWorldLogged = 0;

    u32 laneCount = 0, storageCount = 0;

    // Pass A discovers actual annex storage descriptors and mine outputs. Live
    // mines supplement the definition-derived mined-resource catalogue so load
    // ordering cannot disable a valid mine already present in the republic.
    for (usize i = 0; i < buildingCount; ++i) {
        u8* building = (u8*)read_u64((u8*)begin + i * sizeof(u64));
        if (!completed_building(building)) continue;
        u8* templ = building_template(building);
        i32 type = -1;
        if (!templ || !template_building_type(templ, &type)) continue;

        if (type == BUILDING_TYPE_STORAGE || type == BUILDING_TYPE_CARGO_STATION ||
            type == BUILDING_TYPE_WATER_PUMP || type == BUILDING_TYPE_TRASH_CONTAINER) {
            u32 mask = 0;
            int added = append_supported_storage_candidates(game, building, &storageCount, &mask);
            observe_storage_template(templ, mask);
            if (added < 0 && !g_overflowLogged && g_host && g_host->log) {
                g_host->log("[EnhancedStorageManagement] supported storage-descriptor candidate limit reached; additional stores ignored");
                g_overflowLogged = 1;
            }
        }
        if (type == BUILDING_TYPE_MINE) {
            add_live_mine_recipe_lanes(building, templ, &laneCount);
            add_production_waste_lanes(building, &laneCount);
        }
    }

    // Pass B derives consumer/processor lanes after every live mine has had the
    // opportunity to supplement mined-resource semantics.
    for (usize i = 0; i < buildingCount; ++i) {
        u8* building = (u8*)read_u64((u8*)begin + i * sizeof(u64));
        if (!completed_building(building)) continue;
        u8* templ = building_template(building);
        i32 type = -1;
        if (!templ || !template_building_type(templ, &type)) continue;
        // Use recipe-scoped root discovery for established production families. The catch-all directional-store scanner
        // turned hundreds of unrelated building stores into roots and filled the
        // 768-lane table, starving normal factories placed later in the world.
        if (type == BUILDING_TYPE_FACTORY) add_factory_semantic_lanes(building, templ, &laneCount);
        else if (type == BUILDING_TYPE_WATER_TREATMENT) add_water_treatment_lanes(building, templ, &laneCount);
        else if (type == BUILDING_TYPE_POWERPLANT) add_powerplant_lanes(building, templ, &laneCount);
        else if (type == BUILDING_TYPE_SHOP || type == BUILDING_TYPE_PUB) add_shop_pub_lanes(building, &laneCount);
        else if (type == BUILDING_TYPE_FARM) add_farm_lanes(building, templ, &laneCount);
        else if (type == BUILDING_TYPE_HEATING_PLANT) add_heating_plant_lanes(building, templ, &laneCount);
        else if (type == BUILDING_TYPE_PRODUCTION_LINE) add_production_line_lanes(building, templ, &laneCount);
        else if (type == BUILDING_TYPE_REPAIR_OFFICE) add_repair_material_lanes(building, &laneCount);

        // Fuel is a cross-cutting facility capability rather than a building
        // family.  Scan it independently so repair/maintenance buildings, end
        // stations, passenger heliports, airport terminals and cargo airports can
        // all be roots while retaining their normal primary type semantics.
        if (template_has_native_fuel_store(templ) || fuel_service_type(type))
            add_service_fuel_lanes(building, &laneCount);
    }

    // Third building class: freight delivery buildings may attach directly to
    // standalone storage. Represent each completed storage descriptor as a
    // delivery-only host lane. candidate_lane_compatible() forbids ordinary
    // storage annexes from targeting these lanes, so this does NOT create
    // storage-to-storage propagation. If the same store is annexed to a real
    // production root, gateway pairing migrates to that production group.
    for (u32 st = 0; st < storageCount; ++st) {
        StorageCandidate* candidate = &g_storages[st];
        if (!candidate->building || !candidate->store || candidate->cargoGateway || candidate->blueprintOnly) continue;
        u8* templ = building_template(candidate->building);
        i32 type = -1;
        if (!templ || !template_building_type(templ, &type)) continue;
        const int standaloneStorage = type == BUILDING_TYPE_STORAGE ||
            (type == BUILDING_TYPE_WATER_PUMP && template_engine_idle(templ));
        if (!standaloneStorage) continue;
        add_storage_delivery_host_lane(candidate->building, candidate->store, candidate->transport, &laneCount);
    }

    if (laneCount >= MAX_HOST_LANES && !g_overflowLogged && g_host && g_host->log) {
        g_host->log("[EnhancedStorageManagement] host-lane candidate limit reached; additional compatible lanes ignored");
        g_overflowLogged = 1;
    }
    *outLaneCount = laneCount;
    *outStorageCount = storageCount;
    return 1;
}


static HostLane* lane_for_state(AnnexState* s, u32 laneCount) {
    if (!s) return nullptr;
    for (u32 i = 0; i < laneCount; ++i) if (same_lane(&g_hostLanes[i], s)) return &g_hostLanes[i];
    return nullptr;
}

static int candidate_index_for_state(AnnexState* s, u32 storageCount) {
    if (!s) return -1;
    for (u32 i = 0; i < storageCount; ++i) {
        if (g_storages[i].building == s->storage && g_storages[i].store == s->annexStore) return (int)i;
    }
    return -1;
}

static int candidate_lane_structurally_compatible(void* game, StorageCandidate* candidate, HostLane* lane,
                                                  float* outGapSq) {
    if (outGapSq) *outGapSq = 0.0f;
    if (!game || !candidate || !lane || !candidate->building || !candidate->store) return 0;
    if (candidate->building == lane->building) return 0;
    if (lane->rootClass == HOST_STORAGE_DELIVERY_ONLY) {
        if (!candidate->cargoGateway) return 0;
        if (storage_host_store_is_active_annex(lane->building, lane->hostStore)) return 0;
    } else if ((candidate->modeMask & lane_mode_mask(lane->mode)) == 0) {
        return 0;
    }
    if (lane->transport != candidate->transport) return 0;
    // A freight gateway's live routing records are transient and change with cargo.
    // Transport class is the stable capability boundary; using current routing
    // contents here would make an established station relationship flap on/off.
    if (!candidate->cargoGateway && !lane_annex_store_overlap(lane, candidate->store)) return 0;
    float sq = 0.0f;
    if (!lane_annex_gap_sq(game, lane, candidate, &sq) || sq > g_linkGapSq) return 0;
    if (outGapSq) *outGapSq = sq;
    return 1;
}

static int candidate_lane_compatible(void* game, StorageCandidate* candidate, HostLane* lane,
                                     int trashContainer, int* outEvidence, float* outGapSq) {
    if (outEvidence) *outEvidence = 0;
    float sq = 0.0f;
    if (!candidate_lane_structurally_compatible(game, candidate, lane, &sq)) return 0;
    if (candidate->cargoGateway) {
        // A station/loader is an access port, not a resource selector. Its current
        // routing records are consequences of the connection and must never be a
        // prerequisite for creating or retaining that connection.
        if (outEvidence) *outEvidence = 1;
        if (outGapSq) *outGapSq = sq;
        return 1;
    }
    int evidence = 0;
    if (!annex_selection_matches_lane(lane, candidate->store, trashContainer, &evidence)) return 0;
    if (outEvidence) *outEvidence = evidence;
    if (outGapSq) *outGapSq = sq;
    return 1;
}

static u32 candidate_matches_on_root(void* game, StorageCandidate* candidate, u8* root,
                                     u32 laneCount, int trashContainer, int* outAnyEvidence,
                                     int* outFirstLane, int* outWaterOutputLane) {
    if (outAnyEvidence) *outAnyEvidence = 0;
    if (outFirstLane) *outFirstLane = -1;
    if (outWaterOutputLane) *outWaterOutputLane = -1;
    if (!game || !candidate || !root) return 0;
    u32 count = 0;
    for (u32 l = 0; l < laneCount; ++l) {
        HostLane* lane = &g_hostLanes[l];
        if (lane->building != root) continue;
        int evidence = 0;
        float sq = 0.0f;
        if (!candidate_lane_compatible(game, candidate, lane, trashContainer, &evidence, &sq)) continue;
        if (outFirstLane && *outFirstLane < 0) *outFirstLane = (int)l;
        if (outAnyEvidence && evidence) *outAnyEvidence = 1;
        if (outWaterOutputLane && lane->mode == ANNEX_OUTPUT) *outWaterOutputLane = (int)l;
        ++count;
    }
    return count;
}

static u32 candidate_structural_matches_on_root(void* game, StorageCandidate* candidate, u8* root,
                                                u32 laneCount) {
    if (!game || !candidate || !root) return 0;
    u32 count = 0;
    for (u32 i = 0; i < laneCount; ++i) {
        HostLane* lane = &g_hostLanes[i];
        if (lane->building != root) continue;
        if (candidate_lane_structurally_compatible(game, candidate, lane, nullptr)) ++count;
    }
    return count;
}

static int fixed_water_treatment_reservoir(StorageCandidate* candidate, i32 storageType, u8* templ, u8* root) {
    return candidate && root && storageType == BUILDING_TYPE_WATER_PUMP &&
           candidate->transport == TRANSPORT_WATER && template_engine_idle(templ) &&
           building_is_runtime_type(root, BUILDING_TYPE_WATER_TREATMENT);
}

static void process_world(void* game) {
    u32 laneCount = 0, storageCount = 0;
    if (!scan_world_candidates(game, &laneCount, &storageCount)) return;
    g_currentHostLaneCount = laneCount;
    g_currentStorageCount = storageCount;
    expire_annex_states(game, laneCount, storageCount);

    // An established spatial relationship remains stable while cargo moves.
    // Explicit player allocation can still retarget an ambiguous multi-resource
    // storage, but transient stock is discovery evidence only.
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active) continue;
        HostLane* lane = lane_for_state(s, laneCount);
        int ci = candidate_index_for_state(s, storageCount);
        if (!lane || ci < 0) continue;
        StorageCandidate* candidate = &g_storages[(u32)ci];
        u8* templ = building_template(candidate->building);
        i32 storageType = -1;
        if (!templ || !template_building_type(templ, &storageType)) {
            detach_state(game, s, nullptr);
            continue;
        }
        if (!candidate_lane_structurally_compatible(game, candidate, lane, nullptr)) {
            detach_state(game, s, nullptr);
            continue;
        }

        const int trashContainer = storageType == BUILDING_TYPE_TRASH_CONTAINER;
        if (!trashContainer && !candidate->cargoGateway) {
            int hasExplicit = 0;
            if (!explicit_allocation_matches_lane(lane, candidate->store, &hasExplicit)) {
                detach_state(game, s, nullptr);
                continue;
            }
            if (hasExplicit) {
                s->explicitSelection = 1;
            } else if (s->explicitSelection &&
                       candidate_structural_matches_on_root(game, candidate, s->host, laneCount) > 1) {
                detach_state(game, s, nullptr);
                continue;
            }
        }
        apply_pool_pair(game, lane, candidate);
    }

    // Process real storage annexes BEFORE freight gateways. This ordering lets a
    // storage join its production/resource root first, after which a loader,
    // unloader or freight station touching that storage resolves through the
    // storage contact surface to the same pool. The second pass handles gateways.
    for (int gatewayPass = 0; gatewayPass < 2; ++gatewayPass) {
      for (u32 st = 0; st < storageCount; ++st) {
        StorageCandidate* candidate = &g_storages[st];
        if ((gatewayPass == 0 && candidate->cargoGateway) ||
            (gatewayPass == 1 && !candidate->cargoGateway)) continue;
        if (candidate->blueprintOnly) {
            // Template descriptors classify specialised cargo gateways for the
            // placement classifier, but they are immutable definition data and are
            // never used as a live storage object. A dedicated vehicle-transfer
            // bridge is required for stations whose routing store never materialises.
            if (!g_gatewayBlueprintOnlyLogged && g_host && g_host->log) {
                g_host->log("[EnhancedStorageManagement] specialised freight gateway has template routing metadata but no live routing descriptor; placement classification remains available while transfer integration stays inactive");
                g_gatewayBlueprintOnlyLogged = 1;
            }
            continue;
        }
        u8* templ = building_template(candidate->building);
        i32 storageType = -1;
        if (!templ || !template_building_type(templ, &storageType)) continue;
        int trashContainer = storageType == BUILDING_TYPE_TRASH_CONTAINER;

        u8* targetRoot = active_pool_root_for_member(candidate->building);
        float bestSq = g_linkGapSq + 1.0f;
        if (!targetRoot) {
            for (u32 l = 0; l < laneCount; ++l) {
                HostLane* lane = &g_hostLanes[l];
                int evidence = 0;
                float sq = 0.0f;
                if (!candidate_lane_compatible(game, candidate, lane, trashContainer, &evidence, &sq)) continue;
                if (!targetRoot || sq < bestSq) {
                    targetRoot = lane->building;
                    bestSq = sq;
                }
            }
        }
        if (!targetRoot) continue;

        int anyEvidence = 0;
        int firstLane = -1;
        int waterOutputLane = -1;
        u32 matchCount = candidate_matches_on_root(game, candidate, targetRoot, laneCount,
                                                   trashContainer, &anyEvidence,
                                                   &firstLane, &waterOutputLane);
        if (matchCount == 0 || firstLane < 0) continue;

        const int fixedWater = fixed_water_treatment_reservoir(candidate, storageType, templ, targetRoot);
        if (candidate->cargoGateway && matchCount > 1) {
            if (g_hostLanes[(u32)firstLane].rootClass == HOST_STORAGE_DELIVERY_ONLY) {
                // A standalone storage descriptor is one physical stock pool. Do
                // not create opposite-mode duplicate groups on the same store;
                // the canonical storage-delivery lane is bidirectional through
                // drain-before-sim + refill-after-sim gateway staging.
                apply_pool_pair(game, &g_hostLanes[(u32)firstLane], candidate);
            } else {
                // A cargo station is an access gateway, not a player-configured
                // warehouse. General cargo terminals are intentionally capable of
                // serving several resource lanes on the same production building
                // (e.g. crops in + food out). Bind every direction/resource lane
                // that its native live descriptor can actually represent.
                for (u32 l = 0; l < laneCount; ++l) {
                    HostLane* lane = &g_hostLanes[l];
                    if (lane->building != targetRoot || lane->rootClass != HOST_RESOURCE_ROOT) continue;
                    int evidence = 0; float sq = 0.0f;
                    if (!candidate_lane_compatible(game, candidate, lane, trashContainer, &evidence, &sq)) continue;
                    apply_pool_pair(game, lane, candidate);
                }
            }
            continue;
        }
        if (!trashContainer && !anyEvidence && matchCount > 1) {
            if (fixedWater && waterOutputLane >= 0) {
                apply_pool_pair(game, &g_hostLanes[(u32)waterOutputLane], candidate);
            } else {
                if (!g_ambiguousSelectionLogged && g_host && g_host->log) {
                    g_host->log("[EnhancedStorageManagement] compatible unallocated descriptor %p/store %p (%s) matches %u lanes on one root; waiting for native resource allocation/cargo instead of choosing arbitrarily",
                                candidate->building, candidate->store,
                                transport_label(candidate->transport), matchCount);
                    g_ambiguousSelectionLogged = 1;
                }
            }
            continue;
        }

        if (!anyEvidence) {
            // A fixed/single-purpose descriptor or a multi-resource descriptor
            // with only one compatible root lane needs no user disambiguation.
            apply_pool_pair(game, &g_hostLanes[(u32)firstLane], candidate);
            continue;
        }

        // Explicit native allocation/cargo may intentionally split one physical
        // descriptor across several host lanes. Apply every matching slice; the
        // per-resource allocation weights partition its physical capacity.
        for (u32 l = 0; l < laneCount; ++l) {
            HostLane* lane = &g_hostLanes[l];
            if (lane->building != targetRoot) continue;
            int evidence = 0;
            float sq = 0.0f;
            if (!candidate_lane_compatible(game, candidate, lane, trashContainer, &evidence, &sq) || !evidence) continue;
            apply_pool_pair(game, lane, candidate);
        }
      }
    }
}

static AnnexState* active_state_for_host_store(u8* store) {
    if (!store) return nullptr;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (s->active && s->hostStore == store && state_layout_valid(s)) return s;
    }
    return nullptr;
}

static float state_resource_capacity_for_host(AnnexState* s, u8* resource) {
    if (!s || !resource || !state_layout_valid(s) || !state_accepts_resource(s, resource)) return 0.0f;
    if (building_is_runtime_type(s->storage, BUILDING_TYPE_CARGO_STATION)) return 0.0f;
    if (!find_resource_record(s->annexStore, resource)) return 0.0f;
    if (building_is_runtime_type(s->storage, BUILDING_TYPE_TRASH_CONTAINER)) return s->annexCapacity;
    if (!store_has_explicit_positive_allocation(s->annexStore)) return s->annexCapacity;
    int active = 0;
    float weight = store_policy_weight(s->annexStore, resource, &active);
    if (weight < 0.0f || !active || weight <= 0.000001f) return 0.0f;
    return s->annexCapacity * clamp_f(weight, 0.0f, 1.0f);
}

static float group_resource_capacity_for_mechanics(u8* hostStore, u8* resource) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || !resource) return 0.0f;
    float total = resource_policy_max_units(hostStore, first->originalHostCapacity, resource);
    if (!finite_reasonable(total) || total < 0.0f) return 0.0f;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* s = &g_annexState[i];
        if (!s->active || s->hostStore != hostStore || !state_layout_valid(s)) continue;
        float add = state_resource_capacity_for_host(s, resource);
        if (!finite_reasonable(add) || add < 0.0f) continue;
        total += add;
    }
    return finite_reasonable(total) ? total : 0.0f;
}


struct TransferPoolSelection {
    u8* root;
    u8* hostStore;
    i32 transport;
    AnnexMode mode;
};

static float group_total_used_for_transfer(u8* hostStore) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || !state_layout_valid(first)) return -1.0f;
    float total = store_total_used_units(hostStore, first->transport);
    if (total < 0.0f) return -1.0f;
    u8* seen[MAX_POOL_RESOURCES] = {};
    u32 seenCount = 0;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* st = &g_annexState[i];
        if (!st->active || st->hostStore != hostStore || !state_layout_valid(st) || !st->annexStore) continue;
        int duplicate = 0;
        for (u32 j = 0; j < seenCount; ++j) if (seen[j] == st->annexStore) { duplicate = 1; break; }
        if (duplicate) continue;
        if (seenCount < MAX_POOL_RESOURCES) seen[seenCount++] = st->annexStore;
        float used = store_total_used_units(st->annexStore, st->transport);
        if (used < 0.0f) return -1.0f;
        total += used;
    }
    return finite_reasonable(total) ? total : -1.0f;
}

static float group_resource_used_for_transfer(u8* hostStore, u8* resource) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || !resource || !state_layout_valid(first)) return -1.0f;
    float total = resource_units_in_store(hostStore, resource, first->transport);
    if (!finite_reasonable(total) || total < -0.001f) return -1.0f;
    u8* seen[MAX_POOL_RESOURCES] = {};
    u32 seenCount = 0;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* st = &g_annexState[i];
        if (!st->active || st->hostStore != hostStore || !state_layout_valid(st) ||
            !st->annexStore || !state_accepts_resource(st, resource)) continue;
        int duplicate = 0;
        for (u32 j = 0; j < seenCount; ++j) if (seen[j] == st->annexStore) { duplicate = 1; break; }
        if (duplicate) continue;
        if (seenCount < MAX_POOL_RESOURCES) seen[seenCount++] = st->annexStore;
        float used = resource_units_in_store(st->annexStore, resource, st->transport);
        if (!finite_reasonable(used) || used < -0.001f) return -1.0f;
        total += used;
    }
    return finite_reasonable(total) ? total : -1.0f;
}

static float group_resource_free_for_transfer(u8* hostStore, u8* resource) {
    const float logicalCapacity = group_pooled_capacity(hostStore);
    const float totalUsed = group_total_used_for_transfer(hostStore);
    const float resourceCapacity = group_resource_capacity_for_mechanics(hostStore, resource);
    const float resourceUsed = group_resource_used_for_transfer(hostStore, resource);
    if (!finite_reasonable(logicalCapacity) || !finite_reasonable(totalUsed) ||
        !finite_reasonable(resourceCapacity) || !finite_reasonable(resourceUsed) ||
        logicalCapacity <= 0.0f || totalUsed < 0.0f || resourceCapacity < 0.0f || resourceUsed < 0.0f)
        return 0.0f;
    float totalFree = logicalCapacity - totalUsed;
    float resourceFree = resourceCapacity - resourceUsed;
    if (totalFree < 0.0f) totalFree = 0.0f;
    if (resourceFree < 0.0f) resourceFree = 0.0f;
    return resourceFree < totalFree ? resourceFree : totalFree;
}

static int drain_delivery_resource_into_storage_pool(void* game, u8* hostStore, u8* resource) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || !resource || !state_layout_valid(first)) return 0;
    const float nativeCap = first->originalHostCapacity;
    if (!finite_reasonable(nativeCap) || nativeCap <= 0.0f) return 0;

    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* gateway = &g_annexState[i];
        if (!gateway->active || gateway->hostStore != hostStore || !state_layout_valid(gateway) ||
            !building_is_runtime_type(gateway->storage, BUILDING_TYPE_CARGO_STATION) ||
            !state_accepts_resource(gateway, resource)) continue;
        float remaining = resource_units_in_store(gateway->annexStore, resource, gateway->transport);
        if (!finite_reasonable(remaining) || remaining <= 0.000001f) continue;

        float moved = move_resource_units(game, gateway->storage, gateway->annexStore,
                                          gateway->host, hostStore, gateway->transport,
                                          resource, remaining, nativeCap);
        remaining -= moved;
        if (remaining <= 0.000001f) continue;

        u8* seen[MAX_POOL_RESOURCES] = {};
        u32 seenCount = 0;
        for (u32 j = 0; j < MAX_ANNEX_STATES && remaining > 0.000001f; ++j) {
            AnnexState* target = &g_annexState[j];
            if (!target->active || target->hostStore != hostStore || !state_layout_valid(target) ||
                target->annexStore == gateway->annexStore ||
                building_is_runtime_type(target->storage, BUILDING_TYPE_CARGO_STATION) ||
                !state_accepts_resource(target, resource) || !annex_resource_allowed(target->annexStore, resource)) continue;
            int duplicate = 0;
            for (u32 k = 0; k < seenCount; ++k) if (seen[k] == target->annexStore) { duplicate = 1; break; }
            if (duplicate) continue;
            if (seenCount < MAX_POOL_RESOURCES) seen[seenCount++] = target->annexStore;
            moved = move_resource_units(game, gateway->storage, gateway->annexStore,
                                        target->storage, target->annexStore, target->transport,
                                        resource, remaining, target->annexCapacity);
            remaining -= moved;
        }
    }
    return 1;
}

static int stage_resource_pool_into_host_for_transfer(void* game, u8* hostStore, u8* resource) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || !resource || !state_layout_valid(first)) return 0;
    u8* hostRec = find_resource_record(hostStore, resource);
    const float coeff = resource_coefficient(resource, first->transport);
    if (!hostRec || coeff <= 0.0f) return 0;

    u8* seen[MAX_POOL_RESOURCES] = {};
    u32 seenCount = 0;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* st = &g_annexState[i];
        if (!st->active || st->hostStore != hostStore || !state_layout_valid(st) ||
            !st->annexStore || !state_accepts_resource(st, resource)) continue;
        int duplicate = 0;
        for (u32 j = 0; j < seenCount; ++j) if (seen[j] == st->annexStore) { duplicate = 1; break; }
        if (duplicate) continue;
        if (seenCount < MAX_POOL_RESOURCES) seen[seenCount++] = st->annexStore;
        u8* fromRec = find_resource_record(st->annexStore, resource);
        if (!fromRec) continue;
        float fromAmount = read_f32(fromRec + RECORD_CURRENT);
        float hostAmount = read_f32(hostRec + RECORD_CURRENT);
        if (!finite_reasonable(fromAmount) || !finite_reasonable(hostAmount) || fromAmount <= 0.0f) continue;
        write_f32(fromRec + RECORD_CURRENT, 0.0f);
        write_f32(hostRec + RECORD_CURRENT, hostAmount + fromAmount);
        touch_storage_visual(game, st->storage, st->annexStore, st->transport, resource);
        touch_storage_visual(game, first->host, hostStore, first->transport, resource);
    }
    return 1;
}

static int select_transfer_pool(u8* member, u8* resource, float amount, TransferPoolSelection* out) {
    if (!member || !resource || !out || amount == 0.0f) return 0;
    const AnnexMode preferred = amount > 0.0f ? ANNEX_INPUT : ANNEX_OUTPUT;
    int bestScore = -1;
    int ambiguous = 0;
    TransferPoolSelection best = {};
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* st = &g_annexState[i];
        if (!st->active || (st->host != member && st->storage != member)) continue;
        if (!state_layout_valid(st) || !state_accepts_resource(st, resource)) continue;
        int score = st->mode == preferred ? 8 : 0;
        if (st->host == member) score += 4;
        if (st->storage == member) score += 2;
        if (st->storage == member && building_is_runtime_type(member, BUILDING_TYPE_CARGO_STATION)) score += 2;
        if (score > bestScore) {
            bestScore = score;
            best.root = st->host;
            best.hostStore = st->hostStore;
            best.transport = st->transport;
            best.mode = st->mode;
            ambiguous = 0;
        } else if (score == bestScore && best.hostStore != st->hostStore) {
            ambiguous = 1;
        }
    }
    if (bestScore < 0 || !best.root || !best.hostStore || ambiguous) {
        if (ambiguous && !g_transferAmbiguousLogged && g_host && g_host->log) {
            g_host->log("[EnhancedStorageManagement] unified transfer bridge found multiple equally valid pool lanes for one building/resource; native transfer semantics preserved rather than choosing a store arbitrarily");
            g_transferAmbiguousLogged = 1;
        }
        return 0;
    }
    *out = best;
    return 1;
}

static int prepare_group_for_transfer(void* game, const TransferPoolSelection& selection,
                                      u8* resource, float amount, float* outShadow) {
    if (!game || !selection.hostStore || !resource || !outShadow) return 0;
    AnnexState* first = first_state_for_host_store(selection.hostStore);
    if (!first || !state_layout_valid(first)) return 0;
    float pooled = group_pooled_capacity(selection.hostStore);
    const float nativeCap = first->originalHostCapacity;
    float live = 0.0f;
    if (!finite_reasonable(pooled) || pooled <= 0.0f ||
        !group_capacity_owned(selection.hostStore, nativeCap, pooled, &live)) return 0;

    if (!rebalance_group(game, selection.hostStore)) return 0;
    if (!drain_delivery_resource_into_storage_pool(game, selection.hostStore, resource)) return 0;
    pooled = group_pooled_capacity(selection.hostStore);
    if (!finite_reasonable(pooled) || pooled <= 0.0f) return 0;

    float shadow = pooled;
    if (amount < 0.0f) {
        write_f32(selection.hostStore + STORE_CAPACITY, pooled);
        update_group_capacity_metadata(selection.hostStore, pooled, pooled);
        if (!stage_resource_pool_into_host_for_transfer(game, selection.hostStore, resource)) {
            write_f32(selection.hostStore + STORE_CAPACITY, nativeCap);
            update_group_capacity_metadata(selection.hostStore, pooled, nativeCap);
            return 0;
        }
        float hostUsed = store_total_used_units(selection.hostStore, first->transport);
        if (!finite_reasonable(hostUsed) || hostUsed < 0.0f) {
            write_f32(selection.hostStore + STORE_CAPACITY, nativeCap);
            update_group_capacity_metadata(selection.hostStore, pooled, nativeCap);
            return 0;
        }
        if (hostUsed > shadow) shadow = hostUsed;
    } else {
        // Gather this resource into the canonical root before WRSR evaluates its
        // native per-resource allocation. Without this, stock already sitting in
        // an annex is invisible to the root selector and the transient capacity
        // could over-admit that resource even though the collective allocation is full.
        const float logicalFree = group_resource_free_for_transfer(selection.hostStore, resource);
        const float totalUsedBefore = group_total_used_for_transfer(selection.hostStore);
        if (!finite_reasonable(logicalFree) || !finite_reasonable(totalUsedBefore) ||
            logicalFree < 0.0f || totalUsedBefore < 0.0f) return 0;
        float totalFree = pooled - totalUsedBefore;
        if (totalFree < 0.0f) totalFree = 0.0f;

        write_f32(selection.hostStore + STORE_CAPACITY, pooled);
        update_group_capacity_metadata(selection.hostStore, pooled, pooled);
        if (!stage_resource_pool_into_host_for_transfer(game, selection.hostStore, resource)) {
            write_f32(selection.hostStore + STORE_CAPACITY, nativeCap);
            update_group_capacity_metadata(selection.hostStore, pooled, nativeCap);
            return 0;
        }

        const float hostUsed = store_total_used_units(selection.hostStore, first->transport);
        const float hostResource = resource_units_in_store(selection.hostStore, resource, first->transport);
        if (!finite_reasonable(hostUsed) || !finite_reasonable(hostResource) ||
            hostUsed < 0.0f || hostResource < 0.0f) {
            write_f32(selection.hostStore + STORE_CAPACITY, nativeCap);
            update_group_capacity_metadata(selection.hostStore, pooled, nativeCap);
            return 0;
        }
        shadow = hostUsed + totalFree;
        int active = 0;
        const float weight = store_policy_weight(selection.hostStore, resource, &active);
        if (weight > 0.000001f && active) {
            float resourceShadow = (hostResource + logicalFree) / weight;
            if (finite_reasonable(resourceShadow) && resourceShadow >= hostUsed && resourceShadow < shadow)
                shadow = resourceShadow;
        }
        if (shadow > pooled && hostUsed <= pooled) shadow = pooled;
        if (shadow < hostUsed) shadow = hostUsed;
    }
    if (!finite_reasonable(shadow) || shadow <= 0.0f || shadow > 100000000.0f) return 0;
    write_f32(selection.hostStore + STORE_CAPACITY, shadow);
    update_group_capacity_metadata(selection.hostStore, pooled, shadow);
    *outShadow = shadow;
    return 1;
}

static void publish_group_after_transfer(void* game, const TransferPoolSelection& selection, u8* resource) {
    if (!selection.hostStore) return;
    if (resource) drain_delivery_resource_into_storage_pool(game, selection.hostStore, resource);
    publish_group_after_simulation(game, selection.hostStore);
}

static int group_has_real_capacity_annex(u8* hostStore) {
    if (!hostStore) return 0;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* st = &g_annexState[i];
        if (!st->active || st->hostStore != hostStore || !state_layout_valid(st) ||
            building_is_runtime_type(st->storage, BUILDING_TYPE_CARGO_STATION)) continue;
        float cap = annex_effective_capacity_for_host(st);
        if (finite_reasonable(cap) && cap > 0.00001f) return 1;
    }
    return 0;
}

static int transfer_needs_pool_window(const TransferPoolSelection& selection,
                                      u8* member, u8* resource, float amount) {
    if (!selection.hostStore || !resource || !member) return 0;
    if (member != selection.root) return 1;
    AnnexState* first = first_state_for_host_store(selection.hostStore);
    if (!first || !state_layout_valid(first)) return 0;
    if (amount > 0.0f) {
        float free = resource_free_units(selection.hostStore, first->originalHostCapacity,
                                         first->transport, resource);
        return !finite_reasonable(free) || free + 0.00001f < amount;
    }
    float available = resource_units_in_store(selection.hostStore, resource, first->transport);
    return !finite_reasonable(available) || available + 0.00001f < -amount;
}

static int group_is_storage_delivery_only(const AnnexState* first) {
    if (!first || !first->host || !first->hostStore) return 0;
    for (u32 i = 0; i < g_currentHostLaneCount; ++i) {
        const HostLane* lane = &g_hostLanes[i];
        if (lane->building == first->host && lane->hostStore == first->hostStore &&
            lane->rootClass == HOST_STORAGE_DELIVERY_ONLY) return 1;
    }
    // Fail-safe fallback if the world snapshot rotated between simulation and UI.
    return host_is_storage_delivery_root(first->host);
}

static float display_state_resource_capacity(AnnexState* s, u8* resource) {
    if (!s || building_is_runtime_type(s->storage, BUILDING_TYPE_CARGO_STATION)) return 0.0f;
    return state_resource_capacity_for_host(s, resource);
}

static int group_has_display_storage_annex(u8* hostStore) {
    if (!hostStore) return 0;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* st = &g_annexState[i];
        if (!st->active || st->hostStore != hostStore || !state_layout_valid(st)) continue;
        if (!building_is_runtime_type(st->storage, BUILDING_TYPE_CARGO_STATION)) return 1;
    }
    return 0;
}

static float group_display_pooled_capacity(u8* hostStore) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first) return 0.0f;
    float pooled = first->originalHostCapacity;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* st = &g_annexState[i];
        if (!st->active || st->hostStore != hostStore || !state_layout_valid(st) ||
            building_is_runtime_type(st->storage, BUILDING_TYPE_CARGO_STATION)) continue;
        float contribution = annex_effective_capacity_for_host(st);
        if (!finite_reasonable(contribution) || contribution < 0.0f) return 0.0f;
        pooled += contribution;
    }
    return finite_reasonable(pooled) && pooled > 0.0f ? pooled : 0.0f;
}

static float group_display_resource_capacity(u8* hostStore, u8* resource) {
    AnnexState* first = first_state_for_host_store(hostStore);
    if (!first || !resource) return 0.0f;
    float total = resource_policy_max_units(hostStore, first->originalHostCapacity, resource);
    if (!finite_reasonable(total) || total < 0.0f) return 0.0f;
    for (u32 i = 0; i < MAX_ANNEX_STATES; ++i) {
        AnnexState* st = &g_annexState[i];
        if (!st->active || st->hostStore != hostStore || !state_layout_valid(st)) continue;
        float add = display_state_resource_capacity(st, resource);
        if (!finite_reasonable(add) || add < 0.0f) continue;
        total += add;
    }
    return finite_reasonable(total) ? total : 0.0f;
}



static int append_temporary_connection_result(i64 game, u8* target) {
    if (!game || !target || !g_vectorPushBackU64 || !completed_building(target)) return 0;
    u8* base = (u8*)game;
    if (!readable(base + GAME_TEMP_CONNECTIONS_BEGIN,
                  GAME_TEMP_CONNECTIONS_CAP - GAME_TEMP_CONNECTIONS_BEGIN + sizeof(u64))) return 0;
    u64 begin = read_u64(base + GAME_TEMP_CONNECTIONS_BEGIN);
    u64 end = read_u64(base + GAME_TEMP_CONNECTIONS_END);
    if (end < begin || ((end - begin) & 7ull) != 0 ||
        (end - begin) / 8ull > 1024ull) return 0;
    if (begin && end > begin && !readable((const void*)begin, (usize)(end - begin))) return 0;
    for (u64 at = begin; at < end; at += 8) {
        if (read_u64((const u8*)at) == (u64)target) return 1;
    }
    const u64 value = (u64)target;
    g_vectorPushBackU64((i64*)(base + GAME_TEMP_CONNECTIONS_BEGIN), &value);
    begin = read_u64(base + GAME_TEMP_CONNECTIONS_BEGIN);
    end = read_u64(base + GAME_TEMP_CONNECTIONS_END);
    if (end < begin || ((end - begin) & 7ull) != 0 ||
        (end - begin) / 8ull > 1024ull || end < begin + 8 ||
        !readable((const void*)(end - 8), sizeof(u64))) return 0;
    return read_u64((const u8*)(end - 8)) == value;
}

// Native FUN_1407CDC40 invokes FUN_140422C30 with exactly (-1, mode=2, flag=0)
// before deciding whether to show the cargo-station "no connection" warning.
// Extend only that temporary read-only query. Persistent station+0xD38 native
// factory-connection entries are deliberately untouched because WRSR simulation
// consumes them and they participate in object/save lifetime.
static void __fastcall factory_connection_query_detour(i64 game, u64 building, int filterType,
                                                        u64 param4, u64 param5, int queryMode,
                                                        char directionFlag) {
    if (!g_originalFactoryConnectionQuery) return;
    if (g_connectionQueryReentrant) {
        g_originalFactoryConnectionQuery(game, building, filterType, param4, param5,
                                         queryMode, directionFlag);
        return;
    }
    g_connectionQueryReentrant = 1;
    g_originalFactoryConnectionQuery(game, building, filterType, param4, param5,
                                     queryMode, directionFlag);

    if (filterType == -1 && queryMode == 2 && directionFlag == '\0' && building) {
        u8* delivery = (u8*)building;
        if (completed_building(delivery) &&
            building_is_runtime_type(delivery, BUILDING_TYPE_CARGO_STATION)) {
            u8* roots[8] = {};
            u32 rootCount = collect_unique_active_pool_roots_for_member(delivery, roots, 8);
            u32 bounded = rootCount < 8 ? rootCount : 8;
            for (u32 i = 0; i < bounded; ++i) {
                if (roots[i]) append_temporary_connection_result(game, roots[i]);
            }
            if (rootCount > 8 && !g_freightConnectionUiAmbiguousLogged && g_host && g_host->log) {
                g_host->log("[EnhancedStorageManagement] freight UI bridge found %u distinct virtual pool roots for one delivery building; exposing the first 8 temporary UI results and leaving persistent native connections untouched",
                            rootCount);
                g_freightConnectionUiAmbiguousLogged = 1;
            }
        }
    }
    g_connectionQueryReentrant = 0;
}


static u64 __fastcall resource_transfer_detour(i64 game, u64 building, int transferMode, i64 resourceRaw,
                                               float amount, float* residual, float param7,
                                               char exactBuilding, i64 param9) {
    if (!g_originalResourceTransfer) return 0;
    if (g_transferReentrant || g_simulationWindowActive || transferMode != 0 ||
        building == 0 || resourceRaw == 0 ||
        !finite_reasonable(amount) || abs_f(amount) <= 0.000001f) {
        return g_originalResourceTransfer(game, building, transferMode, resourceRaw, amount,
                                          residual, param7, exactBuilding, param9);
    }

    TransferPoolSelection selection = {};
    u8* member = (u8*)building;
    u8* resource = (u8*)resourceRaw;
    if (!select_transfer_pool(member, resource, amount, &selection) ||
        !group_has_real_capacity_annex(selection.hostStore) ||
        !transfer_needs_pool_window(selection, member, resource, amount)) {
        return g_originalResourceTransfer(game, building, transferMode, resourceRaw, amount,
                                          residual, param7, exactBuilding, param9);
    }

    g_transferReentrant = 1;
    float shadow = 0.0f;
    if (!prepare_group_for_transfer((void*)game, selection, resource, amount, &shadow)) {
        g_transferReentrant = 0;
        return g_originalResourceTransfer(game, building, transferMode, resourceRaw, amount,
                                          residual, param7, exactBuilding, param9);
    }

    const u64 effectiveBuilding = (u64)selection.root;

    u64 result = g_originalResourceTransfer(game, effectiveBuilding, transferMode, resourceRaw, amount,
                                            residual, param7, exactBuilding, param9);
    publish_group_after_transfer((void*)game, selection, resource);
    g_transferReentrant = 0;
    return result;
}

static void __fastcall generic_storage_ui_detour(i64 param1, i64* param2, i64* param3,
                                                  float* param4, float* param5, int param6,
                                                  char param7, float param8, u32 param9,
                                                  int param10, u32 param11, float param12,
                                                  float param13, float param14, char param15,
                                                  char param16) {
    if (!g_originalGenericStorageUi) return;
    if (g_uiReentrant) {
        g_originalGenericStorageUi(param1,param2,param3,param4,param5,param6,param7,param8,param9,
                                   param10,param11,param12,param13,param14,param15,param16);
        return;
    }
    g_uiReentrant = 1;

    AnnexState* first = active_state_for_host_store((u8*)param3);
    i64* uiStore = param3;
    alignas(16) u8 shadowStore[STORE_STRIDE] = {};
    alignas(16) u8 shadowRecords[RECORD_STRIDE * MAX_UI_SHADOW_ENTRIES] = {};
    alignas(16) u8 shadowAlloc[ALLOC_STRIDE * MAX_UI_SHADOW_ENTRIES] = {};

    const int deliveryOnlyRoot = group_is_storage_delivery_only(first);
    const int hasPhysicalDisplayAnnex = first ? group_has_display_storage_annex(first->hostStore) : 0;
    if (first && !deliveryOnlyRoot && hasPhysicalDisplayAnnex && readable(first->hostStore, STORE_STRIDE)) {
        u64 recBegin = read_u64(first->hostStore + STORE_RECORDS_BEGIN);
        u64 recEnd = read_u64(first->hostStore + STORE_RECORDS_END);
        usize recCount = 0;
        u64 allocBegin = read_u64(first->hostStore + STORE_ALLOC_BEGIN);
        u64 allocEnd = read_u64(first->hostStore + STORE_ALLOC_END);
        usize allocCount = 0;
        if (valid_vector(recBegin, recEnd, RECORD_STRIDE, MAX_UI_SHADOW_ENTRIES, &recCount) &&
            valid_vector(allocBegin, allocEnd, ALLOC_STRIDE, MAX_UI_SHADOW_ENTRIES, &allocCount)) {
            memcpy(shadowStore, first->hostStore, STORE_STRIDE);
            if (recCount) memcpy(shadowRecords, (void*)recBegin, recCount * RECORD_STRIDE);
            if (allocCount) memcpy(shadowAlloc, (void*)allocBegin, allocCount * ALLOC_STRIDE);
            *(u64*)(shadowStore + STORE_RECORDS_BEGIN) = (u64)shadowRecords;
            *(u64*)(shadowStore + STORE_RECORDS_END) = (u64)(shadowRecords + recCount * RECORD_STRIDE);
            *(u64*)(shadowStore + STORE_ALLOC_BEGIN) = (u64)shadowAlloc;
            *(u64*)(shadowStore + STORE_ALLOC_END) = (u64)(shadowAlloc + allocCount * ALLOC_STRIDE);
            float pooled = group_display_pooled_capacity(first->hostStore);
            if (pooled > 0.0f) write_f32(shadowStore + STORE_CAPACITY, pooled);

            // Aggregate every physical annex slice attached to this host lane.
            // The shadow allocation vector is also rewritten PER RESOURCE so the
            // central UI displays the real native+annex limit after a Workshop
            // warehouse has been split by percentages. Only the shadow copy is
            // modified; the factory and annex native selectors remain untouched.
            usize pairedCount = recCount < allocCount ? recCount : allocCount;
            for (usize i = 0; i < recCount; ++i) {
                u8* rec = shadowRecords + i * RECORD_STRIDE;
                u8* resource = (u8*)read_u64(rec + RECORD_RESOURCE);
                if (!resource || resource_coefficient(resource, first->transport) <= 0.0f) continue;
                float total = read_f32(rec + RECORD_CURRENT);
                if (!finite_reasonable(total)) continue;
                for (u32 a = 0; a < MAX_ANNEX_STATES; ++a) {
                    AnnexState* s = &g_annexState[a];
                    if (!s->active || s->hostStore != first->hostStore || !state_layout_valid(s) ||
                        building_is_runtime_type(s->storage, BUILDING_TYPE_CARGO_STATION) ||
                        display_state_resource_capacity(s, resource) <= 0.000001f) continue;
                    u8* annexRec = find_resource_record(s->annexStore, resource);
                    if (!annexRec) continue;
                    float amount = read_f32(annexRec + RECORD_CURRENT);
                    if (finite_reasonable(amount) && amount > 0.0f) total += amount;
                }
                write_f32(rec + RECORD_CURRENT, total);

                // Waste selectors belong to the production building. An annex
                // may add compatible capacity, but its selected container type
                // must never be projected back into the root's allocation vector.
                if (first->transport != TRANSPORT_WASTE && i < pairedCount && pooled > 0.000001f) {
                    float resourceCap = group_display_resource_capacity(first->hostStore, resource);
                    if (finite_reasonable(resourceCap) && resourceCap >= 0.0f) {
                        u8* alloc = shadowAlloc + i * ALLOC_STRIDE;
                        write_f32(alloc + ALLOC_ACTIVE, 1.0f);
                        write_f32(alloc + ALLOC_WEIGHT, clamp_f(resourceCap / pooled, 0.0f, 1.0f));
                    }
                }
            }
            uiStore = (i64*)shadowStore;
        }
    }

    g_originalGenericStorageUi(param1,param2,uiStore,param4,param5,param6,param7,param8,param9,
                               param10,param11,param12,param13,param14,param15,param16);
    g_uiReentrant = 0;
}

static void __fastcall simulation_dispatch_detour(void* game) {
    if (!g_originalDispatch) return;
    if (g_reentrant) {
        g_originalDispatch(game);
        return;
    }
    g_reentrant = 1;

    // Active relationships are maintained on every dispatcher pass; world
    // discovery/pair arbitration remains throttled.
    prepare_all_active_pairs(game);
    g_simulationWindowActive = 1;
    g_originalDispatch(game);
    g_simulationWindowActive = 0;
    publish_all_active_pairs(game);

    ++g_scanCounter;
    if (g_scanCounter >= SCAN_DIVISOR) {
        g_scanCounter = 0;
        process_world(game);
    }
    g_reentrant = 0;
}

static void __fastcall construction_tool_frame_detour(void* game) {
    if (!g_originalConstructionFrame) return;
    if (g_frameReentrant) {
        g_originalConstructionFrame(game);
        return;
    }
    g_frameReentrant = 1;

    // Replay only the previous frame's already-validated completed target before
    // WRSR advances the construction tool. This avoids touching transitional cargo
    // preview objects pre-frame while restoring the older native whole-building
    // green path that is known to be visible in WRSR's construction lifecycle.
    u8* preToolTemplate = current_tool_template(game);
    if (g_showPlacementOverlay && g_cachedOverlayTarget &&
        g_cachedOverlayToolTemplate == preToolTemplate &&
        completed_building(g_cachedOverlayTarget) && readable(g_cachedOverlayTarget, 1)) {
        queue_native_green_highlight(game, g_cachedOverlayTarget);
    } else if (g_cachedOverlayToolTemplate != preToolTemplate) {
        g_cachedOverlayTarget = nullptr;
        g_cachedOverlayToolTemplate = nullptr;
    }

    g_originalConstructionFrame(game);

    u8* chosenCompletedTarget = nullptr;
    u8* postToolTemplate = current_tool_template(game);
    u32 placementTransportMask = 0;
    if (g_showPlacementOverlay && current_tool_storage_transport_mask(game, &placementTransportMask)) {
        if (!g_placementTopologyPrimed || (g_currentHostLaneCount == 0 && g_currentStorageCount == 0)) {
            process_world(game);
            g_placementTopologyPrimed = 1;
        }
        const int cargoTool = current_tool_is_building_type(game, BUILDING_TYPE_CARGO_STATION);
        float gapSq = 0.0f;
        u8* pooledRoot = nullptr;
        u8* surfaceBuilding = nearest_attachment_to_preview(game, placementTransportMask,
                                                            &pooledRoot, &gapSq,
                                                            cargoTool ? 1 : 0);
        if (surfaceBuilding) {
            queue_native_green_highlight(game, surfaceBuilding);
            if (completed_building(surfaceBuilding)) chosenCompletedTarget = surfaceBuilding;
        }
    } else {
        // RESOURCE building placed after an existing STORAGE or DELIVERY building.
        u32 hostInputMask = 0, hostOutputMask = 0;
        if (g_showPlacementOverlay && current_tool_host_transport_masks(game, &hostInputMask, &hostOutputMask)) {
            float gapSq = 0.0f;
            u8* annex = nearest_existing_annex_to_host_preview(game, hostInputMask, hostOutputMask, &gapSq);
            if (annex) {
                queue_native_green_highlight(game, annex);
                if (completed_building(annex)) chosenCompletedTarget = annex;
            }
        } else {
            g_placementTopologyPrimed = 0;
        }
    }

    if (chosenCompletedTarget && postToolTemplate) {
        g_cachedOverlayTarget = chosenCompletedTarget;
        g_cachedOverlayToolTemplate = postToolTemplate;
    } else {
        g_cachedOverlayTarget = nullptr;
        g_cachedOverlayToolTemplate = nullptr;
    }
    g_frameReentrant = 0;
}

extern "C" __declspec(dllexport) unsigned TsmPluginApiVersion(void) { return 4; }

extern "C" __declspec(dllexport) int TsmPluginInit(const void* hostPtr, TsmPluginInfoCompat* info) {
    if (info) {
        info->name = PLUGIN_NAME;
        info->version = PLUGIN_VERSION;
    }
    const TsmHostCompat* host = (const TsmHostCompat*)hostPtr;
    if (!host || host->structSize < sizeof(TsmHostCompat) || host->apiVersion < 3 ||
        !host->exeBase || !host->installInlineHook || !host->readablePtr || !host->log) return 1;

    g_host = host;
    g_exeBase = host->exeBase;
    // The old aggregate-storage refresh RVA is still unverified on 1.1.1.13.
    // Native building-highlight is not yet mapped for 1.2.0.8. Keep that
    // optional integration disabled fail-closed; the vector push-back helper is verified.
    g_aggregateStorageTouch = nullptr;
    u8* highlightEntry = RVA_BUILDING_HIGHLIGHT ? g_exeBase + RVA_BUILDING_HIGHLIGHT : nullptr;
    g_buildingHighlight = highlightEntry && readable(highlightEntry, sizeof(BUILDING_HIGHLIGHT_EXPECTED)) &&
        bytes_equal(highlightEntry, BUILDING_HIGHLIGHT_EXPECTED, sizeof(BUILDING_HIGHLIGHT_EXPECTED))
        ? (BuildingHighlightFn)highlightEntry : nullptr;
    u8* vectorPushBackEntry = g_exeBase + RVA_VECTOR_PUSH_BACK_U64;
    g_vectorPushBackU64 = readable(vectorPushBackEntry, sizeof(VECTOR_PUSH_BACK_U64_EXPECTED)) &&
        bytes_equal(vectorPushBackEntry, VECTOR_PUSH_BACK_U64_EXPECTED, sizeof(VECTOR_PUSH_BACK_U64_EXPECTED))
        ? (VectorPushBackU64Fn)vectorPushBackEntry : nullptr;
    g_collision2DBBox = host->engineModule ? (Collision2DBBoxFn)((u8*)host->engineModule + RVA_C3D_COLLISION_2D_BBOX) : nullptr;
    g_getRotationStep = host->engineModule ? (GetRotationStepFn)((u8*)host->engineModule + RVA_C3D_GET_ROTATION_STEP) : nullptr;
    g_vec3TransformCoord = host->engineModule ? (Vec3TransformCoordFn)((u8*)host->engineModule + RVA_C3D_VEC3_TRANSFORM_COORD) : nullptr;

    if (!readable((void*)g_aggregateStorageTouch, 1)) g_aggregateStorageTouch = nullptr;
    if (!readable((void*)g_buildingHighlight, 1)) g_buildingHighlight = nullptr;
    if (!readable((void*)g_vectorPushBackU64, 1)) g_vectorPushBackU64 = nullptr;
    if (!readable((void*)g_collision2DBBox, 1)) g_collision2DBBox = nullptr;
    if (!readable((void*)g_getRotationStep, 1)) g_getRotationStep = nullptr;
    if (!readable((void*)g_vec3TransformCoord, 1)) g_vec3TransformCoord = nullptr;

    if (host->configInt) {
        int gap = host->configInt("EnhancedStorageManagement.ini", "StorageManagement", "LinkGapMeters", 5);
        if (gap < 2) gap = 2;
        if (gap > 20) gap = 20;
        g_linkGapMeters = gap;
        g_linkGapSq = (float)gap * (float)gap;
        g_showPlacementOverlay = host->configInt("EnhancedStorageManagement.ini", "StorageManagement", "ShowPlacementOverlay", 1) ? 1 : 0;
    }


    if (!g_collision2DBBox) {
        host->log("[EnhancedStorageManagement] required C3DDLL64 Collision2DBBox entry unavailable; footprint-gap storage-link discovery cannot run safely");
    }
    if (g_showPlacementOverlay && (!g_getRotationStep || !g_vec3TransformCoord)) {
        host->log("[EnhancedStorageManagement] C3D preview-footprint helpers unavailable; placement feedback will use the conservative cursor-origin fallback while completed storage-link mechanics remain active");
    }
    if (g_showPlacementOverlay && !g_buildingHighlight) {
        host->log("[EnhancedStorageManagement] native building-highlight entry unavailable or signature mismatch; placement feedback disabled while storage mechanics remain active");
    }
    if (!g_vectorPushBackU64) {
        host->log("[EnhancedStorageManagement] native temporary-vector grow helper unavailable; freight station connection-warning UI bridge will remain disabled fail-closed");
    }
    return 0;
}

extern "C" __declspec(dllexport) int TsmPluginStart(void) {
    if (!g_host || !g_exeBase || !g_host->installInlineHook) return 1;
    if (!g_collision2DBBox) {
        g_host->log("[EnhancedStorageManagement] native footprint collision service unavailable; plugin disabled rather than using an imprecise centre-radius fallback");
        return 1;
    }
    u8* simulationTarget = g_exeBase + RVA_BUILDING_SIMULATION_DISPATCH;
    u8 simulationBridge[ABSOLUTE_ENTRY_BRIDGE_SPAN] = {};
    const u8* simulationExpected = nullptr;
    usize simulationSpan = 0;
    const int simulationAdmission =
        hook_expected_bytes(simulationTarget, DISPATCH_EXPECTED, DISPATCH_HOOK_SPAN,
                            simulationBridge, &simulationExpected, &simulationSpan);
    if (!simulationAdmission) {
        g_host->log("[EnhancedStorageManagement] unsupported building-simulation entry; plugin disabled safely");
        return 1;
    }
    if (simulationAdmission == 2) {
        g_host->log("[EnhancedStorageManagement] building-simulation entry is already detoured; chaining ESM over the current effective owner");
    }

    void* simulationContinuation = nullptr;
    if (!g_host->installInlineHook(simulationTarget, (void*)&simulation_dispatch_detour, &simulationContinuation,
                                   simulationExpected, simulationSpan,
                                   "Enhanced Storage Management: wrsr.building.simulation_dispatch")) {
        g_host->log("[EnhancedStorageManagement] hook host refused building-simulation integration; plugin disabled");
        return 1;
    }
    if (!simulationContinuation || !readable(simulationContinuation, 1)) {
        g_host->log("[EnhancedStorageManagement] simulation hook installed without a readable continuation; plugin disabled");
        return 1;
    }
    g_originalDispatch = (SimulationDispatchFn)simulationContinuation;

    // FUN_140420350 is WRSR's central signed resource-transfer primitive. It is
    // the correct augmentation point for the unified-storage contract because
    // road/rail stations, factory connections, conveyors and other native
    // logistics callers converge here after WRSR has already established that
    // a transfer is legitimate. Enhanced Storage Management temporarily exposes the
    // linked physical pool through its canonical root, calls native transfer,
    // then redistributes and restores authored capacities before returning.
    {
        u8* transferTarget = g_exeBase + RVA_RESOURCE_TRANSFER;
        u8 transferBridge[ABSOLUTE_ENTRY_BRIDGE_SPAN] = {};
        usize transferSpan = 0;
        const u8* transferExpected = nullptr;
        if (!hook_expected_bytes(transferTarget, RESOURCE_TRANSFER_EXPECTED, RESOURCE_TRANSFER_HOOK_SPAN,
                                 transferBridge, &transferExpected, &transferSpan)) {
            g_host->log("[EnhancedStorageManagement] unified external transfer bridge unavailable on this executable state; simulation pooling remains active");
        } else {
            void* transferContinuation = nullptr;
            if (!g_host->installInlineHook(transferTarget, (void*)&resource_transfer_detour,
                                           &transferContinuation, transferExpected, transferSpan,
                                           "Enhanced Storage Management: wrsr.resource_transfer")) {
                g_host->log("[EnhancedStorageManagement] hook host refused unified transfer integration; simulation pooling remains active");
            } else if (!transferContinuation || !readable(transferContinuation, 1)) {
                g_host->log("[EnhancedStorageManagement] unified transfer continuation is invalid; plugin disabled safely");
                return 1;
            } else {
                g_originalResourceTransfer = (ResourceTransferFn)transferContinuation;
            }
        }
    }

    u8* uiTarget = g_exeBase + RVA_GENERIC_STORAGE_UI;
    u8 uiBridge[ABSOLUTE_ENTRY_BRIDGE_SPAN] = {};
    const u8* uiExpected = nullptr;
    usize uiSpan = 0;
    if (!hook_expected_bytes(uiTarget, GENERIC_UI_EXPECTED, GENERIC_UI_HOOK_SPAN,
                             uiBridge, &uiExpected, &uiSpan)) {
        g_host->log("[EnhancedStorageManagement] unsupported storage-UI entry; plugin disabled to avoid misleading pooled values");
        return 1;
    }
    void* uiContinuation = nullptr;
    if (!g_host->installInlineHook(uiTarget, (void*)&generic_storage_ui_detour, &uiContinuation,
                                   uiExpected, uiSpan,
                                   "Enhanced Storage Management: wrsr.generic_storage_ui")) {
        g_host->log("[EnhancedStorageManagement] hook host refused storage-UI integration; plugin disabled");
        return 1;
    }
    if (!uiContinuation || !readable(uiContinuation, 1)) {
        g_host->log("[EnhancedStorageManagement] generic-storage UI hook installed without a readable continuation; plugin disabled");
        return 1;
    }
    g_originalGenericStorageUi = (GenericStorageUiFn)uiContinuation;

    // Extend only WRSR's temporary connection query; persistent factory-connection
    // topology and save data remain untouched.
    if (g_vectorPushBackU64) {
        u8* connectionTarget = g_exeBase + RVA_FACTORY_CONNECTION_QUERY;
        u8 connectionBridge[ABSOLUTE_ENTRY_BRIDGE_SPAN] = {};
        usize connectionSpan = 0;
        const u8* connectionExpected = nullptr;
        if (!hook_expected_bytes(connectionTarget, FACTORY_QUERY_EXPECTED, FACTORY_QUERY_HOOK_SPAN,
                                 connectionBridge, &connectionExpected, &connectionSpan)) {
            g_host->log("[EnhancedStorageManagement] freight connection-status UI bridge unavailable; storage mechanics remain active");
        } else {
            void* connectionContinuation = nullptr;
            if (!g_host->installInlineHook(connectionTarget, (void*)&factory_connection_query_detour,
                                           &connectionContinuation, connectionExpected, connectionSpan,
                                           "Enhanced Storage Management: wrsr.factory_storage_connection_query")) {
                g_host->log("[EnhancedStorageManagement] hook host refused freight connection-status UI integration; mechanics remain active");
            } else if (!connectionContinuation || !readable(connectionContinuation, 1)) {
                g_host->log("[EnhancedStorageManagement] freight connection-status continuation is invalid; plugin disabled safely");
                return 1;
            } else {
                g_originalFactoryConnectionQuery = (FactoryConnectionQueryFn)connectionContinuation;
            }
        }
    }

    if (g_showPlacementOverlay) {
        u8* constructionTarget = g_exeBase + RVA_CONSTRUCTION_TOOL_FRAME;
        if (!readable(constructionTarget, CONSTRUCTION_HOOK_SPAN) || !g_buildingHighlight) {
            g_host->log("[EnhancedStorageManagement] placement feedback unavailable; pooled-storage mechanics remain active");
            g_showPlacementOverlay = 0;
        } else {
            u8 constructionBridge[ABSOLUTE_ENTRY_BRIDGE_SPAN] = {};
            const u8* constructionExpected = nullptr;
            usize constructionSpan = 0;
            const int constructionAdmission =
                hook_expected_bytes(constructionTarget, CONSTRUCTION_EXPECTED, CONSTRUCTION_HOOK_SPAN,
                                    constructionBridge, &constructionExpected, &constructionSpan);
            if (!constructionAdmission) {
                g_host->log("[EnhancedStorageManagement] placement target-discovery entry is unsupported; placement feedback disabled safely");
                g_showPlacementOverlay = 0;
            } else {
                if (constructionAdmission == 2) {
                    g_host->log("[EnhancedStorageManagement] construction entry is already detoured; chaining placement feedback over the current effective owner");
                }
                void* constructionContinuation = nullptr;
                if (!g_host->installInlineHook(constructionTarget, (void*)&construction_tool_frame_detour,
                                               &constructionContinuation, constructionExpected, constructionSpan,
                                               "Enhanced Storage Management: wrsr.construction.tool_frame")) {
                    g_host->log("[EnhancedStorageManagement] mod loader refused placement target-discovery hook; mechanics remain active without placement feedback");
                    g_showPlacementOverlay = 0;
                } else if (!constructionContinuation || !readable(constructionContinuation, 1)) {
                    g_host->log("[EnhancedStorageManagement] placement-feedback hook installed without a readable continuation; refusing unsafe partial hook");
                    return 1;
                } else {
                    g_originalConstructionFrame = (ConstructionToolFrameFn)constructionContinuation;
                }
            }
            if (g_showPlacementOverlay && (!g_originalConstructionFrame || !readable((void*)g_originalConstructionFrame, 1))) {
                g_host->log("[EnhancedStorageManagement] placement-feedback continuation unavailable after hook admission; disabling overlay rather than running a partial frame path");
                g_showPlacementOverlay = 0;
            }
        }
    }

    g_host->log("[EnhancedStorageManagement] %s active; unified storage pools enabled, link gap=%d m, placement feedback=%s",
                PLUGIN_VERSION, g_linkGapMeters,
                g_showPlacementOverlay ? "full building highlight" : "off");
    return 0;
}

extern "C" int __stdcall DllMain(void* module, unsigned reason, void*) {
    if (reason == 1) DisableThreadLibraryCalls(module);
    return g_aslrAnchor ? 1 : 0;
}
