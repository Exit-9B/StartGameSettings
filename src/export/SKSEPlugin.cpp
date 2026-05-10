#include "Hooks/Difficulty.hpp"
#include "Hooks/MainMenu.hpp"
#include "Settings/StaticCollection.hpp"

namespace
{
	void InitializeLog()
	{
#ifndef NDEBUG
		auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
		auto path = logger::log_directory();
		if (!path) {
			util::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= fmt::format("{}.log"sv, Plugin::NAME);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
		const auto level = spdlog::level::trace;
#else
		const auto level = spdlog::level::info;
#endif

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(level);
		log->flush_on(level);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("%s(%#): [%^%l%$] %v"s);
	}
}

#ifndef SKYRIMVR
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []()
{
	SKSE::PluginVersionData v{};

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);
	v.AuthorName("Parapets"sv);

	v.UsesAddressLibrary(true);
	v.HasNoStructUse(true);
	v.UsesStructsPost629(false);

	return v;
}();
#endif

extern "C" DLLEXPORT bool SKSEAPI
SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
#ifndef SKYRIMVR
	if (ver < SKSE::RUNTIME_1_6_318) {
		return false;
	}
#else
	if (ver != SKSE::RUNTIME_VR_1_4_15_1) {
		return false;
	}
#endif

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	logger::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(8);

	Hooks::MainMenu::Install();
	Hooks::Difficulty::Install();

	SKSE::GetMessagingInterface()->RegisterListener(
		[](auto a_msg)
		{
			switch (a_msg->type) {
			case SKSE::MessagingInterface::kDataLoaded:
			{
				SKSE::Translation::ParseTranslation(Plugin::NAME.data());
				Settings::StaticCollection::Load();
				if (const auto dataHandler = RE::TESDataHandler::GetSingleton()) {
					const auto global = dataHandler->LookupForm<RE::TESGlobal>(
						0x800,
						"StartGameSettings_Difficulty.esp"sv);
					Hooks::Difficulty::SetGlobal(global);
				}
			} break;
			case SKSE::MessagingInterface::kNewGame:
			{
				Hooks::Difficulty::OnNewGame();
			} break;
			case SKSE::MessagingInterface::kPostLoadGame:
			{
				Hooks::Difficulty::OnLoadGame();
			} break;
			}
		});

	return true;
}
