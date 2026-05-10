#include "Settings/StaticCollection.hpp"

namespace Settings::StaticCollection
{
	Collection Instance;

	void Load()
	{
		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return;
		}

		for (const auto& file : dataHandler->files) {
			if (!file || file->recordFlags.none(RE::TESFile::RecordFlag::kChecked)) {
				continue;
			}

			auto fileName = std::filesystem::path(file->fileName);
			fileName.replace_extension("json"sv);

			const auto directory =
				std::filesystem::path(fmt::format("SKSE/Plugins/{}"sv, Plugin::NAME));
			const auto configFile = directory / fileName;

			RE::BSResourceNiBinaryStream resource{ configFile.string() };
			if (!resource.good()) {
				continue;
			}

			const auto size = resource.stream->totalSize;
			const auto buffer = std::make_unique<char[]>(size);
			resource.read(buffer.get(), size);

			const auto error = glz::read<glz::opts{
				.comments = true,
				.error_on_unknown_keys = false,
			}>(Instance, std::string_view{ buffer.get(), size });

			if (error) {
				logger::error("Error occurred parsing config at position {}"sv, error.count);
			}
		}
	}
}
