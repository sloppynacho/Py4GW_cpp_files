
#pragma once

class PyPointers {
public:
	static uintptr_t GetMissionMapContextPtr();
	static uintptr_t GetWorldMapContextPtr();
	static uintptr_t GetGameplayContextPtr();
	static uintptr_t GetAreaInfoPtr();
	static uintptr_t GetMapContextPtr();
	static uintptr_t GetGameContextPtr();
	static uintptr_t GetPreGameContextPtr();
	static uintptr_t GetWorldContextPtr();
	static uintptr_t GetCharContextPtr();
	static uintptr_t GetAgentContextPtr();
	static uintptr_t GetCinematicPtr();
	static uintptr_t GetGuildContextPtr();
};

uintptr_t  PyPointers::GetWorldMapContextPtr(){
	return reinterpret_cast<uintptr_t>(GW::Map::GetWorldMapContext());
}

uintptr_t  PyPointers::GetMissionMapContextPtr(){
	return reinterpret_cast<uintptr_t>(GW::Map::GetMissionMapContext());
}

uintptr_t PyPointers::GetGameplayContextPtr(){
	return reinterpret_cast<uintptr_t>(GW::GetGameplayContext());
}

uintptr_t PyPointers::GetAreaInfoPtr(){
	return reinterpret_cast<uintptr_t>(GW::Map::GetMapInfo(GW::Map::GetMapID()));
}

uintptr_t PyPointers::GetMapContextPtr(){
	return reinterpret_cast<uintptr_t>(GW::GetMapContext());
}

uintptr_t PyPointers::GetGameContextPtr() {
	return reinterpret_cast<uintptr_t>(GW::GetGameContext());
}

uintptr_t PyPointers::GetPreGameContextPtr() {
	return reinterpret_cast<uintptr_t>(GW::GetPreGameContext());
}

uintptr_t PyPointers::GetWorldContextPtr() {
	return reinterpret_cast<uintptr_t>(GW::GetWorldContext());
}

uintptr_t PyPointers::GetCharContextPtr() {
	return reinterpret_cast<uintptr_t>(GW::GetCharContext());
}

uintptr_t PyPointers::GetAgentContextPtr() {
	return reinterpret_cast<uintptr_t>(GW::GetAgentContext());
}

uintptr_t PyPointers::GetCinematicPtr() {
	auto* game_context = GW::GetGameContext();
	return reinterpret_cast<uintptr_t>(game_context->cinematic);
}

uintptr_t PyPointers::GetGuildContextPtr() {
	auto* game_context = GW::GetGameContext();
	return reinterpret_cast<uintptr_t>(game_context->guild);
}


PYBIND11_EMBEDDED_MODULE(PyPointers, m) {

	py::class_<PyPointers>(m, "PyPointers")
		.def_static("GetMissionMapContextPtr", &PyPointers::GetMissionMapContextPtr)
		.def_static("GetWorldMapContextPtr", &PyPointers::GetWorldMapContextPtr)
		.def_static("GetGameplayContextPtr", &PyPointers::GetGameplayContextPtr)
		.def_static("GetMapContextPtr", &PyPointers::GetMapContextPtr)
		.def_static("GetAreaInfoPtr", &PyPointers::GetAreaInfoPtr)
		.def_static("GetGameContextPtr", &PyPointers::GetGameContextPtr)
		.def_static("GetPreGameContextPtr", &PyPointers::GetPreGameContextPtr)
		.def_static("GetWorldContextPtr", &PyPointers::GetWorldContextPtr)
		.def_static("GetCharContextPtr", &PyPointers::GetCharContextPtr)
		.def_static("GetAgentContextPtr", &PyPointers::GetAgentContextPtr)
		.def_static("GetCinematicPtr", &PyPointers::GetCinematicPtr)
		.def_static("GetGuildContextPtr", &PyPointers::GetGuildContextPtr)
		;

}

