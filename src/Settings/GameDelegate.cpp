#include "Settings/GameDelegate.hpp"

#include "RE/Offset.hpp"
#include "Settings/Collection.hpp"
#include "Settings/StaticCollection.hpp"

static constexpr bool DeferOptionChange = true;

static constexpr double ID_NewGame0 = 0;

namespace Settings
{
	struct HelpSetter
	{
		RE::GFxMovieView* movie;
		RE::GFxValue& entryObject;

		void operator()(const std::monostate&) {}
		void operator()(const std::string& help) { entryObject.SetMember("help", help.c_str()); }
		void operator()(const std::vector<std::string>& help)
		{
			RE::GFxValue items;
			movie->CreateArray(&items);
			for (const auto& str : help) {
				items.PushBack(str.c_str());
			}
			entryObject.SetMember("help", items);
		}
	};

	struct EntryDataSetter
	{
		RE::GFxMovieView* movie;
		RE::GFxValue& entryObject;

		using enum Settings::Type;

		void operator()(const Settings::ItemA<slider>& item)
		{
			entryObject.SetMember("movieType", 0);
			entryObject.SetMember("text", item.text.c_str());
			std::visit(HelpSetter{ movie, entryObject }, item.help);

			if (const auto& valueOptions = item.valueOptions) {
				entryObject.SetMember("defaultVal", item.valueOptions->defaultValue);
				entryObject.SetMember("value", item.FetchValue());

				entryObject.SetMember("minimum", item.valueOptions->min);
				entryObject.SetMember("maximum", item.valueOptions->max);
				entryObject.SetMember("interval", item.valueOptions->step);
			}
		}

		void operator()(const Settings::ItemA<stepper>& item)
		{
			entryObject.SetMember("movieType", 1);
			entryObject.SetMember("text", item.text.c_str());
			std::visit(HelpSetter{ movie, entryObject }, item.help);

			if (const auto& valueOptions = item.valueOptions) {
				entryObject.SetMember("defaultVal", item.valueOptions->defaultValue);
				entryObject.SetMember("value", item.FetchValue());

				RE::GFxValue options;
				movie->CreateArray(&options);
				for (const auto& option : item.valueOptions->options) {
					options.PushBack(option.c_str());
				}
				entryObject.SetMember("options", options);
			}
		}

		void operator()(const Settings::ItemA<toggle>& item)
		{
			entryObject.SetMember("movieType", 2);
			entryObject.SetMember("text", item.text.c_str());
			std::visit(HelpSetter{ movie, entryObject }, item.help);

			if (const auto& valueOptions = item.valueOptions) {
				entryObject.SetMember("defaultVal", item.valueOptions->defaultValue);
				entryObject.SetMember("value", item.FetchValue());
			}
		}
	};

	[[nodiscard]] static const Item* QueryID(double ID)
	{
		if (ID >= ID_NewGame0) {
			std::size_t index = static_cast<std::size_t>(ID - ID_NewGame0);
			if (index < StaticCollection::Instance.NewGame.size()) {
				return std::addressof(StaticCollection::Instance.NewGame[index]);
			}
		}

		return nullptr;
	}

	[[nodiscard]] static bool IsPrefSetting(double ID)
	{
		if (const auto item = QueryID(ID)) {
			return std::visit(
				[](auto&& item)
				{
					return item.valueOptions &&
						item.valueOptions->sourceType == Settings::SourceType::INIPrefSetting;
				},
				*item);
		}

		return true;
	}

	static void StoreValue(double ID, double value)
	{
		if (const auto item = QueryID(ID)) {
			std::visit(
				[value](auto&& item)
				{
					item.StoreValue(value);
				},
				*item);
			return;
		}

		logger::error("Failed to resolve option ID: {}"sv, ID);
	}

	void RequestNewGameOptions(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			logger::error(
				"RequestNewGameOptions: Expected 1 argument, received {}",
				a_params.GetArgCount());
			return;
		}

		RE::GFxMovieView* const movie = a_params.GetMovie();
		if (!movie) {
			return;
		}

		RE::GFxValue entryList = a_params[0];

		if (!entryList.IsArray()) {
			logger::error("RequestNewGameOptions: Expected [Array]");
			return;
		}

		const auto& collection = Settings::StaticCollection::Instance;
		for (auto&& [i, item] : std::views::enumerate(collection.NewGame)) {
			RE::GFxValue entryObject;
			movie->CreateObject(&entryObject);
			std::visit(EntryDataSetter{ movie, entryObject }, item);
			entryObject.SetMember("ID", ID_NewGame0 + i);
			entryList.PushBack(entryObject);
		}
	}

	void OptionChange(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 2) {
			logger::error(
				"OptionChange: Expected 2 arguments, received {}",
				a_params.GetArgCount());
			return;
		}

		RE::GFxValue ID = a_params[0];
		RE::GFxValue value = a_params[1];

		if (!ID.IsNumber() || !value.IsNumber()) {
			logger::error("OptionChange: Expected [Number, Number]", a_params.GetArgCount());
			return;
		}

		if constexpr (!DeferOptionChange) {
			StoreValue(ID.GetNumber(), value.GetNumber());
		}

		RE::GFxMovieView* const movie = a_params.GetMovie();
		if (!movie) {
			return;
		}

		if (IsPrefSetting(ID.GetNumber())) {
			movie->SetVariable(
				"_root.MenuHolder.Menu_mc.bPrefsChanged",
				true,
				RE::GFxMovie::SetVarType::kNormal);
		}
	}

	void SaveSettings(const RE::FxDelegateArgs& a_params)
	{
		RE::GFxMovieView* const movie = a_params.GetMovie();
		if (!movie) {
			return;
		}

		if constexpr (DeferOptionChange) {
			RE::GFxValue entryList;
			movie->GetVariable(
				&entryList,
				"_root.MenuHolder.Menu_mc.SettingsPanel_mc.List_mc.entryList");
			if (!entryList.IsArray()) {
				return;
			}

			for (const auto i : std::views::iota(0U, entryList.GetArraySize())) {
				RE::GFxValue entryObject;
				entryList.GetElement(i, &entryObject);
				if (!entryObject.IsObject()) {
					continue;
				}

				RE::GFxValue ID, value;
				entryObject.GetMember("ID", &ID);
				entryObject.GetMember("value", &value);
				if (!ID.IsNumber() || !value.IsNumber()) {
					return;
				}

				StoreValue(ID.GetNumber(), value.GetNumber());
			}
		}

		RE::GFxValue bPrefsChanged;
		if (movie->GetVariable(&bPrefsChanged, "_root.MenuHolder.Menu_mc.bPrefsChanged")) {
			if (bPrefsChanged.IsBool() && bPrefsChanged.GetBool()) {
				const auto prefSettings = RE::INIPrefSettingCollection::GetSingleton();
				if (prefSettings && prefSettings->OpenHandle(true)) {
					prefSettings->WriteSettings();
					prefSettings->CloseHandle();
				}
			}
		}

		RE::FxResponseArgs<0> response;
		RE::FxDelegate::Invoke(movie, "SettingsSaved", response);
	}
}
