#pragma once

#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <string>

static constexpr uint32_t agent_array_max_size = 300;

struct SharedMemoryHeader {
    uint32_t version = 1;
    uint32_t total_size = 0;
    uint32_t sequence = 0;
    uint32_t process_id = 0;
    uint64_t window_handle = 0;
};

#pragma pack(push, 1)
struct TimeSharedMemoryExample {
    uint64_t tick_count = 0;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Agent_SHMemStruct {
    uintptr_t ptr = 0;
    uint32_t agent_id = 0;
};

struct AgentRef_SHMemStruct {
    uint32_t agent_id = 0;
    uint32_t index = 0;
};

struct AgentRefArray_SHMemStruct {
    uint32_t count = 0;
    AgentRef_SHMemStruct entries[agent_array_max_size] = {};
};

struct AgentArray_SHMemStruct {
	uint32_t max_size = agent_array_max_size;
	uint32_t AgentArrayCount = 0;
    Agent_SHMemStruct AgentArray[agent_array_max_size] = {};
    AgentRefArray_SHMemStruct AllArray = {};
    AgentRefArray_SHMemStruct AllyArray = {};
    AgentRefArray_SHMemStruct NeutralArray = {};
    AgentRefArray_SHMemStruct EnemyArray = {};
    AgentRefArray_SHMemStruct SpiritPetArray = {};
    AgentRefArray_SHMemStruct MinionArray = {};
    AgentRefArray_SHMemStruct NPCMinipetArray = {};
    AgentRefArray_SHMemStruct LivingArray = {};
    AgentRefArray_SHMemStruct ItemArray = {};
    AgentRefArray_SHMemStruct OwnedItemArray = {};
    AgentRefArray_SHMemStruct GadgetArray = {};
    AgentRefArray_SHMemStruct DeadAllyArray = {};
    AgentRefArray_SHMemStruct DeadEnemyArray = {};
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Pointers_SHMemStruct {
	uintptr_t MissionMapContext = 0;
	uintptr_t WorldMapContext = 0;
	uintptr_t GameplayContext = 0;
	uintptr_t InstanceInfo = 0;
	uintptr_t MapContext = 0;
	uintptr_t GameContext = 0;
	uintptr_t PreGameContext = 0;
	uintptr_t WorldContext = 0;
	uintptr_t CharContext = 0;
	uintptr_t AgentContext = 0;
	uintptr_t CinematicContext = 0;
	uintptr_t GuildContext = 0;
	uintptr_t AvailableCharacters = 0;
	uintptr_t PartyContext = 0;
	uintptr_t ServerRegionContext = 0;
};

#pragma pack(pop)


class SharedMemoryManager {
public:
    SharedMemoryManager() = default;
    ~SharedMemoryManager();

    SharedMemoryManager(const SharedMemoryManager&) = delete;
    SharedMemoryManager& operator=(const SharedMemoryManager&) = delete;

    bool Create(const std::wstring& name, size_t total_size);
    void Destroy();

    bool IsValid() const;
    void* Data() const;
    size_t Size() const;
    const std::wstring& Name() const;

    SharedMemoryHeader* Header() const;
    // Writer-side sync helpers for the sequence-lock in SharedMemoryHeader.
    // Readers should only trust snapshots where:
    // 1. the first observed sequence is even
    // 2. the sequence did not change after reading
    void BeginWrite() const;
    void EndWrite() const;

    // Pointer to the bytes immediately after the header. This is where
    // payload structs should normally live.
    void* PayloadData(size_t offset = sizeof(SharedMemoryHeader)) const;

    template <typename T>
    T* PayloadAs(size_t offset = sizeof(SharedMemoryHeader)) const {
        return static_cast<T*>(PayloadData(offset));
    }

    bool CreateRuntimeRegion(const std::wstring& name);

	//AgentArray
    bool UpdateAgentArrayRegion();
    AgentArray_SHMemStruct* AgentArraySMStruct() const;

    static constexpr size_t AgentArrayPayloadOffset() {
        return sizeof(SharedMemoryHeader);
    }

    static constexpr size_t AgentArraySMStructSize() {
        return sizeof(AgentArray_SHMemStruct);
    }

    static std::wstring BuildName(const wchar_t* prefix, DWORD process_id, HWND window_handle = nullptr);

	// Pointers struct
	bool UpdatePointersRegion();
	Pointers_SHMemStruct* PointersSMStruct() const;
    static constexpr size_t PointersPayloadOffset() {
        return AgentArrayPayloadOffset() + AgentArraySMStructSize();
    }

	static constexpr size_t PointersSMStructSize() {
		return sizeof(Pointers_SHMemStruct);
	}

    static constexpr size_t RuntimeSMStructSize() {
        return PointersPayloadOffset() + PointersSMStructSize();
    }

private:
    HANDLE mapping_handle_ = nullptr;
    void* view_ = nullptr;
    size_t total_size_ = 0;
    std::wstring name_;
};
